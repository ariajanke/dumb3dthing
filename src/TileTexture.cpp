/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "TileTexture.hpp"

TileTexture::TileTexture
    (Vector2 nw, Vector2 se):
    m_nw(nw),
    m_se(se)
{}

TileTexture::TileTexture(Vector2I tileset_loc, const Size2 & tile_size) {
    using cul::convert_to;
    Vector2 offset{tileset_loc.x*tile_size.width, tileset_loc.y*tile_size.height};
    m_nw = offset;
    m_se = offset + convert_to<Vector2>(tile_size);
}

Vector2 TileTexture::texture_position_for
    (const Vector2 & tile_normalized_location) const
{
    auto r = tile_normalized_location;
    return Vector2{r.x*m_se.x + m_nw.x*(1 - r.x),
                   r.y*m_se.y + m_nw.y*(1 - r.y)};
}
