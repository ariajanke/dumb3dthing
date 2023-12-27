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

void RampPropertiesLoaderBase::load(const MapTilesetTile & tile) {
    auto elevations = read_elevation_of(tile);
    auto direction  = read_direction_of(tile);
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

/* static */ Optional<CardinalDirection>
    RampPropertiesLoaderBase::cardinal_direction_from(const char * nullable_str)
{
    auto seq = [nullable_str](const char * s) { return !::strcmp(nullable_str, s); };
    using Cd = CardinalDirection;
    if (nullable_str) {
        if (seq("n" )) return Cd::n;
        if (seq("s" )) return Cd::s;
        if (seq("e" )) return Cd::e;
        if (seq("w" )) return Cd::w;
        if (seq("ne")) return Cd::ne;
        if (seq("nw")) return Cd::nw;
        if (seq("se")) return Cd::se;
        if (seq("sw")) return Cd::sw;
    }
    return {};
}

/* static */ Optional<TileCornerElevations>
    RampPropertiesLoaderBase::read_elevation_of
    (const MapTilesetTile & tileset_tile)
{
    if (auto elv = tileset_tile.get_numeric_property<Real>("elevation")) {
        return TileCornerElevations{*elv, *elv, *elv, *elv};
    }
    return {};
}

/* static */ Optional<CardinalDirection>
    RampPropertiesLoaderBase::read_direction_of
    (const MapTilesetTile & tile_properties)
{
    return cardinal_direction_from
        (tile_properties.get_string_property("direction"));
}

// ----------------------------------------------------------------------------

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
    (const TileCornerElevations & neighboring_elevations,
     ProducableTileCallbacks & callbacks) const
{
    m_quad_tileset_tile.make(neighboring_elevations, callbacks);
}
