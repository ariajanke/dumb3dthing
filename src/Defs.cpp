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

#include "Defs.hpp"
#include "TriangleSegment.hpp"

#include <iostream>

namespace {

constexpr const Real k_error = 0.0005;

inline Real round_close_to_zero(Real x)
    { return are_very_close(x, 0.) ? 0. : x; }

} // end of <anonymous> namespace

BadBranchException::BadBranchException(int line, const char * file):
    std::runtime_error(  "Bad \"impossible\" branch hit at: "
                       + std::string{file} + " line "
                       + std::to_string(line))
{}

Vector next_in_direction(Vector r, Vector dir) {
    return Vector{std::nextafter(r.x, r.x + dir.x),
                  std::nextafter(r.y, r.y + dir.y),
                  std::nextafter(r.z, r.z + dir.z)};
}

Vector2 next_in_direction(Vector2 r, Vector2 dir) {
    return Vector2{std::nextafter(r.x, r.x + dir.x),
                   std::nextafter(r.y, r.y + dir.y)};
}

bool are_very_close(Vector a, Vector b)
    { return cul::sum_of_squares(a - b) <= k_error*k_error; }

bool are_very_close(Vector2 a, Vector2 b)
    { return cul::sum_of_squares(a - b) <= k_error*k_error; }

bool are_very_close(Real a, Real b)
    { return (a - b)*(a - b) <= k_error*k_error; }

std::ostream & operator << (std::ostream & out, const Vector & r) {
    auto old_prec = out.precision();
    out.precision(5);
    out << "<x: " << round_close_to_zero(r.x)
        << ", y: " << round_close_to_zero(r.y)
        << ", z: " << round_close_to_zero(r.z) << ">";
    out.precision(old_prec);
    return out;
}

std::ostream & operator << (std::ostream & out, const Vector2 & r) {
    auto old_prec = out.precision();
    out.precision(5);
    out << "<x: " << round_close_to_zero(r.x)
        << ", y: " << round_close_to_zero(r.y) << ">";
    out.precision(old_prec);
    return out;
}

std::ostream & operator <<
    (std::ostream & out, const TriangleSegment & triangle)
{
    out << "a: " << triangle.point_a() << " b: " << triangle.point_b()
        << " c: " << triangle.point_c();
    return out;
}
