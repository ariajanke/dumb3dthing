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

#include "../Definitions.hpp"
#include "MapElementValuesMap.hpp"
#include "MapObject.hpp" // just for document owning node

class MapTileset;

class MapTilesetTile final : public MapElementValuesAggregable {
public:
    const MapTileset * parent_tileset() const;

    const char * type() const;

    int id() const;
};

// ----------------------------------------------------------------------------

class MapTileset final : public MapElementValuesAggregable {
public:
    void load(const DocumentOwningNode & tileset_el);

    // Vector2 texture_position_at(const Vector2I & tile_location) const;

    const MapTilesetTile * tile_at(const Vector2I &) const;

    Vector2I next(const Vector2I &) const;

    Vector2I end_position() const;

    std::size_t tile_count() const;

    Size2I size2() const;

private:
    std::vector<MapTilesetTile> m_tiles;
    Grid<const MapTilesetTile *> m_tile_grid;
};
