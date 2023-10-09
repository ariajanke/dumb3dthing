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
#include "MapRegionTracker.hpp"

TiledMapRegion::TiledMapRegion
    (ProducableTileViewGrid && producables_view_grid,
     ScaleComputation && scale_computation):
    m_producables_view_grid(std::move(producables_view_grid)),
    m_scale(std::move(scale_computation)) {}

void TiledMapRegion::process_load_request
    (const RegionLoadRequestBase & request,
     const RegionPositionFraming & framing,
     RegionLoadCollectorBase & collector,
     const Optional<RectangleI> & grid_scope)
{
    auto producables = grid_scope ?
        m_producables_view_grid.make_subgrid(*grid_scope) :
        m_producables_view_grid.make_subgrid();
    process_load_request_(producables, request, framing, collector);
}

/* private */ void TiledMapRegion::process_load_request_
    (ProducableTileViewGrid::SubGrid producables,
     const RegionLoadRequestBase & request,
     const RegionPositionFraming & region_position_framing,
     RegionLoadCollectorBase & collector)
{
    auto collect_load_jobs =
        [&producables, &collector]
        (const RegionPositionFraming & sub_frame,
         const RectangleI & sub_region_bound_in_tiles)
    {
        const auto & bounds = sub_region_bound_in_tiles;
        collector.collect_load_job
            (sub_frame.as_sub_region_framing(),
             producables.make_sub_grid(cul::top_left_of(bounds), bounds.width, bounds.height));
    };
    region_position_framing.
        with_scaling(m_scale).
        for_each_overlap(producables.size2(),
                         request,
                         std::move(collect_load_jobs));
}

// ----------------------------------------------------------------------------

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
    (Grid<MapSubRegion> && sub_regions_grid,
     const ScaleComputation & scale):
    m_sub_regions(std::move(sub_regions_grid)),
    m_scale(scale) {}

void CompositeMapRegion::process_load_request
    (const RegionLoadRequestBase & request,
     const RegionPositionFraming & framing,
     RegionLoadCollectorBase & collector,
     const Optional<RectangleI> & grid_scope)
{
    MapSubRegionSubGrid subgrid = [this, &grid_scope] () -> MapSubRegionSubGrid {
        return grid_scope ?
            MapSubRegionSubGrid{m_sub_regions, cul::top_left_of(*grid_scope), grid_scope->width, grid_scope->height} :
            MapSubRegionSubGrid{m_sub_regions};
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
            subsubgrid(r).
                process_load_request(request, sub_frame.move(r), collector);
        }
    };
    framing.
        with_scaling(m_scale).
        for_each_overlap(m_sub_regions.size2(), request, std::move(on_overlap));
}
