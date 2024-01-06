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

#include "SlopesTilesetTile.hpp"
#include "../MapTileset.hpp"
#include "../../Texture.hpp"

namespace {

Optional<Real> first_of(Optional<Real> a, Optional<Real> b, Optional<Real> c) {
    if (a) return a;
    if (b) return b;
    return c;
}

} // end of <anonymous> namespace

Optional<Real> NeighborCornerElevations::north_east() const {
    using Cd = CardinalDirection;
    return first_of
        (elevations_from(Cd::north     ).south_east(),
         elevations_from(Cd::east      ).north_west(),
         elevations_from(Cd::north_east).south_west());
}

Optional<Real> NeighborCornerElevations::north_west() const {
    using Cd = CardinalDirection;
    return first_of
        (elevations_from(Cd::north     ).south_west(),
         elevations_from(Cd::west      ).north_east(),
         elevations_from(Cd::north_west).south_east());
}

Optional<Real> NeighborCornerElevations::south_east() const {
    using Cd = CardinalDirection;
    return first_of
        (elevations_from(Cd::south     ).north_east(),
         elevations_from(Cd::east      ).south_west(),
         elevations_from(Cd::south_east).north_west());
}

Optional<Real> NeighborCornerElevations::south_west() const {
    using Cd = CardinalDirection;
    return first_of
        (elevations_from(Cd::south     ).north_west(),
         elevations_from(Cd::west      ).south_east(),
         elevations_from(Cd::south_west).north_east());
}

/* private */ TileCornerElevations NeighborCornerElevations::elevations_from
    (CardinalDirection cd) const
{ return m_neighbors->elevations_from(m_location, cd); }

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

Vertex TilesetTileTexture::interpolate(Vertex vtx) const {
    // x is west-east
    // y is north-south
    auto & tx = vtx.texture_position.x;
    auto & ty = vtx.texture_position.y;
    tx = m_north_west.x + tx*m_tile_size_in_portions.width;
    ty = m_north_west.y + ty*m_tile_size_in_portions.height;
    return vtx;
}
