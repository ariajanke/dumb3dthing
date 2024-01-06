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

#include "InRampTilesetTile.hpp"
#include "RampTilesetTile.hpp"

namespace {

using Orientation = InRampPropertiesLoader::Orientation;

} // end of <anonymous> namespace

TileCornerElevations InRampPropertiesLoader::elevation_offsets_for
    (CardinalDirection direction) const
{
    using Cd = CardinalDirection;
    switch (direction) {
    case Cd::north_east: return TileCornerElevations{1, 1, 0, 1};
    case Cd::north_west: return TileCornerElevations{1, 1, 1, 0};
    case Cd::south_east: return TileCornerElevations{1, 0, 1, 1};
    case Cd::south_west: return TileCornerElevations{0, 1, 1, 1};
    default:
        throw InvalidArgument{"direction bad"};
    }
}

Orientation InRampPropertiesLoader::orientation_for
    (CardinalDirection direction) const
{
    using Cd = CardinalDirection;
    switch (direction) {
    case Cd::north_east: case Cd::south_west:
        return Orientation::sw_to_ne_elements;
    case Cd::north_west: case Cd::south_east:
        return Orientation::nw_to_se_elements;
    default:
        throw InvalidArgument{"direction bad"};
    }
}

// ----------------------------------------------------------------------------

void InRampTilesetTile::load
    (const MapTilesetTile & map_tileset_tile,
     const TilesetTileTexture & tileset_texture,
     PlatformAssetsStrategy & platform)
{
    InRampPropertiesLoader loader;
    loader.load(map_tileset_tile);
    if (loader.elements_orientation() == Orientation::nw_to_se_elements) {
        m_quad_tile.set_diagonal_to_nw_to_se();
    } else {
        m_quad_tile.set_diagonal_to_sw_to_ne();
    }
    m_quad_tile.setup(tileset_texture, loader.corner_elevations(), platform);
}

TileCornerElevations InRampTilesetTile::corner_elevations() const
    { return m_quad_tile.corner_elevations(); }

void InRampTilesetTile::make
    (const NeighborCornerElevations &,
     ProducableTileCallbacks & callbacks) const
{ m_quad_tile.make(callbacks); }
