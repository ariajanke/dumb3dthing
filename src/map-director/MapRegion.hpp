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

class RegionLoadRequest;
class MapRegionContainerN;
class RegionLoadCollectorN;

class MapRegionN {
public:
    virtual ~MapRegionN() {}

    virtual void process_load_request
        (const RegionLoadRequest &, const Vector2I & spawn_offset,
         RegionLoadCollectorN &) = 0;
};

class TiledMapRegionN final : public MapRegionN {
public:
    explicit TiledMapRegionN(ProducableTileViewGrid && full_factory_grid);

    void process_load_request
        (const RegionLoadRequest &, const Vector2I & spawn_offset,
         RegionLoadCollectorN &) final;

private:
    ProducableTileViewGrid m_producables_view_grid;
};
