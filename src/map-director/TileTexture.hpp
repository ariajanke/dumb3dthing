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

#pragma once

#include "../Definitions.hpp"

class TileTexture final {
public:
    TileTexture() {}

    TileTexture(const Vector2 & nw, const Vector2 & se);

    TileTexture(const Vector2I & tileset_loc, const Size2 & tile_size);

    Vector2 texture_position_for(const Vector2 & tile_normalized_location) const;

private:
    Vector2 m_nw, m_se;
};
