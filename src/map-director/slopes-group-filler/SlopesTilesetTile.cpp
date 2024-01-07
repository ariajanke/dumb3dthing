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

/* static */ NeighborCornerElevations::NeighborElevations &
    NeighborCornerElevations::NeighborElevations::null_instance()
{
    class Impl final : public NeighborElevations {
        TileCornerElevations elevations_from
            (const Vector2I &, CardinalDirection) const final
        { throw RuntimeError{""}; }
    };
    static Impl impl;
    return impl;
}

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

// ----------------------------------------------------------------------------

TileCornerElevations::TileCornerElevations
    (Optional<Real> ne_,
     Optional<Real> nw_,
     Optional<Real> sw_,
     Optional<Real> se_):
    m_nw(nw_.value_or(k_inf)),
    m_ne(ne_.value_or(k_inf)),
    m_sw(sw_.value_or(k_inf)),
    m_se(se_.value_or(k_inf)) {}

bool TileCornerElevations::operator == (const TileCornerElevations & rhs) const noexcept
    { return are_same(rhs); }

bool TileCornerElevations::operator != (const TileCornerElevations & rhs) const noexcept
    { return !are_same(rhs); }

TileCornerElevations TileCornerElevations::add
    (const TileCornerElevations & rhs) const
{
    return TileCornerElevations
        {add(north_east(), rhs.north_east()),
         add(north_west(), rhs.north_west()),
         add(south_west(), rhs.south_west()),
         add(south_east(), rhs.south_east())};
}

TileCornerElevations TileCornerElevations::value_or
    (const NeighborCornerElevations & rhs) const
{
    using Nce = NeighborCornerElevations;
    return TileCornerElevations
        {value_or<&Nce::north_east>(north_east(), rhs),
         value_or<&Nce::north_west>(north_west(), rhs),
         value_or<&Nce::south_west>(south_west(), rhs),
         value_or<&Nce::south_east>(south_east(), rhs)};
}

/* private static */ Optional<Real> TileCornerElevations::add
    (const Optional<Real> & lhs, const Optional<Real> & rhs)
{
    if (!lhs && !rhs)
        { return {}; }
    return lhs.value_or(0) + rhs.value_or(0);
}

/* private static */ Optional<Real> TileCornerElevations::value_or
    (const Optional<Real> & lhs, const Optional<Real> & rhs)
{ return lhs ? lhs : rhs; }

template <Optional<Real>(NeighborCornerElevations::*kt_corner_f)() const>
/* private static */ Optional<Real> TileCornerElevations::value_or
    (const Optional<Real> & lhs, const NeighborCornerElevations & rhs)
{
    if (lhs) {
        return lhs;
    }
    return (rhs.*kt_corner_f)();
}

/* private static */ Optional<Real> TileCornerElevations::as_optional(Real r) {
    if (std::equal_to<Real>{}(r, k_inf))
        { return {}; }
    return r;
}

/* private */ bool TileCornerElevations::are_same
    (const TileCornerElevations & rhs) const noexcept
{
    using Fe = std::equal_to<Real>;
    return    Fe{}(m_nw, rhs.m_nw) && Fe{}(m_ne, rhs.m_ne)
           && Fe{}(m_sw, rhs.m_sw) && Fe{}(m_se, rhs.m_se);
}

// ----------------------------------------------------------------------------

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
