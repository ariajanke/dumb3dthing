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

#include "OutRampTilesetTile.hpp"
#include "RampTilesetTile.hpp"

namespace {

using Orientation = OutRampPropertiesLoader::Orientation;

} // end of <anonymous> namespace

void OutRampTilesetTile::load
    (const MapTilesetTile & map_tileset_tile,
     const TilesetTileTexture & tileset_texture,
     PlatformAssetsStrategy & platform)
{
    OutRampPropertiesLoader loader;
    loader.load(map_tileset_tile);
    if (loader.elements_orientation() == Orientation::nw_to_se_elements) {
        m_quad_tile.set_diagonal_to_nw_to_se();
    } else {
        m_quad_tile.set_diagonal_to_sw_to_ne();
    }
    m_quad_tile.setup(tileset_texture, loader.corner_elevations(), platform);
}

TileCornerElevations OutRampTilesetTile::corner_elevations() const
    { return m_quad_tile.corner_elevations(); }

void OutRampTilesetTile::make
    (const NeighborCornerElevations &, ProducableTileCallbacks & callbacks) const
{ m_quad_tile.make(callbacks); }
