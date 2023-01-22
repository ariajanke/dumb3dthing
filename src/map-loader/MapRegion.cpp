/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include <iostream>

namespace {

using namespace cul::exceptions_abbr;

class EntityAndLinkInsertingAdder final : public EntityAndTrianglesAdder {
public:
    using GridViewTriangleInserter = ViewGridInserter<SharedPtr<TriangleLink>>;

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

    Tuple<GridViewTriangleInserter::ElementContainer,
          Grid<GridViewTriangleInserter::ElementView>>
        move_out_container_and_grid_view()
    {
        return m_triangle_inserter.
            transform_values<SharedPtr<TriangleLink>>(to_link).
            move_out_container_and_grid_view();
    }
private:
    static SharedPtr<TriangleLink> to_link(const TriangleSegment & segment)
        { return make_shared<TriangleLink>(segment); }

    ViewGridInserter<TriangleSegment> m_triangle_inserter;
    std::vector<Entity> m_entities;
};

} // end of <anonymous> namespace

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

    auto [link_container, link_view_grid] =
        triangle_entities_adder.move_out_container_and_grid_view();
    link_triangles(link_view_grid);
    // ^ vectorize

    auto ents = triangle_entities_adder.move_out_entities();
    for (auto & link : link_container) {
        callbacks.add(link);
    }
    for (auto & ent : ents) {
        callbacks.add(ent);
    }

    m_completer.on_complete
        (InterTriangleLinkContainer{link_view_grid},
         make_shared<TeardownTask>(std::move(ents), std::move(link_container)));
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
