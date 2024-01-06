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

#include "RampTilesetTile.hpp"

#include "../TilesetPropertiesGrid.hpp"
#include "../MapTileset.hpp"

#include <map>

#include <cstring>

namespace {

Optional<CardinalDirection> cardinal_direction_from(const char * nullable_str) {
    static const auto k_strings_as_directions = [] {
        using Cd = CardinalDirection;
        std::map<std::string, CardinalDirection> rv {
            { "n"         , Cd::north      },
            { "s"         , Cd::south      },
            { "e"         , Cd::east       },
            { "w"         , Cd::west       },
            { "ne"        , Cd::north_east },
            { "nw"        , Cd::north_west },
            { "se"        , Cd::south_east },
            { "sw"        , Cd::south_west },
            { "north"     , Cd::north      },
            { "south"     , Cd::south      },
            { "east"      , Cd::east       },
            { "west"      , Cd::west       },
            { "north-east", Cd::north_east },
            { "north-west", Cd::north_west },
            { "south-east", Cd::south_east },
            { "south-west", Cd::south_west },
        };
        return rv;
    } ();
    auto seq = [nullable_str](const char * s) { return !::strcmp(nullable_str, s); };
    using Cd = CardinalDirection;
    if (nullable_str) {
        auto itr = k_strings_as_directions.find(nullable_str);
        if (itr != k_strings_as_directions.end()) {
            return itr->second;
        }
    }
    return {};
}

} // end of <anonymous> namespace

void RampPropertiesLoaderBase::load(const MapTilesetTile & tile) {
    auto elevations = FlatTilesetTile::read_elevation_of(tile);
    auto direction  = RampTileseTile ::read_direction_of(tile);
    if (elevations) {
        m_elevations = *elevations;
    } else {
        m_elevations = TileCornerElevations{};
    }
    if (direction) {
        m_elevations = m_elevations.add(elevation_offsets_for(*direction));
        m_orientation = orientation_for(*direction);
    }
}

const TileCornerElevations & RampPropertiesLoaderBase::corner_elevations() const
    { return m_elevations; }

// ----------------------------------------------------------------------------

/* static */ Optional<CardinalDirection> RampTileseTile::read_direction_of
    (const MapTilesetTile & map_tileset_tile)
{
    return cardinal_direction_from
        (map_tileset_tile.get_string_property("direction"));
}

void RampTileseTile::load
    (const MapTilesetTile & tileset_tile,
     const TilesetTileTexture & tileset_tile_texture,
     PlatformAssetsStrategy & platform)
{
    RampPropertiesLoader loader;
    loader.load(tileset_tile);
    m_quad_tileset_tile.
        setup(tileset_tile_texture, loader.corner_elevations(), platform);
}

TileCornerElevations RampTileseTile::corner_elevations() const {
    return m_quad_tileset_tile.corner_elevations();
}

void RampTileseTile::make
    (const NeighborCornerElevations &, ProducableTileCallbacks & callbacks) const
{ m_quad_tileset_tile.make(callbacks); }
