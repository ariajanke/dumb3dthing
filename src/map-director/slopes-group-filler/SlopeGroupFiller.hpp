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

#include "../ProducableGroupFiller.hpp"
#include "../TilesetPropertiesGrid.hpp"

#include <map>

class SlopesTilesetTile;
class MapTileset;

class SlopeGroupFiller final : public ProducableGroupFiller {
public:
    using TilesetTilePtr = SharedPtr<SlopesTilesetTile>;
    using TilesetTileMakerFunction = TilesetTilePtr(*)();
    using TilesetTileMakerMap = std::map<std::string, TilesetTileMakerFunction>;
    using TilesetTileGrid = Grid<TilesetTilePtr>;
    using TilesetTileGridPtr = SharedPtr<TilesetTileGrid>;

    static const TilesetTileMakerMap & builtin_makers();

    void make_group(CallbackWithCreator &) const final;

    void load
        (const MapTileset & map_tileset,
         PlatformAssetsStrategy & platform,
         const TilesetTileMakerMap & = builtin_makers());

private:
    TilesetTileGridPtr m_tileset_tiles;
};
