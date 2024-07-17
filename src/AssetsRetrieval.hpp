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

#pragma once

#include "RenderModel.hpp"
#include "Texture.hpp"

class AssetsRetrieval {
public:
    static SharedPtr<AssetsRetrieval> make_non_saving_instance
        (PlatformAssetsStrategy &);

    static SharedPtr<AssetsRetrieval> make_saving_instance
        (PlatformAssetsStrategy &);

    virtual ~AssetsRetrieval() {}

    virtual SharedPtr<const RenderModel> make_cube_model() = 0;

    virtual SharedPtr<const RenderModel> make_cone_model() = 0;

    virtual SharedPtr<const RenderModel> make_vaguely_tree_like_model() = 0;

    virtual SharedPtr<const RenderModel> make_grass_model() = 0;

    virtual SharedPtr<const Texture> make_ground_texture() = 0;
};
