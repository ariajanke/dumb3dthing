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

#include "ProducableGrid.hpp"

#include <unordered_map>

class MapRegionPreparer;
class RegionLoadRequest;
class MapRegionContainer;
class ScaleComputation;

class RegionLoadCollectorBase {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    virtual ~RegionLoadCollectorBase() {}

    virtual void collect_load_job
        (const Vector2I & on_field_position,
         const ProducableSubGrid &,
         const TriangleSegmentTransformation &) = 0;
};

class MapRegion {
public:
    virtual ~MapRegion() {}

    virtual void process_load_request
        (const RegionLoadRequest &, const Vector2I & spawn_offset,
         RegionLoadCollectorBase &) = 0;

    // can I send a request to only *part* of the map?
    virtual void process_limited_load_request
        (const RegionLoadRequest &,
         const Vector2I & spawn_offset,
         const RectangleI & grid_scope,
         RegionLoadCollectorBase &) = 0;
};

class TiledMapRegion final : public MapRegion {
public:
    TiledMapRegion(ProducableTileViewGrid &&, ScaleComputation &&);

    void process_load_request
        (const RegionLoadRequest &,
         const Vector2I & spawn_offset,
         RegionLoadCollectorBase &) final;

    void process_limited_load_request
        (const RegionLoadRequest &,
         const Vector2I & spawn_offset,
         const RectangleI & grid_scope,
         RegionLoadCollectorBase &) final;

private:
    void collect_producables
        (const Vector2I & position_in_region,
         const Vector2I & offset,
         const Size2I & subgrid_size,
         RegionLoadCollectorBase &);

    void process_load_request_
        (ProducableTileViewGrid::SubGrid producables,
         const RegionLoadRequest &,
         const Vector2I & spawn_offset,
         RegionLoadCollectorBase &);

    Size2I region_size() const { return m_producables_view_grid.size2(); }

    // there's something that lives in here, but what?
    // something that breaks down into loadable "sub regions"
    ProducableTileViewGrid m_producables_view_grid;
    ScaleComputation m_scale;
};

// CompositeMapTileSet
// - each tile represents a region in a tiled map


class CompositeMapRegion final : public MapRegion {
public:

    void process_load_request
        (const RegionLoadRequest &,
         const Vector2I & spawn_offset,
         RegionLoadCollectorBase &) final;

    void process_limited_load_request
        (const RegionLoadRequest &,
         const Vector2I & spawn_offset,
         const RectangleI & grid_scope,
         RegionLoadCollectorBase &) final;
private:

    void collect_load_tasks
        (const Vector2I & position_in_region,
         const Vector2I & offset,
         const Size2I & subgrid_size,
         RegionLoadCollectorBase & collector);

    // not just MapRegions, but how to load them...
    // is it a view grid? not really
    // want to support 3D? nah, not for this "ticket"
    //
    // A tile is a chunk of map from the TiledMapRegion
    // Yet, only tilesets may speak of tiles in that fashion


};
