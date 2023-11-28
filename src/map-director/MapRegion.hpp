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

class RegionLoadRequestBase;
class ScaleComputation;
class ProducableTileGridStacker;

class RegionLoadCollectorBase {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    virtual ~RegionLoadCollectorBase() {}

    virtual void collect_load_job
        (const SubRegionPositionFraming &, const ProducableSubGrid &) = 0;
};

// ----------------------------------------------------------------------------

class MapRegion {
public:
    virtual ~MapRegion() {}

    virtual void process_load_request
        (const RegionLoadRequestBase &,
         const RegionPositionFraming &,
         RegionLoadCollectorBase &,
         const Optional<RectangleI> & grid_scope = {}) = 0;

    virtual Size2I size2() const = 0;
};

// ----------------------------------------------------------------------------

class TiledMapRegion final : public MapRegion {
public:
    TiledMapRegion(ProducableTileViewGrid &&, ScaleComputation &&);

    void process_load_request
        (const RegionLoadRequestBase &,
         const RegionPositionFraming &,
         RegionLoadCollectorBase &,
         const Optional<RectangleI> & = {}) final;

    Size2I size2() const final { return m_producables_view_grid.size2(); }

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

// ----------------------------------------------------------------------------

class StackableProducableTileGrid final {
public:
    using ProducableGroupCollection = std::vector<SharedPtr<ProducableGroup_>>;

    StackableProducableTileGrid();

    StackableProducableTileGrid
        (Grid<ProducableTile *> && producables,
         ProducableGroupCollection && producable_owners);

    ProducableTileGridStacker stack_with(ProducableTileGridStacker &&);

private:
    Grid<ProducableTile *> m_producable_grid;
    ProducableGroupCollection m_producable_owners;
};

// ----------------------------------------------------------------------------

class ProducableTileGridStacker final {
public:
    static ViewGrid<ProducableTile *> producable_grids_to_view_grid
        (std::vector<Grid<ProducableTile *>> &&);

    void stack_with
        (Grid<ProducableTile *> && producable_grid,
         std::vector<SharedPtr<ProducableGroup_>> && producable_owners);

    ProducableTileViewGrid to_producables();

private:
    std::vector<Grid<ProducableTile *>> m_producable_grids;
    std::vector<SharedPtr<ProducableGroup_>> m_producable_owners;
};
