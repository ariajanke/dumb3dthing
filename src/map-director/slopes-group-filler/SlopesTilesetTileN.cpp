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


#include "SlopesTilesetTileN.hpp"
#include "../MapTileset.hpp"
#include "../../Texture.hpp"

namespace {

Optional<Real> first_of(Optional<Real> a, Optional<Real> b, Optional<Real> c) {
    if (a) return a;
    if (b) return b;
    return c;
}

} // end of <anonymous> namespace

TileCornerElevations TileCornerElevations::NeighborElevations::elevations() const {
    using Cd = CardinalDirection;
    const auto & northern_tile = elevations_from(Cd::n);
    const auto & southern_tile = elevations_from(Cd::s);
    const auto & eastern_tile = elevations_from(Cd::e);
    const auto & western_tile = elevations_from(Cd::w);

    auto north_west = first_of
        (northern_tile.south_west(),
         elevations_from(Cd::nw).south_east(),
         western_tile.north_east());
    auto south_west = first_of
        (western_tile.south_east(),
         elevations_from(Cd::sw).north_east(),
         southern_tile.north_west());
    auto south_east = first_of
        (southern_tile.north_east(),
         elevations_from(Cd::se).north_west(),
         eastern_tile.south_west());
    auto north_east = first_of
        (eastern_tile.north_west(),
         elevations_from(Cd::ne).south_west(),
         northern_tile.south_east());
    return TileCornerElevations
        {north_east, north_west, south_west, south_east};
}

void TilesetTileTexture::load_texture
    (const MapTileset & map_tileset, PlatformAssetsStrategy & platform)
{
    auto texture = platform.make_texture();
    const auto image_tag = map_tileset.image();
    texture->load_from_file(image_tag.filename());
    m_texture = std::move(texture);
    auto tile_width = map_tileset.get_numeric_attribute<int>("tilewidth");
    auto tile_height = map_tileset.get_numeric_attribute<int>("tileheight");
    if (!tile_width || !tile_height) {
        throw RuntimeError{"I forgor"};
    }
    const auto & image_size = image_tag.image_size();
    m_tile_size_in_portions = Size2
        {*tile_width / image_size.width, *tile_height / image_size.height};
}

void TilesetTileTexture::set_texture_bounds
    (const Vector2I & location_on_tileset)
{
    m_north_west = Vector2
        {location_on_tileset.x*m_tile_size_in_portions.width,
         location_on_tileset.y*m_tile_size_in_portions.height};
}

Vector2 TilesetTileTexture::north_east() const {
    return m_north_west + Vector2{m_tile_size_in_portions.width, 0};
}

Vector2 TilesetTileTexture::north_west() const {
    return m_north_west;
}

Vector2 TilesetTileTexture::south_east() const {
    return m_north_west +
        Vector2{m_tile_size_in_portions.width, m_tile_size_in_portions.height};
}

Vector2 TilesetTileTexture::south_west() const {
    return m_north_west + Vector2{0, m_tile_size_in_portions.height};
}

const SharedPtr<const Texture> & TilesetTileTexture::texture() const {
    return m_texture;
}
