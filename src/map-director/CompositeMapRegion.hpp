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

class SubRegionGridStacker;

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

    bool belongs_to_parent() const
        { return static_cast<bool>(m_parent_region); }

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
        (Tuple<MapSubRegionViewGrid, MapSubRegionOwners> &&,
         ScaleComputation &&);

    void process_load_request
        (const RegionLoadRequestBase & request,
         const RegionPositionFraming & framing,
         RegionLoadCollectorBase & collector,
         const Optional<RectangleI> & grid_scope = {}) final;

    Size2I size2() const final { return m_sub_regions.size2(); }

private:
    using MapSubRegionGrid = Grid<MapSubRegion>;
    using MapSubRegionSubGrid = MapSubRegionViewGrid::SubGrid;

    void collect_load_tasks
        (const RegionLoadRequestBase & request,
         const RegionPositionFraming & framing,
         const MapSubRegionSubGrid & subgrid,
         RegionLoadCollectorBase & collector);

    MapSubRegionViewGrid m_sub_regions;
    MapSubRegionOwners m_sub_region_owners;
    ScaleComputation m_scale;
};

// ----------------------------------------------------------------------------

class StackableSubRegionGrid final {
public:
    using MapSubRegionViewGrid = CompositeMapRegion::MapSubRegionViewGrid;
    using MapSubRegionOwners = CompositeMapRegion::MapSubRegionOwners;

    StackableSubRegionGrid() {}

    StackableSubRegionGrid
        (Grid<const MapSubRegion *> && subregions,
         const SharedPtr<Grid<MapSubRegion>> & owner);

    SubRegionGridStacker stack_with(SubRegionGridStacker &&);

private:
    StackableSubRegionGrid
        (std::vector<Grid<const MapSubRegion *>> && subregions,
         std::vector<SharedPtr<Grid<MapSubRegion>>> && owner);

    Grid<const MapSubRegion *> m_subregion;
    SharedPtr<Grid<MapSubRegion>> m_owner;
};

// ----------------------------------------------------------------------------

class SubRegionGridStacker final {
public:
    using MapSubRegionViewGrid = CompositeMapRegion::MapSubRegionViewGrid;
    using MapSubRegionOwners = CompositeMapRegion::MapSubRegionOwners;
    using MapSubRegionOwnerPtr = SharedPtr<Grid<MapSubRegion>>;
    using MapSubRegionOwnersMap =
        rigtorp::HashMap<MapSubRegionOwnerPtr, std::monostate>;

    static MapSubRegionViewGrid make_view_grid
        (std::vector<Grid<const MapSubRegion *>> &&);

    static MapSubRegionOwners make_owners_container
        (MapSubRegionOwnersMap &&);

    void stack_with
        (Grid<const MapSubRegion *> && subregion,
         SharedPtr<Grid<MapSubRegion>> && owner);

    Tuple<MapSubRegionViewGrid, MapSubRegionOwners>
        to_sub_region_view_grid();

    bool is_empty() const { return m_subregions.empty(); }

private:
    static const MapSubRegionOwnersMap k_default_owners_map;

    std::vector<Grid<const MapSubRegion *>> m_subregions;
    MapSubRegionOwnersMap m_owners = k_default_owners_map;
};