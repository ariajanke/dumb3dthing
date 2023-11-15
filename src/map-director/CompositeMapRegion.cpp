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

using MapSubRegionViewGrid = StackableSubRegionGrid::MapSubRegionViewGrid;
using MapSubRegionOwners = StackableSubRegionGrid::MapSubRegionOwners;

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
    (Tuple<MapSubRegionViewGrid, MapSubRegionOwners> && tup,
     ScaleComputation && scale):
    m_sub_regions(std::move(std::get<MapSubRegionViewGrid>(tup))),
    m_sub_region_owners(std::move(std::get<MapSubRegionOwners>(tup))),
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
        for_each_overlap(m_sub_regions.size2(), request, std::move(on_overlap));
}

// ----------------------------------------------------------------------------

StackableSubRegionGrid::StackableSubRegionGrid
    (Grid<const MapSubRegion *> && subregions,
     const SharedPtr<Grid<MapSubRegion>> & owner):
    m_subregion(std::move(subregions)),
    m_owner(owner) {}

StackableSubRegionGrid StackableSubRegionGrid::stack_with
    (StackableSubRegionGrid && stackable_sub_region_grid)
{
    // guh... well if this isn't evidence of a problem...
    using std::move_iterator;
    auto osubregions = std::move(stackable_sub_region_grid.m_subregions);
    auto oowners = std::move(stackable_sub_region_grid.m_owners);
    m_subregions.reserve(m_subregions.size() + osubregions.size() + 2);
    m_owners.reserve(m_owners.size() + oowners.size() + 2);
    m_subregions.insert
        (m_subregions.end(),
         move_iterator{osubregions.begin()},
         move_iterator{osubregions.end()});
    m_owners.insert
        (m_owners.end(),
         move_iterator{oowners.begin()},
         move_iterator{oowners.end()});
    m_subregions.emplace_back(std::move(m_subregion));
    m_subregions.emplace_back(std::move(stackable_sub_region_grid.m_subregion));
    m_owners.emplace_back(std::move(m_owner));
    m_owners.emplace_back(std::move(stackable_sub_region_grid.m_owner));
    return StackableSubRegionGrid
        {std::move(m_subregions), std::move(m_owners)};
}

Tuple<MapSubRegionViewGrid, MapSubRegionOwners>
    StackableSubRegionGrid::to_sub_region_view_grid()
{
    auto & first = m_subregions.front();
    ViewGridInserter<const MapSubRegion *> grid_inserter{first.size2()};
    while (!grid_inserter.filled()) {
        for (auto & subregion_grid : m_subregions) {
            // bug: plopping empty grids into your stack
            if (!subregion_grid.has_position(grid_inserter.position()))
                continue;
            const auto * subregion = subregion_grid(grid_inserter.position());
            if (!subregion) continue;
            if (!subregion->belongs_to_parent()) continue;
            grid_inserter.push(subregion);
        }
        grid_inserter.advance();
    }
#   if 0
    for (Vector2I r; r != first.end_position(); r = first.next(r)) {
        for (auto & subregion_grid : m_subregions) {
            for (const MapSubRegion * subregion : subregion_grid) {
                if (!subregion) continue;
                if (!subregion->belongs_to_parent()) continue;
                grid_inserter.push(subregion);
            }
        }
        grid_inserter.advance();
    }
#   endif
    m_subregions.clear();
    return make_tuple(grid_inserter.finish(), std::move(m_owners));
}

/* private */ StackableSubRegionGrid::StackableSubRegionGrid
    (std::vector<Grid<const MapSubRegion *>> && subregions,
     std::vector<SharedPtr<Grid<MapSubRegion>>> && owners):
    m_subregions(std::move(subregions)),
    m_owners(std::move(owners)) {}
