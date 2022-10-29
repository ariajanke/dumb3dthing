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

#include "SweepLine.hpp"

SweepLine::Interval SweepLine::interval_for(const Triangle & triangle) const {
    Interval rv;
    rv.min =  k_inf;
    rv.max = -k_inf;
    auto pts_on_line = {
        point_for(triangle.point_a()),
        point_for(triangle.point_b()),
        point_for(triangle.point_c())
    };
    for (auto v : pts_on_line) {
        rv.min = std::min(rv.min, v);
        rv.max = std::max(rv.max, v);
    }
    return rv;
}

Real SweepLine::point_for(const Vector & r) const {
    auto r_on_line = cul::find_closest_point_to_line(m_a, m_b, r);
    auto mag = magnitude(r_on_line - m_a);
    Real dir = dot(r_on_line - m_a, m_b - m_a) < 0 ? -1 : 1;
    return mag*dir;
}
