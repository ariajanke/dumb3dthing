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

struct RegionEntry final {
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    Vector2I on_field_position;
    Vector2I maps_offset;
    ProducableSubGrid subgrid;
};

class MapObjectsLoaderTask final : public LoaderTask {
public:
    MapObjectsLoaderTask
        (std::vector<RegionEntry> && entries, MapRegionContainer & container):
        m_entries(std::move(entries)), m_container(container) {}

    void operator () (LoaderTask::Callbacks & callbacks) const final {
        for (auto & entry : m_entries) process_entry(entry, callbacks);
        // glue_to_neighbors(?) on an instance of InterRegionLinkContainer
    }

private:
    void process_entry
        (const RegionEntry & entry, LoaderTask::Callbacks & callbacks) const
    {
        const auto & producables = entry.subgrid;
        EntityAndLinkInsertingAdder triangle_entities_adder{producables.size2()};
        for (auto & producables_view : producables) {
            for (auto producable : producables_view) {
                if (!producable) continue;
                // producables need a map offset, not a region offset!
                auto offset = entry.maps_offset;
                auto & platform = callbacks.platform();
                (*producable)(offset, triangle_entities_adder, platform);
            }
            triangle_entities_adder.advance_grid_position();
        }
        // need to add entities, and links
        // ...

        auto triangle_grid = triangle_entities_adder.finish_triangle_grid();
        auto entities = triangle_entities_adder.move_out_entities();
        InterTriangleLinkContainer link_edge_container{triangle_grid};
        // need to defer to a loader task
        for (auto & link : triangle_grid.elements()) {
            callbacks.add(link);
        }
        for (auto & ent : entities) {
            callbacks.add(ent);
        }

        // link triangles within region
        link_triangles(triangle_grid);
        // link to neighbors by container
        // I don't know what the neighbors are here
        m_container.set_region(entry.on_field_position, triangle_grid.size2(),
                               std::move(entities));
        // accumulate things that address inter triangle link containers
        // add on an instance of RegionEdgeConnectionAdder
    }

    std::vector<RegionEntry> m_entries;
    MapRegionContainer & m_container;
};

class RegionLoadCollectorComplete final : public RegionLoadCollector {
public:
    explicit RegionLoadCollectorComplete(MapRegionContainer & container_):
        m_container(container_) {}

    // change around params s.t. I know how to make the subgrid and that
    // subgrid's position on the overall map
    void add_tiles
        (const Vector2I & on_field_position, const Vector2I & maps_offset,
         const ProducableSubGrid & subgrid)
    {
        if (auto refresh = m_container.region_refresh_at(on_field_position)) {
            refresh->keep_this_frame();
            return;
        }

        RegionEntry entry;
        entry.on_field_position = on_field_position;
        entry.maps_offset = maps_offset;
        entry.subgrid = subgrid;
        m_entries.push_back(entry);
    }

    SharedPtr<LoaderTask> finish() {
        if (m_entries.empty()) return nullptr;
        return make_shared<MapObjectsLoaderTask>(std::move(m_entries), m_container);
    }

private:
    std::vector<RegionEntry> m_entries;
    MapRegionContainer & m_container;
};

} // end of <anonymous> namespace

void MapRegionTracker::frame_hit
    (const RegionLoadRequest & request, TaskCallbacks & callbacks)
{
    if (!m_root_region_n) return;
    // need another container type for links
    // need to figure out why region is being unloaded when not called for

    RegionLoadCollectorComplete collector{m_container_n};
    m_root_region_n->process_load_request(request, Vector2I{}, collector);
    m_container_n.decay_regions(callbacks);
    callbacks.add(collector.finish());
}

// ----------------------------------------------------------------------------

struct FinishedLoader final {
    using ViewGridTriangle = TeardownTask::ViewGridTriangle;
    InterTriangleLinkContainer link_edge_container;
    std::vector<Entity> entities;
    ViewGridTriangle triangle_grid;
};

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
