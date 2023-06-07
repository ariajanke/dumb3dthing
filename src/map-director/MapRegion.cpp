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

#include "MapRegion.hpp"
#include "TileFactory.hpp"
#include "MapRegionTracker.hpp"
#include "MapDirector.hpp"
#include "RegionLoadRequest.hpp"

#include <iostream>

namespace {

using namespace cul::exceptions_abbr;

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


Vector2I region_load_step(const ProducableTileViewGrid & view_grid,
                          const RegionLoadRequest & request)
{
    auto step_of_ = [] (int length, int max) {
        // I want as even splits as possible
        if (length < max) return length;
        return length / (length / max);
    };
    return Vector2I
        {step_of_(view_grid.width (), request.max_region_size().width ),
         step_of_(view_grid.height(), request.max_region_size().height)};
}

TiledMapRegionN::TiledMapRegionN(ProducableTileViewGrid && full_factory_grid):
    m_producables_view_grid(std::move(full_factory_grid)) {}

void TiledMapRegionN::process_load_request
    (const RegionLoadRequest & request, const Vector2I & offset,
     RegionLoadCollectorN & collector)
{
    // reminder: tiles are laid out eastward (not westward)
    //           it's assumed that bottom-top interpretation of a tiled map is
    //           not comfortable (and therefore top-down)
    auto step = region_load_step(m_producables_view_grid, request);
    auto subgrid_size = cul::convert_to<Size2I>(step);
    for (Vector2I r; r.x < m_producables_view_grid.width (); r.x += step.x) {
    for (          ; r.y < m_producables_view_grid.height(); r.y += step.y) {
        auto on_field_position = offset + r;
        bool overlaps_this_subregion =
            request.overlaps_with(RectangleI{on_field_position, subgrid_size});
        if (!overlaps_this_subregion) continue;
        auto subgrid = m_producables_view_grid.make_subgrid(RectangleI{r, subgrid_size});
        collector.add_tiles(on_field_position, offset, subgrid);
    }}
}

// ----------------------------------------------------------------------------

void TiledMapRegion::request_region_load
    (const Vector2I & local_region_position,
     const SharedPtr<MapRegionPreparer> & region_preparer,
     TaskCallbacks & callbacks)
{
    Vector2I lrp{local_region_position.x % map_size_in_regions().width,
                 local_region_position.y % map_size_in_regions().height};
    auto region_left   = std::max
        (lrp.x*m_region_size.width, 0);
    auto region_top    = std::max
        (lrp.y*m_region_size.height, 0);
    auto region_right  = std::min
        (region_left + m_region_size.width, m_factory_grid.width());
    auto region_bottom = std::min
        (region_top + m_region_size.height, m_factory_grid.height());
    if (region_left == region_right || region_top == region_bottom)
        { return; }

    region_preparer->assign_tile_producable_grid
        (RectangleI
            {region_left, region_top,
             region_right - region_left, region_bottom - region_top},
         m_factory_grid                                             );

    callbacks.add(region_preparer);
}

/* private */ Size2I TiledMapRegion::map_size_in_regions() const {
    auto dim_to_size = [] (int grid_len_in_tiles, int region_len) {
        return   grid_len_in_tiles / region_len
               + (grid_len_in_tiles % region_len ? 1 : 0);
    };
    return Size2I{dim_to_size(m_factory_grid.width (), m_region_size.width ),
                  dim_to_size(m_factory_grid.height(), m_region_size.height)};
}

// ----------------------------------------------------------------------------

MapRegionCompleter::MapRegionCompleter
    (const Vector2I & region_position,
     GridMapRegionCompleter & grid_completer):
    m_pos(region_position),
    m_completer(&grid_completer)
{}

/* private */ GridMapRegionCompleter & MapRegionCompleter::
    verify_completer() const
{
    if (m_completer) return *m_completer;
    throw RtError{"Grid wide completer must be set to on_complete"};
}

// ----------------------------------------------------------------------------

void MapRegionPreparer::operator () (LoaderTask::Callbacks & callbacks) const {
    if (!m_tile_producable_grid) {
        throw RtError{"Grid not set"};
    }
    EntityAndLinkInsertingAdder triangle_entities_adder
        {cul::size_of(m_producable_grid_range)};
    auto producables_subgrid = m_tile_producable_grid->make_subgrid
        (m_producable_grid_range);
    auto spawn_offset = m_tile_offset - cul::top_left_of(m_producable_grid_range);
    for (auto & producables_view : producables_subgrid) {
        for (auto producable : producables_view) {
            if (!producable) continue;
            (*producable)(spawn_offset, triangle_entities_adder, callbacks.platform());
        }
        triangle_entities_adder.advance_grid_position();
    }

    auto link_view_grid = triangle_entities_adder.finish_triangle_grid();
    link_triangles(link_view_grid);
    // ^ vectorize

    auto ents = triangle_entities_adder.move_out_entities();
    for (auto & link : link_view_grid.elements()) {
        callbacks.add(link);
    }
    for (auto & ent : ents) {
        callbacks.add(ent);
    }

    m_completer.on_complete
        (InterTriangleLinkContainer{link_view_grid},
         make_shared<TeardownTask>(std::move(ents), link_view_grid.elements()));
}

MapRegionPreparer::MapRegionPreparer
    (const Vector2I & tile_offset):
    m_tile_offset(tile_offset) {}

void MapRegionPreparer::assign_tile_producable_grid
    (const RectangleI & region_range,
     const ProducableTileViewGrid & tile_factory_grid)
{
    m_producable_grid_range = region_range;
    m_tile_producable_grid = &tile_factory_grid;
}

void MapRegionPreparer::set_completer(const MapRegionCompleter & completer)
    { m_completer = completer; }

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
