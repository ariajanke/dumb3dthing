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

namespace {

using Size2I = TiledMapRegion::Size2I;
using namespace cul::exceptions_abbr;

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
    if (region_left == region_right || region_top == region_bottom) {
        return;
    }

    auto factory_subgrid = m_factory_grid.make_subgrid(Rectangle
        {region_left, region_top,
         region_right - region_left, region_bottom - region_top});

    region_preparer->set_tile_producable_subgrid
        (Vector2I{region_left, region_top}, std::move(factory_subgrid));
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
    EntityAndLinkInsertingAdder triangle_entities_adder{m_tile_factory_grid.size2()};
    for (Vector2I r; r != m_tile_factory_grid.end_position();
         r = m_tile_factory_grid.next(r)                     )
    {
        for (auto producable : m_tile_factory_grid(r)) {
            if (!producable) continue;
            (*producable)(m_tile_offset - m_subgrid_offset, triangle_entities_adder, callbacks.platform());
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

void MapRegionPreparer::set_tile_producable_subgrid
    (const Vector2I & subgrid_tl, ProducableTileViewSubGrid && tile_factory_grid)
{
    m_subgrid_offset = subgrid_tl;
    m_tile_factory_grid = std::move(tile_factory_grid);
}

void MapRegionPreparer::set_completer(const MapRegionCompleter & completer)
    { m_completer = completer; }
