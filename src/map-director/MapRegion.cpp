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

namespace {

using ProducableFillerMap = StackableProducableTileGrid::ProducableFillerMap;

} // end of <anonymous> namespace

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

/* static */ const ProducableFillerMap
    StackableProducableTileGrid::k_default_producable_filler_map =
    ProducableFillerMap{2, SharedPtr<const ProducableGroupFiller>{}};

/* static */
    Optional<ViewGrid<ProducableTile *>>
    StackableProducableTileGrid::producable_grids_to_view_grid
    (std::vector<Grid<ProducableTile *>> && producables)
{
    if (producables.empty())
        { return {}; }

    ViewGridInserter<ProducableTile *> inserter{producables.begin()->size2()};
    for (; !inserter.filled(); inserter.advance()) {
        for (const auto & grid : producables) {
            if (auto * producable = grid(inserter.position()))
                inserter.push(producable);
        }
    }
    return inserter.finish();
}

/* static */ StackableProducableTileGrid
    StackableProducableTileGrid::make_with_fillers
    (const std::vector<SharedPtr<const ProducableGroupFiller>> & fillers,
     Grid<ProducableTile *> && producables,
     ProducableGroupCollection && producable_owners)
{
    auto filler_map = k_default_producable_filler_map;
    for (auto & filler : fillers) {
        (void)filler_map.emplace(filler, std::monostate{});
    }
    return StackableProducableTileGrid
        {std::vector<Grid<ProducableTile *>>{std::move(producables)},
         std::move(filler_map),
         std::move(producable_owners)};
}

/* static */ std::vector<SharedPtr<const ProducableGroupFiller>>
    StackableProducableTileGrid::filler_map_to_vector
    (ProducableFillerMap && filler_map)
{
    std::vector<SharedPtr<const ProducableGroupFiller>>
        filler_vector;
    filler_vector.reserve(filler_map.size());
    for (auto & [filler, _] : filler_map) {
        filler_vector.push_back(filler);
    }
    return filler_vector;
}

StackableProducableTileGrid::StackableProducableTileGrid():
    m_fillers(k_default_producable_filler_map) {}

StackableProducableTileGrid::StackableProducableTileGrid
    (Grid<ProducableTile *> && producables,
     ProducableFillerMap && fillers,
     ProducableGroupCollection && producable_owners):
    m_producable_grids({ std::move(producables) }),
    m_fillers(std::move(fillers)),
    m_producable_owners(producable_owners) {}

StackableProducableTileGrid StackableProducableTileGrid::stack_with
    (StackableProducableTileGrid && stackable_grid)
{
    return stackable_grid.stack_with
        (std::move(m_producable_grids),
         std::move(m_fillers),
         std::move(m_producable_owners));
}

StackableProducableTileGrid StackableProducableTileGrid::stack_with
    (std::vector<Grid<ProducableTile *>> && producable_grids,
     ProducableFillerMap && fillers,
     ProducableGroupCollection && producable_owners)
{
    m_producable_grids.insert
        (m_producable_grids.end(),
         std::move_iterator{producable_grids.begin()},
         std::move_iterator{producable_grids.end()});
    m_fillers.reserve(fillers.size() + m_fillers.size());
    for (auto & [filler, _] : fillers)
        { m_fillers.emplace(filler, std::monostate{}); }
    m_producable_owners.insert
        (m_producable_owners.end(),
         std::move_iterator{producable_owners.begin()},
         std::move_iterator{producable_owners.end()});
    return StackableProducableTileGrid
        {std::move(m_producable_grids), std::move(m_fillers),
         std::move(m_producable_owners)};
}

ProducableTileViewGrid StackableProducableTileGrid::to_producables() {
    auto producables = producable_grids_to_view_grid
        (std::move(m_producable_grids));

    if (!producables)
        return ProducableTileViewGrid{};

    return ProducableTileViewGrid
        {std::move(*producables),
         std::move(m_producable_owners),
         filler_map_to_vector(std::move(m_fillers))};
}

/* private */ StackableProducableTileGrid::StackableProducableTileGrid
    (std::vector<Grid<ProducableTile *>> && producable_grids,
     ProducableFillerMap && fillers,
     ProducableGroupCollection && producable_owners):
    m_producable_grids(std::move(producable_grids)),
    m_fillers(std::move(fillers)),
    m_producable_owners(producable_owners) {}
