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

#include "MapRegionChangesTask.hpp"
#include "TileFactory.hpp"

namespace {

class EntityAndLinkInsertingAdder final : public EntityAndTrianglesAdder {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;
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

RegionLoadJob::RegionLoadJob
    (const Vector2I & on_field_position,
     const Vector2I & maps_offset,
     const ProducableSubGrid & subgrid):
    m_on_field_position(on_field_position),
    m_maps_offset(maps_offset),
    m_subgrid(subgrid) {}

void RegionLoadJob::operator ()
    (MapRegionContainer & container,
     RegionEdgeConnectionsAdder & edge_container_adder,
     LoaderTask::Callbacks & callbacks) const
{
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;
    const auto & producables = m_subgrid;
    EntityAndLinkInsertingAdder triangle_entities_adder{producables.size2()};
    for (auto & producables_view : producables) {
        for (auto producable : producables_view) {
            if (!producable) continue;
            // producables need a map offset, not a region offset!
            auto offset = m_maps_offset;
            auto & platform = callbacks.platform();
            (*producable)(offset, triangle_entities_adder, platform);
        }
        triangle_entities_adder.advance_grid_position();
    }

    auto triangle_grid =
        make_shared<ViewGridTriangle>(triangle_entities_adder.finish_triangle_grid());
    auto entities = triangle_entities_adder.move_out_entities();

    for (auto & link : triangle_grid->elements()) {
        callbacks.add(link);
    }
    for (auto & ent : entities) {
        callbacks.add(ent);
    }

    link_triangles(*triangle_grid);
    container.set_region(m_on_field_position, triangle_grid,
                         std::move(entities));
    edge_container_adder.add(m_on_field_position, std::move(triangle_grid));
}

// ----------------------------------------------------------------------------

RegionDecayJob::RegionDecayJob
    (const RectangleI & subgrid_bounds,
     SharedPtr<ViewGridTriangle> && triangle_grid,
     std::vector<Entity> && entities):
    m_on_field_subgrid(subgrid_bounds),
    m_triangle_grid(std::move(triangle_grid)),
    m_entities(std::move(entities)) {}

void RegionDecayJob::operator ()
    (RegionEdgeConnectionsRemover & connection_remover,
     LoaderTask::Callbacks & callbacks) const
{
    for (auto ent : m_entities)
        { ent.request_deletion(); }
    for (auto & linkptr : m_triangle_grid->elements())
        { callbacks.remove(linkptr); }
    connection_remover.remove_region
        (cul::top_left_of(m_on_field_subgrid),
         m_triangle_grid);
}

// ----------------------------------------------------------------------------

RegionLoadCollector::RegionLoadCollector
    (MapRegionContainer & container_):
    m_container(container_) {}

void RegionLoadCollector::add_tiles
    (const Vector2I & on_field_position,
     const Vector2I & maps_offset,
     const ProducableSubGrid & subgrid)
{
    if (auto refresh = m_container.region_refresh_at(on_field_position)) {
        refresh->keep_this_frame();
        return;
    }

    m_entries.emplace_back(on_field_position, maps_offset, subgrid);
}

RegionDecayCollector RegionLoadCollector::finish()
    { return RegionDecayCollector{std::move(m_entries)}; }

// ----------------------------------------------------------------------------

RegionDecayCollector::RegionDecayCollector
    (std::vector<RegionLoadJob> && load_entries):
    m_load_entries(std::move(load_entries)) {}

void RegionDecayCollector::add
    (const Vector2I & on_field_position,
     const Size2I & grid_size,
     SharedPtr<ViewGridTriangle> && triangle_links,
     std::vector<Entity> && entities)
{
    m_decay_entries.emplace_back
        (RectangleI{on_field_position, grid_size},
         std::move(triangle_links),
         std::move(entities));
}

SharedPtr<LoaderTask> RegionDecayCollector::finish
    (RegionEdgeConnectionsContainer & edge_container,
     MapRegionContainer & container)
{
    if (m_load_entries.empty() && m_decay_entries.empty())
        { return nullptr; }
    return make_shared<MapRegionChangesTask>
        (std::move(m_load_entries), std::move(m_decay_entries),
         edge_container, container);
}

// ----------------------------------------------------------------------------

MapRegionChangesTask::MapRegionChangesTask
    (std::vector<RegionLoadJob> && load_entries,
     std::vector<RegionDecayJob> && decay_entries,
     RegionEdgeConnectionsContainer & edge_container,
     MapRegionContainer & container):
    m_load_entries(std::move(load_entries)),
    m_decay_entries(std::move(decay_entries)),
    m_edge_container(edge_container),
    m_container(container) {}

void MapRegionChangesTask::operator ()
    (LoaderTask::Callbacks & callbacks) const
{
    auto adder = m_edge_container.make_adder();
    for (auto & load_entry : m_load_entries) {
        load_entry(m_container, adder, callbacks);
    }

    auto remover = adder.finish().make_remover();
    for (auto & decay_entry : m_decay_entries) {
        decay_entry(remover, callbacks);
    }
    m_edge_container = remover.finish();
}

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
                TriangleLink::attach_matching_points(this_tri, other_tri);
#               if 0
                this_tri->attempt_attachment_to(other_tri);
#               endif
        }}
    }}
}

} // end of <anonymous> namespace
