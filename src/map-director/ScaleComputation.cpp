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

#include "ScaleComputation.hpp"
#include "ParseHelpers.hpp"

#include "../TriangleSegment.hpp"

#include <cstring>

namespace {

constexpr const auto k_comma_splitter = [](char c) { return c == ','; };
constexpr const auto k_whitespace_trimmer =
    make_trim_whitespace<const char *>();

} // end of <anonymous> namespace

/* static */ Optional<ScaleComputation>
    ScaleComputation::parse(const char * string)
{
    if (string == nullptr)
        { return {}; }
    std::array<Real, 3> args;
    auto read_pos = args.begin();
    auto data_substrings = split_range
        (string, string + ::strlen(string), k_comma_splitter,
         k_whitespace_trimmer);
    for (auto data_substring : data_substrings) {
        // too many arguments
        if (read_pos == args.end())
            { return {}; }
        auto read_number = cul::string_to_number
            (data_substring.begin(), data_substring.end(), *read_pos++);
        if (!read_number)
            { return {}; }
    }
    switch (read_pos - args.begin()) {
    case 1: return ScaleComputation{args[0], args[0], args[0]};
    case 3: return ScaleComputation{args[0], args[1], args[2]};
    // 0 or 2 arguments
    default: return {};
    }
}

ScaleComputation::ScaleComputation
    (Real eastwest_factor,
     Real updown_factor,
     Real northsouth_factor):
    m_factor(eastwest_factor, updown_factor, northsouth_factor) {}

TriangleSegment ScaleComputation::of(const TriangleSegment & triangle) const {
    // this is also LoD, but I don't wanna load up triangle segment too much
    return TriangleSegment
        {scale(triangle.point_a()),
         scale(triangle.point_b()),
         scale(triangle.point_c())};
}

RectangleI ScaleComputation::of(const RectangleI & rect) const {
    return RectangleI
        {scale_x(rect.left ), scale_z(rect.top   ),
         scale_x(rect.width), scale_z(rect.height)};
}

bool ScaleComputation::operator == (const ScaleComputation & rhs) const
    { return are_very_close(m_factor, rhs.m_factor); }

ModelScale ScaleComputation::to_model_scale() const
    { return ModelScale{m_factor}; }

Vector ScaleComputation::scale(const Vector & r) const
    { return Vector{r.x*m_factor.x, r.y*m_factor.y, r.z*m_factor.z}; }

// ----------------------------------------------------------------------------

std::array<RegionAxisAddressAndSide, 4>
    ScaledTriangleViewGrid::sides_and_addresses_at
    (const Vector2I & on_field_position) const
{
    return RegionAxisAddressAndSide::for_
        (on_field_position, m_scale.of(m_triangle_grid->size2()));
}
