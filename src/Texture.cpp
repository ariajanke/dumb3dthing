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

#include "Texture.hpp"
#include "Definitions.hpp"
#include "platform.hpp"

#include <ariajanke/cul/Util.hpp>

#include <string>

/* static */ SharedPtr<Texture> Texture::make_ground
    (PlatformAssetsStrategy & platform)
{
    static WeakPtr<Texture> s_memoized;
    if (!s_memoized.expired()) {
        return s_memoized.lock();
    }
    auto tx = platform.make_texture();
    tx->load_from_file("ground.png");
    s_memoized = tx;
    return tx;
}

void Texture::load_from_file(const char * filename) {
    if (load_from_file_no_throw(filename)) return;

    throw RuntimeError
        {std::string{"Texture::load_from_file: Failed to load texture \""} +
         filename + "\""};
}
