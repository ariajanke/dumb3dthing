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

#include "SplitWallGeometry.hpp"

namespace {

StripVertex::StripSide other_side_of(StripVertex::StripSide side);

} // end of <anonymous> namespace

StripTriangle::StripTriangle
    (const StripVertex & a,
     const StripVertex & b,
     const StripVertex & c):
    m_a(a),
    m_b(b),
    m_c(c) {}

TriangleSegment StripTriangle::to_triangle_segment() const {
    return TriangleSegment{m_a.point, m_b.point, m_c.point};
}

StripVertex StripTriangle::vertex_a() const { return m_a; }

StripVertex StripTriangle::vertex_b() const { return m_b; }

StripVertex StripTriangle::vertex_c() const { return m_c; }

// ----------------------------------------------------------------------------

/* private */ void LinearStripTriangleCollection::triangle_strip
    (const Vector & a_point,
     const Vector & b_start,
     const Vector & b_last,
     StripVertex::StripSide a_side,
     int steps_count)
{
    if (are_very_close(b_start, b_last)) {
        throw InvalidArgument{""};
    }
    auto b_side_pt = [b_start, b_last] (Real t)
        { return b_start*(1 - t) + b_last*t; };
    auto b_side = other_side_of(a_side);
    for (int i = 0; i != steps_count; ++i) {
        Real t = Real(i) / Real(steps_count);
        Real next_t = Real(i + 1) / Real(steps_count);
        add_triangle(StripTriangle
            {StripVertex{a_point, {}, a_side},
             StripVertex{b_side_pt(t     ), t     , b_side},
             StripVertex{b_side_pt(next_t), next_t, b_side}});
    }
}

void LinearStripTriangleCollection::make_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     int steps_count)
{
    using StripSide = StripVertex::StripSide;
    using Triangle = TriangleSegment;
    if (steps_count < 0) {
        throw InvalidArgument{"steps_count must be a non-negative integer"};
    }

    if (   are_very_close(a_start, a_last)
        && are_very_close(b_start, b_last))
    {
        return;
    }
    // atempting to generate a one dimensional line
    else if (   are_very_close(a_start, b_start)
             && are_very_close(a_last , b_last ))
    {
        return;
    } else if (steps_count == 0) {
        return;
    } else if (are_very_close(a_start, a_last)) {
        return triangle_strip(a_start, b_start, b_last, StripSide::a, steps_count);
    } else if (are_very_close(b_start, b_last)) {
        return triangle_strip(b_start, a_start, a_last, StripSide::b, steps_count);
    }

    auto a_side_pt = [a_start, a_last] (Real t)
        { return a_start*(1 - t) + a_last*t; };
    auto b_side_pt = [b_start, b_last] (Real t)
        { return b_start*(1 - t) + b_last*t; };
    auto normal_step = [this, &a_side_pt, &b_side_pt] (Real last, Real next) {
        add_triangle(StripTriangle
            {StripVertex{a_side_pt(last), last, StripSide::a},
             StripVertex{b_side_pt(last), last, StripSide::b},
             StripVertex{b_side_pt(next), next, StripSide::b}});
        add_triangle(StripTriangle
            {StripVertex{a_side_pt(last), last, StripSide::a},
             StripVertex{a_side_pt(next), next, StripSide::a},
             StripVertex{b_side_pt(next), next, StripSide::b}});
    };
    auto first_step =
        [this, &a_side_pt, &b_side_pt, &a_start, &b_start]
        (Real next)
    {
        assert(are_very_close(a_start, b_start));
        add_triangle(StripTriangle
            {StripVertex{a_start, 0, {}},
             StripVertex{a_side_pt(next), next, StripSide::a},
             StripVertex{b_side_pt(next), next, StripSide::b}});
    };
    int step = 0;
    Real step_size = 1 / Real(steps_count);
    auto get_last = [step_size, step] { return step*step_size; };
    auto get_next = [step_size, step] { return (step + 1)*step_size; };

    if (step == 0 && steps_count > 1) {
        if (are_very_close(a_start, b_start)) {
            first_step(get_next());
        } else {
            // normal first step
            normal_step(get_last(), get_next());
        }
        ++step;
    }

    for (; step < steps_count - 1; ++step) {
        normal_step(get_last(), get_next());
    }

    if (step == 0 && are_very_close(a_start, b_start)) {
        first_step(get_next());
    } else if (are_very_close(a_last, b_last)) {
        auto last = get_last();
        add_triangle(StripTriangle
            {StripVertex{a_side_pt(last), last, StripSide::a},
             StripVertex{b_side_pt(last), last, StripSide::b},
             StripVertex{a_last, 1, StripSide::both}});
    } else {
        normal_step(get_last(), get_next());
    }
}

// ----------------------------------------------------------------------------

/* static */ SplitWallGeometry::GeometryGenerationStrategy &
    SplitWallGeometry::null_generation_strategy
    (CardinalDirection)
{
    throw RuntimeError{"generation strategy not set"};
}

namespace {

StripVertex::StripSide other_side_of(StripVertex::StripSide side) {
    using Side = StripVertex::StripSide;
    switch (side) {
    case Side::a: return Side::b;
    case Side::b: return Side::a;
    default: break;
    }
    throw InvalidArgument{"bad argument"};
}

} // end of <anonymous> namespace
