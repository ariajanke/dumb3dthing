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
#if 0
#pragma once

#include "../MapTileset.hpp"

class TexturedMapTilesetTile final {
public:
    void load(const MapTilesetTile &, PlatformAssetsStrategy &);

    Vector2 texture_position() const;

    Size2 texture_size() const;
};

class TexturedMapTileset final {
public:
    void load(const MapTileset &, PlatformAssetsStrategy & platform);

    const SharedPtr<const Texture> & texture() const;

private:
    SharedPtr<const Texture> m_texture;
};
#endif
