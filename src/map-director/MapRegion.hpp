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

#include "RegionPositionFraming.hpp"

#include <unordered_map>

class MapRegionPreparer;
class RegionLoadRequestBase;
class MapRegionContainer;
class ScaleComputation;
class RegionEdgeConnectionsAdder;

class RegionLoadCollectorBase {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    virtual ~RegionLoadCollectorBase() {}

    virtual void collect_load_job
        (const SubRegionPositionFraming &, const ProducableSubGrid &) = 0;
};

class MapRegion {
public:
    virtual ~MapRegion() {}

    virtual void process_load_request
        (const RegionLoadRequestBase &,
         const RegionPositionFraming &,
         RegionLoadCollectorBase &,
         const Optional<RectangleI> & grid_scope = {}) = 0;
};

class TiledMapRegion final : public MapRegion {
public:
    TiledMapRegion(ProducableTileViewGrid &&, ScaleComputation &&);

    void process_load_request
        (const RegionLoadRequestBase &,
         const RegionPositionFraming &,
         RegionLoadCollectorBase &,
         const Optional<RectangleI> & = {}) final;

private:
    void process_load_request_
        (ProducableTileViewGrid::SubGrid producables,
         const RegionLoadRequestBase &,
         const RegionPositionFraming &,
         RegionLoadCollectorBase &);

    Size2I region_size() const { return m_producables_view_grid.size2(); }

    // there's something that lives in here, but what?
    // something that breaks down into loadable "sub regions"
    ProducableTileViewGrid m_producables_view_grid;
    ScaleComputation m_scale;
};

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

class CompositeMapRegion final : public MapRegion {
public:
    CompositeMapRegion() {}

    CompositeMapRegion
        (Grid<MapSubRegion> && sub_regions_grid,
         const ScaleComputation & scale);

    void process_load_request
        (const RegionLoadRequestBase & request,
         const RegionPositionFraming & framing,
         RegionLoadCollectorBase & collector,
         const Optional<RectangleI> & grid_scope = {}) final;

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
