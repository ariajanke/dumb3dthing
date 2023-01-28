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

#include "map-loader-helpers.hpp"
#include "ProducableGrid.hpp"

class MapRegionPreparer;

/// a map region is a grid of task pairs, one to load, one to teardown
class MapRegion {
public:
    virtual ~MapRegion() {}

    virtual void request_region_load
        (const Vector2I & local_position,
         const SharedPtr<MapRegionPreparer> & region_preparer, TaskCallbacks &) = 0;
};

/// a map region for tiled maps
class TiledMapRegion final : public MapRegion {
public:
    TiledMapRegion
        (ProducableTileViewGrid && full_factory_grid,
         const Size2I & region_size_in_tiles):
         m_region_size(region_size_in_tiles),
         m_factory_grid(std::move(full_factory_grid)) {}

    void request_region_load
        (const Vector2I & local_region_position,
         const SharedPtr<MapRegionPreparer> & region_preparer,
         TaskCallbacks & callbacks) final;

private:
    Size2I map_size_in_regions() const;

    Size2I m_region_size;
    ProducableTileViewGrid m_factory_grid;
};

class GridMapRegionCompleter {
public:
    virtual ~GridMapRegionCompleter() {}

    virtual void on_complete
        (const Vector2I & region_position,
         InterTriangleLinkContainer && link_container,
         SharedPtr<TeardownTask> && teardown_task) = 0;
};

class MapRegionCompleter final {
public:
    MapRegionCompleter() {}

    MapRegionCompleter
        (const Vector2I & region_position,
         GridMapRegionCompleter & grid_completer);

    template <typename ... Types>
    void on_complete(Types && ... args) const
        { verify_completer().on_complete(m_pos, std::forward<Types>(args)...); }

private:
    GridMapRegionCompleter & verify_completer() const;

    Vector2I m_pos;
    GridMapRegionCompleter * m_completer = nullptr;
};

/// a loader task that prepares a region of the map
class MapRegionPreparer final : public LoaderTask {
public:
    explicit MapRegionPreparer(const Vector2I & tile_offset);

    void operator () (LoaderTask::Callbacks & callbacks) const final;

    void assign_tile_producable_grid
        (const RectangleI & region_range,
         const ProducableTileViewGrid & tile_factory_grid);

    void set_completer(const MapRegionCompleter &);

private:
    const ProducableTileViewGrid * m_tile_producable_grid = nullptr;
    RectangleI m_producable_grid_range;

    MapRegionCompleter m_completer;
    Vector2I m_tile_offset;
};