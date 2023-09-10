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

using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

// has two jobs :/
// becomes an entity and link grid adder
class EntityAndLinkInsertingAdder final : public ProducableTileCallbacks {
public:
    EntityAndLinkInsertingAdder
        (LoaderTask::Callbacks &,
         Size2I grid_size,
         const TilePositionFraming &);

    SharedPtr<RenderModel> make_render_model() final;

    void add_collidable_(const TriangleSegment &) final;

    Entity add_entity_() final;

    void advance_grid_position()
        { m_tile_framing = m_tile_framing.advance_with(m_triangle_inserter); }

    void add_loader(const SharedPtr<LoaderTask> &) final {}

    std::vector<Entity> finish_adding_entites();

    SharedPtr<ViewGridTriangle> finish_adding_triangles();

    // wants a finish method

private:
    ModelScale model_scale() const final
        { return m_tile_framing.model_scale(); }

    ModelTranslation model_translation() const final
        { return m_tile_framing.model_translation(); }

    ViewGridTriangle finish_triangle_grid();

    LoaderTask::Callbacks & m_callbacks;
    ViewGridInserter<TriangleSegment> m_triangle_inserter;
    std::vector<Entity> m_entities;
    TilePositionFraming m_tile_framing;
};

void link_triangles(ViewGridTriangle &);

} // end of <anonymous> namespace

RegionLoadJob::RegionLoadJob
    (const SubRegionPositionFraming & sub_region_framing,
     const ProducableSubGrid & subgrid):
    m_sub_region_framing(sub_region_framing),
    m_subgrid(subgrid) {}

void RegionLoadJob::operator ()
    (MapRegionContainer & container,
     RegionEdgeConnectionsAdder & edge_container_adder,
     LoaderTask::Callbacks & callbacks) const
{
    EntityAndLinkInsertingAdder triangle_entities_adder
        {callbacks, m_subgrid.size2(), m_sub_region_framing.tile_framing()};
    for (auto & producables_view : m_subgrid) {
        for (auto producable : producables_view) {
            (*producable)(triangle_entities_adder);
        }
        triangle_entities_adder.advance_grid_position();
    }
    m_sub_region_framing.set_containers_with
        (triangle_entities_adder.finish_adding_triangles(),
         triangle_entities_adder.finish_adding_entites(),
         container, edge_container_adder);
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

void RegionLoadCollector::collect_load_job
    (const SubRegionPositionFraming & sub_region_framing,
     const ProducableSubGrid & subgrid)
{
    if (auto refresh = sub_region_framing.region_refresh_for(m_container)) {// .region_refresh_at(on_field_position)) {
        refresh->keep_this_frame();
        return;
    }

    m_entries.emplace_back(sub_region_framing, subgrid);
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

SharedPtr<LoaderTask> RegionDecayCollector::finish_into_task_with
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
    for (auto & load_entry : m_load_entries)
        { load_entry(m_container, adder, callbacks); }

    auto remover = adder.finish().make_remover();
    for (auto & decay_entry : m_decay_entries)
        { decay_entry(remover, callbacks); }
    m_edge_container = remover.finish();
}

namespace {

SharedPtr<TriangleLink> to_link(const TriangleSegment & segment)
    { return make_shared<TriangleLink>(segment); }

EntityAndLinkInsertingAdder::EntityAndLinkInsertingAdder
    (LoaderTask::Callbacks & callbacks,
     Size2I grid_size,
     const TilePositionFraming & tile_framing):
    m_callbacks(callbacks),
    m_triangle_inserter(grid_size),
    m_tile_framing(tile_framing) {}

std::vector<Entity> EntityAndLinkInsertingAdder::finish_adding_entites() {
    for (auto & e : m_entities)
        { m_callbacks.add(e); }
    return std::move(m_entities);
}

SharedPtr<ViewGridTriangle> EntityAndLinkInsertingAdder::finish_adding_triangles() {
    return make_shared<ViewGridTriangle>(finish_triangle_grid());
}

SharedPtr<RenderModel> EntityAndLinkInsertingAdder::make_render_model()
    { return m_callbacks.platform().make_render_model(); }

void EntityAndLinkInsertingAdder::add_collidable_
    (const TriangleSegment & triangle_segment)
{ m_triangle_inserter.push(m_tile_framing.transform(triangle_segment)); }

Entity EntityAndLinkInsertingAdder::add_entity_() {
    auto e = m_callbacks.platform().make_renderable_entity();
    // NOTE: region load job adds the entity to the scene
    m_entities.push_back(e);
    return e;
}

/* private */ ViewGridTriangle EntityAndLinkInsertingAdder::
    finish_triangle_grid()
{
    auto triangle_grid = m_triangle_inserter.
        transform_values<SharedPtr<TriangleLink>>(to_link).
        finish();
    for (auto & link : triangle_grid.elements())
        { m_callbacks.add(link); }

    link_triangles(triangle_grid);
    return triangle_grid;
}

void link_triangles(ViewGridTriangle & link_grid) {
    for (Vector2I r; r != link_grid.end_position(); r = link_grid.next(r)) {
    for (auto & this_tri : link_grid(r)) {
        assert(this_tri);
        for (Vector2I v : { r, Vector2I{1, 0} + r, Vector2I{-1,  0} + r,
                               Vector2I{0, 1} + r, Vector2I{ 0, -1} + r})
        {
            if (!link_grid.has_position(v)) continue;
            for (auto & other_tri : link_grid(v)) {
                assert(other_tri);

                if (this_tri == other_tri) continue;
                TriangleLink::attach_unattached_matching_points
                    (this_tri, other_tri);
            }
        }
    }}
}

} // end of <anonymous> namespace
