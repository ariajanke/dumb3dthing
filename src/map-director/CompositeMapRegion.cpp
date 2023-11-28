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

#include "CompositeMapRegion.hpp"

namespace {

using MapSubRegionViewGrid = SubRegionGridStacker::MapSubRegionViewGrid;
using MapSubRegionOwnersMap = SubRegionGridStacker::MapSubRegionOwnersMap;

} // end of <anonymous> namespace

MapSubRegion::MapSubRegion
    (const RectangleI & sub_region_bounds,
     const SharedPtr<MapRegion> & parent_region):
    m_sub_region_bounds(sub_region_bounds),
    m_parent_region(parent_region) {}

void MapSubRegion::process_load_request
    (const RegionLoadRequestBase & request,
     const RegionPositionFraming & framing,
     RegionLoadCollectorBase & collector) const
{
    m_parent_region->process_load_request
        (request, framing, collector, m_sub_region_bounds);
}

// ----------------------------------------------------------------------------

CompositeMapRegion::CompositeMapRegion
    (Tuple<MapSubRegionViewGrid, MapSubRegionOwnersMap> && tup,
     ScaleComputation && scale):
    m_sub_regions(std::move(std::get<MapSubRegionViewGrid>(tup))),
    m_sub_region_owners(std::move(std::get<MapSubRegionOwnersMap>(tup))),
    m_scale(scale) {}

void CompositeMapRegion::process_load_request
    (const RegionLoadRequestBase & request,
     const RegionPositionFraming & framing,
     RegionLoadCollectorBase & collector,
     const Optional<RectangleI> & grid_scope)
{
    MapSubRegionSubGrid subgrid = [this, &grid_scope] () -> MapSubRegionSubGrid {
        return grid_scope ?
            m_sub_regions.make_subgrid(*grid_scope) :
            m_sub_regions.make_subgrid();
    } ();
    collect_load_tasks(request, framing, subgrid, collector);
}

/* private */ void CompositeMapRegion::collect_load_tasks
    (const RegionLoadRequestBase & request,
     const RegionPositionFraming & framing,
     const MapSubRegionSubGrid & subgrid,
     RegionLoadCollectorBase & collector)
{
    auto on_overlap =
        [&collector, &subgrid, &request]
        (const RegionPositionFraming & sub_frame,
         const RectangleI & sub_region_bound_in_tiles)
    {
        const auto & bounds = sub_region_bound_in_tiles;
        auto subsubgrid = subgrid.make_sub_grid
            (top_left_of(bounds), bounds.width, bounds.height);
        for (Vector2I r; r != subsubgrid.end_position(); r = subsubgrid.next(r)) {
            for (auto & subregion : subsubgrid(r)) {
                subregion->
                    process_load_request(request, sub_frame.move(r), collector);
            }
        }
    };
    framing.
        with_scaling(m_scale).
        for_each_overlap(subgrid.size2(), request, std::move(on_overlap));
}

// ----------------------------------------------------------------------------

StackableSubRegionGrid::StackableSubRegionGrid
    (Grid<const MapSubRegion *> && subregions,
     const SharedPtr<Grid<MapSubRegion>> & owner):
    m_subregion(std::move(subregions)),
    m_owner(owner) {}

SubRegionGridStacker StackableSubRegionGrid::stack_with
    (SubRegionGridStacker && stacker)
{
    stacker.stack_with(std::move(m_subregion), std::move(m_owner));
    return std::move(stacker);
}

// ----------------------------------------------------------------------------

/* static */ MapSubRegionViewGrid SubRegionGridStacker::make_view_grid
    (std::vector<Grid<const MapSubRegion *>> && subregions)
{
    if (subregions.empty())
        { return MapSubRegionViewGrid{}; }

    auto & first = subregions.front();
    ViewGridInserter<const MapSubRegion *> grid_inserter{first.size2()};
    while (!grid_inserter.filled()) {
        for (auto & subregion_grid : subregions) {
            const auto * subregion = subregion_grid(grid_inserter.position());
            if (!subregion) continue;
            if (!subregion->belongs_to_parent()) continue;
            grid_inserter.push(subregion);
        }
        grid_inserter.advance();
    }
    return grid_inserter.finish();
}

void SubRegionGridStacker::stack_with
    (Grid<const MapSubRegion *> && subregion,
     SharedPtr<Grid<MapSubRegion>> && owner)
{
    m_subregions.emplace_back(std::move(subregion));
    m_owners.emplace(std::move(owner), std::monostate{});
}

Tuple<MapSubRegionViewGrid, MapSubRegionOwnersMap>
    SubRegionGridStacker::to_sub_region_view_grid()
{
    return make_tuple
        (make_view_grid(std::move(m_subregions)), std::move(m_owners));
}
