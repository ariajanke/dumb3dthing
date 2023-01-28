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

#include "../Defs.hpp"

class TileSetXmlGrid;
class Platform;

struct TileLocation final {
    Vector2I on_map;
    Vector2I on_field;
};

/// How to fill out a grid with a tile group.
///
///
class ProducableTileFiller {
public:
    struct TileLocation final {
        Vector2I location_on_map;
        Vector2I location_on_tileset;
    };

    virtual ~ProducableTileFiller() {}

    virtual UnfinishedTileGroupGrid operator ()
        (const std::vector<TileLocation> &, UnfinishedTileGroupGrid &&) const = 0;
};
