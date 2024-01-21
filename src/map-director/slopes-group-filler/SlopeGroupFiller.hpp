/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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
#include "SlopesTilesetTile.hpp"

#include <map>

class SlopesTilesetTile;
class MapTileset;

class ProducableSlopesTile final : public ProducableTile {
public:
    ProducableSlopesTile() {}

    explicit ProducableSlopesTile
        (const SharedPtr<const SlopesTilesetTile> & tileset_tile_ptr):
        m_tileset_tile_ptr(tileset_tile_ptr) {}

    void set_neighboring_elevations(const NeighborCornerElevations & elvs) {
        m_elevations = elvs;
    }

    void operator () (ProducableTileCallbacks & callbacks) const final {
        if (m_tileset_tile_ptr)
            m_tileset_tile_ptr->make(m_elevations, callbacks);
    }

private:
    const SlopesTilesetTile & verify_tile_ptr() const {
        if (m_tileset_tile_ptr)
            { return *m_tileset_tile_ptr; }
        throw RuntimeError
            {"Accessor not available without setting tileset tile pointer"};
    }

    SharedPtr<const SlopesTilesetTile> m_tileset_tile_ptr;
    NeighborCornerElevations m_elevations;
};

// ----------------------------------------------------------------------------

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
