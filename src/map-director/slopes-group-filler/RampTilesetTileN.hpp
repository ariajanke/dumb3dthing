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

#include "SlopesTilesetTileN.hpp"
#include "FlatTilesetTileN.hpp"

class RampTileseTile final : public SlopesTilesetTile {
public:
    static CardinalDirection read_direction_of(const TileProperties &);

    static TileCornerElevations elevation_offsets_for(CardinalDirection);

    void load
        (const TilesetXmlGrid &,
         const Vector2I & location_on_tileset,
         PlatformAssetsStrategy & platform) final;

    TileCornerElevations corner_elevations() const final;

    void make
        (const TileCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const;

private:
    FlatTilesetTile m_flat_tileset_tile;
};
