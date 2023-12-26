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

#include "RampTilesetTileN.hpp"

#include "../TilesetPropertiesGrid.hpp"
#include "../MapTileset.hpp"

#include <cstring>

namespace {

CardinalDirection cardinal_direction_from(const char * str) {
    auto seq = [str](const char * s) { return !::strcmp(str, s); };
    using Cd = CardinalDirection;
    if (!str) {
        throw InvalidArgument
            {"cardinal_direction_from: cannot convert nullptr to a cardinal "
             "direction"                                                     };
    }
    if (seq("n" )) return Cd::n;
    if (seq("s" )) return Cd::s;
    if (seq("e" )) return Cd::e;
    if (seq("w" )) return Cd::w;
#   if 0
    if (seq("ne")) return Cd::ne;
    if (seq("nw")) return Cd::nw;
    if (seq("se")) return Cd::se;
    if (seq("sw")) return Cd::sw;
#   endif
    throw InvalidArgument
        {"cardinal_direction_from: cannot convert \"" + std::string{str} +
         "\" to a cardinal direction"};
}

} // end of <anonymous> namespace

/* static */ CardinalDirection RampTileseTile::read_direction_of
    (const MapTilesetTile & tile_properties)
{
    auto * str = tile_properties.get_string_property("direction");
    if (!str) {
        throw RuntimeError{"I forgor to handle direction not being defined"};
    }
    return cardinal_direction_from(str);
}

/* static */ TileCornerElevations RampTileseTile::elevation_offsets_for
    (CardinalDirection direction)
{
    using Cd = CardinalDirection;
    switch (direction) {
    case Cd::n: return TileCornerElevations{1, 1, 0, 0};
    case Cd::e: return TileCornerElevations{1, 0, 0, 1};
    case Cd::s: return TileCornerElevations{0, 0, 1, 1};
    case Cd::w: return TileCornerElevations{0, 1, 1, 0};
    default:
        throw InvalidArgument{"direction bad"};
    }
}

RampTileseTile::RampTileseTile() {}

void RampTileseTile::load
    (const MapTilesetTile & tileset_tile,
     const TilesetTileTexture & tileset_tile_texture,
     PlatformAssetsStrategy & platform)
{
    auto elevations = FlatTilesetTile::read_elevation_of(tileset_tile);
    if (!elevations) {
        throw RuntimeError{"I forgor to handle elevation not being defined"};
    }
    auto adjusted_elevations =
        elevation_offsets_for(read_direction_of(tileset_tile)).add(*elevations);
    m_quad_tileset_tile.setup
        (tileset_tile_texture, adjusted_elevations, platform);
}

TileCornerElevations RampTileseTile::corner_elevations() const {
    return m_quad_tileset_tile.corner_elevations();
}

void RampTileseTile::make
    (const TileCornerElevations & neighboring_elevations,
     ProducableTileCallbacks & callbacks) const
{
    m_quad_tileset_tile.make(neighboring_elevations, callbacks);
}
