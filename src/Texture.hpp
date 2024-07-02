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

#pragma once

#include "Definitions.hpp"

class PlatformAssetsStrategy;

class Texture {
public:
    static SharedPtr<Texture> make_ground(PlatformAssetsStrategy &);

    virtual ~Texture() {}

    virtual bool load_from_file_no_throw(const char *) noexcept = 0;

    void load_from_file(const char *);

    virtual void load_from_memory(int width_, int height_, const void * rgba_pixels) = 0;

    virtual int width () const = 0;

    virtual int height() const = 0;

    virtual void bind_texture(/* there is a rendering context in WebGL */) const = 0;
};
