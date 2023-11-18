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

StackableProducableTileGrid::StackableProducableTileGrid() {}

StackableProducableTileGrid::StackableProducableTileGrid
    (Grid<ProducableTile *> && producables,
     ProducableGroupCollection && producable_owners):
    m_producable_grid(std::move(producables)),
    m_producable_owners(producable_owners) {}

ProducableTileGridStacker StackableProducableTileGrid::stack_with
    (ProducableTileGridStacker && stacker)
{
    stacker.stack_with
        (std::move(m_producable_grid), std::move(m_producable_owners));
    return std::move(stacker);
}

// ----------------------------------------------------------------------------

/* static */ ViewGrid<ProducableTile *>
    ProducableTileGridStacker::producable_grids_to_view_grid
    (std::vector<Grid<ProducableTile *>> && producables_grid)
{
    if (producables_grid.empty())
        { return ViewGrid<ProducableTile *>{}; }

    auto & first = producables_grid.front();
    ViewGridInserter<ProducableTile *> inserter{first.size2()};
    for (; !inserter.filled(); inserter.advance()) {
        for (const auto & grid : producables_grid) {
            if (auto * producable = grid(inserter.position()))
                inserter.push(producable);
        }
    }
    return inserter.finish();
}

void ProducableTileGridStacker::stack_with
    (Grid<ProducableTile *> && producable_grid,
     std::vector<SharedPtr<ProducableGroup_>> && producable_owners)
{
    using std::move_iterator;
    m_producable_grids.emplace_back(std::move(producable_grid));
    m_producable_owners.insert
        (m_producable_owners.end(),
         move_iterator{producable_owners.begin()},
         move_iterator{producable_owners.end  ()});
}

ProducableTileViewGrid ProducableTileGridStacker::to_producables() {
    return ProducableTileViewGrid
        {producable_grids_to_view_grid (std::move(m_producable_grids)),
         std::move(m_producable_owners)                               };
}
