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

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <glm/mat4x4.hpp>

#include <glm/geometric.hpp>

#include <glm/gtx/projection.hpp>

template <typename T>
T identity_matrix();

template <>
inline glm::mat4 identity_matrix() {
    glm::mat4 rv;
    rv[0][0] = 1; rv[0][1] = 0; rv[0][2] = 0; rv[0][3] = 0;
    rv[1][0] = 0; rv[1][1] = 1; rv[1][2] = 0; rv[1][3] = 0;
    rv[2][0] = 0; rv[2][1] = 0; rv[2][2] = 1; rv[2][3] = 0;
    rv[3][0] = 0; rv[3][1] = 0; rv[3][2] = 0; rv[3][3] = 1;
    return rv;
}

inline void * pointer_offset(unsigned x) {
    unsigned char * origin = nullptr;
    return origin + x;
}
