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

#include <common/VectorTraits.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace cul {

template <int kt_dimensionality, typename T, glm::qualifier Q>
struct VectorTraits<glm::vec<kt_dimensionality, T, Q>> {
    static constexpr const bool k_is_vector_type          = true;
    static constexpr const bool k_should_define_operators = false;

    using GlmVec = glm::vec<kt_dimensionality, T, Q>;
    using ScalarType = T;

    template <int kt_idx, typename = int>
    struct Get final {};

    template <typename U>
    struct Get<0, U> final {
        GLM_CONSTEXPR ScalarType operator () (const GlmVec & r) const
            { return r.x; }
    };

    template <typename U>
    struct Get<1, U> final {
        GLM_CONSTEXPR ScalarType operator () (const GlmVec & r) const
            { return r.y; }
    };

    template <typename U>
    struct Get<2, U> final {
        GLM_CONSTEXPR ScalarType operator () (const GlmVec & r) const
            { return r.z; }
    };

    template <typename U>
    struct Get<3, U> final {
        GLM_CONSTEXPR ScalarType operator () (const GlmVec & r) const
            { return r.w; }
    };

    struct Make final {
        template <typename ... Types>
        GLM_CONSTEXPR GlmVec operator () (Types && ... comps) const
            { return GlmVec{ std::forward<Types>(comps)... }; }
    };

    template <typename U>
    using ChangeScalarType = glm::vec<kt_dimensionality, U, Q>;

    static constexpr const int k_dimension_count = kt_dimensionality;
};

} // end of cul(n) namespace
