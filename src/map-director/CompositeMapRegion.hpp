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

#pragma once

#include "MapRegion.hpp"

class MapSubRegion final {
public:
    MapSubRegion() {}

    MapSubRegion
        (const RectangleI & sub_region_bounds,
         const SharedPtr<MapRegion> & parent_region);

    void process_load_request
        (const RegionLoadRequestBase & request,
         const RegionPositionFraming & framing,
         RegionLoadCollectorBase & collector) const;

private:
    RectangleI m_sub_region_bounds;
    SharedPtr<MapRegion> m_parent_region;
};

// ----------------------------------------------------------------------------

class CompositeMapRegion final : public MapRegion {
public:
    using MapSubRegionViewGrid = ViewGrid<const MapSubRegion *>;
    using MapSubRegionOwners = std::vector<SharedPtr<Grid<MapSubRegion>>>;

    CompositeMapRegion() {}

    CompositeMapRegion
        (Grid<MapSubRegion> && sub_regions_grid,
         const ScaleComputation & scale);

    void process_load_request
        (const RegionLoadRequestBase & request,
         const RegionPositionFraming & framing,
         RegionLoadCollectorBase & collector,
         const Optional<RectangleI> & grid_scope = {}) final;

    Size2I size2() const final { return m_sub_regions.size2(); }

private:
    using MapSubRegionGrid = Grid<MapSubRegion>;
    using MapSubRegionSubGrid = cul::ConstSubGrid<MapSubRegionGrid::Element>;

    void collect_load_tasks
        (const RegionLoadRequestBase & request,
         const RegionPositionFraming & framing,
         const MapSubRegionSubGrid & subgrid,
         RegionLoadCollectorBase & collector);

    Grid<MapSubRegion> m_sub_regions;
    ScaleComputation m_scale;
};

// ----------------------------------------------------------------------------

class StackableSubRegionGrid final {
public:
    using MapSubRegionViewGrid = CompositeMapRegion::MapSubRegionViewGrid;
    using MapSubRegionOwners = CompositeMapRegion::MapSubRegionOwners;

    StackableSubRegionGrid();

    StackableSubRegionGrid
        (Grid<const MapSubRegion *> && subregions,
         const SharedPtr<Grid<MapSubRegion>> & owner);

    StackableSubRegionGrid stack_with(StackableSubRegionGrid &&);

    CompositeMapRegion to_composite_map_region();

private:
    std::vector<Grid<const MapSubRegion *>> m_subregions;
    std::vector<SharedPtr<Grid<MapSubRegion>>> m_owners;
};
