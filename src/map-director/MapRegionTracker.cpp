/******************************************************************************

    GPLv3 License
    Copyright (c) 2023 Aria Janke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*****************************************************************************/

#include "MapRegionTracker.hpp"
#include "TileFactory.hpp"

namespace {

class EntityAndLinkInsertingAdder final : public EntityAndTrianglesAdder {
public:
    using ViewGridTriangle = TeardownTask::ViewGridTriangle;
    using ViewGridTriangleInserter = ViewGridTriangle::Inserter;

    explicit EntityAndLinkInsertingAdder(const Size2I & grid_size):
        m_triangle_inserter(grid_size) {}

    void add_triangle(const TriangleSegment & triangle) final
        { m_triangle_inserter.push(triangle); }

    void add_entity(const Entity & ent) final
        { m_entities.push_back(ent); }

    std::vector<Entity> move_out_entities()
        { return std::move(m_entities); }

    void advance_grid_position()
        { m_triangle_inserter.advance(); }

    ViewGridTriangle finish_triangle_grid() {
        return m_triangle_inserter.
            transform_values<SharedPtr<TriangleLink>>(to_link).
            finish();
    }

private:
    static SharedPtr<TriangleLink> to_link(const TriangleSegment & segment)
        { return make_shared<TriangleLink>(segment); }

    ViewGridInserter<TriangleSegment> m_triangle_inserter;
    std::vector<Entity> m_entities;
};

void link_triangles(EntityAndLinkInsertingAdder::ViewGridTriangle &);

} // end of <anonymous> namespace

void MapRegionContainer::on_complete
    (const Vector2I & region_position,
     InterTriangleLinkContainer && link_container,
     SharedPtr<TeardownTask> && teardown_task)
{
    static constexpr const auto k_neighbors = k_plus_shape_neighbor_offsets;

    auto * loaded_region = find(region_position);
    if (!loaded_region) return;
    loaded_region->link_edge_container = std::move(link_container);
    loaded_region->teardown = std::move(teardown_task);

    for (auto n : k_neighbors) {
        auto neighbor = find(n + region_position);
        if (!neighbor) continue;
        loaded_region->link_edge_container.glue_to
            (neighbor->link_edge_container);
    }
}

void MapRegionContainer::ensure_region_available(const Vector2I & r)
    { (void)m_loaded_regions[r]; }

bool MapRegionContainer::find_and_keep(const Vector2I & r) {
    if (auto * region = find(r)) {
        return region->keep_on_refresh = true;
    }
    return false;
}

void MapRegionContainer::frame_refresh(TaskCallbacks & callbacks) {
    for (auto itr = m_loaded_regions.begin(); itr != m_loaded_regions.end(); ) {
        if (itr->second.keep_on_refresh) {
            itr->second.keep_on_refresh = false;
            ++itr;
        } else {
            // teardown task handles removal of entities and physical triangles
            callbacks.add(itr->second.teardown);
            itr = m_loaded_regions.erase(itr);
        }
    }
}

/* private */ MapRegionContainer::LoadedMapRegion *
    MapRegionContainer::find(const Vector2I & r)
{
    auto itr = m_loaded_regions.find(r);
    if (itr == m_loaded_regions.end())
        return nullptr;
    return &itr->second;
}

// ----------------------------------------------------------------------------

MapRegionTracker::MapRegionTracker
    (UniquePtr<MapRegion> && root_region,
     const Size2I & region_size_in_tiles):
    m_root_region(std::move(root_region)),
    m_region_size_in_tiles(region_size_in_tiles) {}

void MapRegionTracker::frame_refresh(TaskCallbacks & callbacks) {
    m_loaded_regions.frame_refresh(callbacks);
}

void MapRegionTracker::frame_hit
    (const Vector2I & global_region_location, TaskCallbacks & callbacks)
{
    if (!m_root_region) return;
    if (m_loaded_regions.find_and_keep(global_region_location))
        return;

    m_loaded_regions.ensure_region_available(global_region_location);
    Vector2I region_tile_offset
        {global_region_location.x*m_region_size_in_tiles.width,
         global_region_location.y*m_region_size_in_tiles.height};
    auto region_preparer = make_shared<MapRegionPreparer>(region_tile_offset);
    region_preparer->set_completer
        (MapRegionCompleter{global_region_location, m_loaded_regions});
    m_root_region->request_region_load(global_region_location, region_preparer, callbacks);
}

void MapRegionTracker::frame_hit
    (const RegionLoadRequest & request, TaskCallbacks & callbacks)
{
    if (!m_root_region_n) return;
    // issue request to region, with container, done

    m_container_n.decay_regions(callbacks);
    m_root_region_n->process_load_request
        (request, Vector2I{}, m_container_n, callbacks);
}

// ----------------------------------------------------------------------------

struct FinishedLoader final {
    using ViewGridTriangle = TeardownTask::ViewGridTriangle;
    InterTriangleLinkContainer link_edge_container;
    std::vector<Entity> entities;
    ViewGridTriangle triangle_grid;
};

class LoaderImpl final : public RegionLoadCollectorN {
public:
    explicit LoaderImpl(Vector2I offset):
        m_offset(offset) {}

    void add_tiles(const ProducableSubGrid & producables, Platform & platform_) final {
        EntityAndLinkInsertingAdder triangle_entities_adder{producables.size2()};
        for (auto & producables_view : producables) {
            for (auto producable : producables_view) {
                if (!producable) continue;
                (*producable)(m_offset, triangle_entities_adder, platform_);
            }
            triangle_entities_adder.advance_grid_position();
        }
        m_result.triangle_grid = triangle_entities_adder.finish_triangle_grid();
        m_result.entities = triangle_entities_adder.move_out_entities();
        m_result.link_edge_container =
            InterTriangleLinkContainer{m_result.triangle_grid};
    }

    FinishedLoader finish() {
        link_triangles(m_result.triangle_grid);
        return std::move(m_result);
    }

private:
    Platform & platform() {
        using namespace cul::exceptions_abbr;
        if (m_platform) return *m_platform;
        throw RtError{"RegionLoadCollector::add_tiles: intialized without a "
                      "platform"};
    }

    FinishedLoader m_result;
    Vector2I m_offset;
    Platform * m_platform = nullptr;
};


void MapRegionContainerN::refresh_or_load_
    (const Vector2I & r, const LoaderCallback & f)
{
    if (auto * ptr = find(r))
        { return; }

    LoaderImpl impl{r};
    f(impl);

    auto & region = m_loaded_regions[r];
    auto res = impl.finish();

    region.teardown = make_shared<TeardownTask>
        (std::move(res.entities), res.triangle_grid.elements());
    region.link_edge_container = std::move(res.link_edge_container);
    region.keep_on_refresh = true;
}

// ----------------------------------------------------------------------------

namespace {

void link_triangles
    (EntityAndLinkInsertingAdder::ViewGridTriangle & link_grid)
{
    // now link them together
    for (Vector2I r; r != link_grid.end_position(); r = link_grid.next(r)) {
    for (auto & this_tri : link_grid(r)) {
        assert(this_tri);
        for (Vector2I v : { r, Vector2I{1, 0} + r, Vector2I{-1,  0} + r,
/*                          */ Vector2I{0, 1} + r, Vector2I{ 0, -1} + r}) {
            if (!link_grid.has_position(v)) continue;
            for (auto & other_tri : link_grid(v)) {
                assert(other_tri);

                if (this_tri == other_tri) continue;
                this_tri->attempt_attachment_to(other_tri);
        }}
    }}
}

} // end of <anonymous> namespace

