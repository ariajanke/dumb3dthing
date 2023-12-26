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

#include "OutRampTilesetTileN.hpp"

/* static */ TileCornerElevations OutRampTilesetTile::elevation_offsets_for
    (CardinalDirection direction)
{
    using Cd = CardinalDirection;
    switch (direction) {
    case Cd::ne: return TileCornerElevations{0, 0, 1, 0};
    case Cd::nw: return TileCornerElevations{0, 0, 0, 1};
    case Cd::se: return TileCornerElevations{0, 1, 0, 0};
    case Cd::sw: return TileCornerElevations{1, 0, 0, 0};
    default:
        throw InvalidArgument{"direction bad"};
    }
}

void OutRampTilesetTile::load
    (const MapTilesetTile & map_tileset_tile,
     const TilesetTileTexture & tileset_texture,
     PlatformAssetsStrategy & platform)
{

}

TileCornerElevations OutRampTilesetTile::corner_elevations() const {

}

void OutRampTilesetTile::make
    (const TileCornerElevations & neighboring_elevations,
     ProducableTileCallbacks & callbacks) const
{

}
