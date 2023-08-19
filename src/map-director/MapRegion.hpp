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

class RegionLoadCollectorBase {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    virtual ~RegionLoadCollectorBase() {}

    virtual void add_tiles
        (const Vector2I & on_field_position,
         const Vector2I & maps_offset, const ProducableSubGrid &) = 0;
};

class MapRegion {
public:
    virtual ~MapRegion() {}

    virtual void process_load_request
        (const RegionLoadRequest &, const Vector2I & spawn_offset,
         RegionLoadCollectorBase &) = 0;
};

class TiledMapRegion final : public MapRegion {
public:
    explicit TiledMapRegion(ProducableTileViewGrid && full_factory_grid);

    void process_load_request
        (const RegionLoadRequest &, const Vector2I & spawn_offset,
         RegionLoadCollectorBase &) final;

private:
    ProducableTileViewGrid m_producables_view_grid;
};

class CompositeMapRegion final : public MapRegion {
public:
    void process_load_request
        (const RegionLoadRequest &, const Vector2I & spawn_offset,
         RegionLoadCollectorBase &) final;
private:
    // not just MapRegions, but how to load them...
    // is it a view grid? not really
    // want to support 3D? nah, not for this "ticket"
};
