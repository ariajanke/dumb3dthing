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

#include "../src/TriangleSegment.hpp"

#include <ariajanke/cul/TreeTestSuite.hpp>

#define mark_it mark_source_position(__LINE__, __FILE__).it

namespace {

using namespace cul::exceptions_abbr;

inline TriangleSegment make_flat_test()
    { return TriangleSegment{Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 1, 0}}; }

inline TriangleSegment make_not_flat_test()
    { return TriangleSegment{Vector{0, 0, 0}, Vector{0, 1, 1}, Vector{1, 1, 2}}; }

template <typename ExpType, typename Func>
cul::tree_ts::TestAssertion expect_exception(Func && f) {
    try {
        f();
    } catch (ExpType &) {
        return cul::tree_ts::test_that(true);
    }
    return cul::tree_ts::test_that(false);
}

template <typename T>
auto test_very_close(const T & lhs, const T & rhs)
    { return cul::tree_ts::test_that(are_very_close(lhs, rhs)); }

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {
using namespace cul::tree_ts;
describe<TriangleSegment>("TriangleSegment #point_at")([] {
    mark_it("sets plane's origin to 3D point a", [] {
        TriangleSegment ts;
        auto a = Vector{1, 2, 3};
        ts = TriangleSegment{a, Vector{4, 5, 6}, Vector{7, 8, 8}};
        return test_very_close(ts.point_at(Vector2{}), a);
    });
    mark_it("+x runs from 3D points a to b", [] {
        auto ts = make_flat_test();
        return test_very_close(ts.point_at(Vector2(0.5, 0.)), Vector{0.5, 0., 0.});
    });
    mark_it("+x ...", [] {
        // basis i here should look like: (0, 1 / sqrt(2), 1 / sqrt(2))
        auto ts = make_not_flat_test();
        Real on_triangle_val = 0.5;
        Real compval = (1. / std::sqrt(2.))*on_triangle_val;
        return test_very_close
            (ts.point_at(Vector2{on_triangle_val, 0.}),
             Vector{0., compval, compval}             );
    });
    mark_it("+y follows normal cross basis i (aka basis j)", [] {
        // I want basis j to favor positive values as being "more inside" the
        // triangle
        auto ts = make_flat_test();
        return test_very_close
            (ts.point_at(Vector2{0., 0.5}), Vector{0., 0.5, 0.});
    });
    mark_it("point_at point a in 2d, is just point a in 3D [flat]", [] {
        auto ts = make_flat_test();
        return test_very_close(ts.point_at(ts.point_a_in_2d()), ts.point_a());
    });
    mark_it("point_at point b in 2d, is just point b in 3D [flat]", [] {
        auto ts = make_flat_test();
        return test_very_close(ts.point_at(ts.point_b_in_2d()), ts.point_b());
    });
    mark_it("point_at point c in 2d, is just point c in 3D [flat]", [] {
        auto ts = make_flat_test();
        return test_very_close(ts.point_at(ts.point_c_in_2d()), ts.point_c());
    });
    mark_it("point_at point a in 2d, is just point a in 3D [3 comp]", [] {
        auto ts = make_flat_test();
        return test_very_close(ts.point_at(ts.point_a_in_2d()), ts.point_a());
    });
    mark_it("point_at point b in 2d, is just point b in 3D [3 comp]", [] {
        auto ts = make_flat_test();
        return test_very_close(ts.point_at(ts.point_b_in_2d()), ts.point_b());
    });
    mark_it("point_at point c in 2d, is just point c in 3D [3 comp]", [] {
        auto ts = make_flat_test();
        return test_very_close(ts.point_at(ts.point_c_in_2d()), ts.point_c());
    });
    mark_it("handles weird point c case", [] {
        TriangleSegment triangle
            {Vector{-0.25, 1,  0.25}, Vector{-0.25, 1, 0.5},
             Vector{ 0.5 , 1, -0.5 }                       };
        return test_very_close
            (triangle.point_at(triangle.point_c_in_2d()), triangle.point_c());
    });
});
describe<TriangleSegment>("TriangleSegment #normal")([] {
    // make sure I'm cross-producting right
    mark_it("normal for flat points in correct direction", [] {
        auto ts = make_flat_test();
        return test_very_close(ts.normal(), Vector(0., 0., 1.));
    });
    mark_it("normal for another triangle is in the correct direction", [] {
        auto ts = make_not_flat_test();
        Real compval = 1. / std::sqrt(3.);
        return test_very_close(ts.normal(), Vector(compval, compval, -compval));
    });
});
describe<TriangleSegment>("TriangleSegment #closest_point")([] {
    mark_it("finds the point closest to the plane of the triangle", [] {
        auto ts = make_flat_test();
        auto p = ts.closest_point(Vector{0.5, 0.5, 0.5});
        return test_very_close(ts.point_at(p), Vector{0.5, 0.5, 0.});
    });
    mark_it("finds the closest point on the plane, from other side", [] {
        auto ts = make_flat_test();
        auto p = ts.closest_point(Vector(-0.5, -0.5, -0.5));

        return test_very_close(ts.point_at(p), Vector(-0.5, -0.5, 0.));
    });
    mark_it("finds the closest point on the plane, even if not inside the triangle", [] {
        auto ts = make_flat_test();
        auto p = ts.closest_point(Vector(10., -10., -123.));
        return test_very_close(ts.point_at(p), Vector(10., -10., 0.));
    });
});
describe<TriangleSegment>("TriangleSegment #contains_point")([] {
    mark_it("detects when a point on the plane, is inside the actual triangle segment", [] {
        Vector2 pt{0.6, 0.6};
        TriangleSegment triangle
            {Vector{0, 0, 0}, Vector{1.4142, 0, 0}, Vector{0.70711, 0.70711, 0}};
        return test_that(triangle.contains_point(pt));
    });
});
describe<TriangleSegment>("TriangleSegment #flip")([] {
    mark_it("flips the normal n such that the new normal is -n", [] {
        TriangleSegment triangle_a;
        auto triangle_b = triangle_a.flip();
        auto ang = angle_between(triangle_a.normal(), triangle_b.normal());
        return test_very_close(ang, k_pi);
    });
});
describe<TriangleSegment>("TriangleSegment #intersection")([] {
    mark_it("considers intersection on the edge of the triangle as a solution", [] {
        Vector a{0.5,  0.1, 0};
        Vector b{0.5, -0.1, 0};
        TriangleSegment triangle{Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 0, 1}};
        auto res = triangle.intersection(a, b);
        return test_that(cul::is_solution(res));
    });
    mark_it("handles issues with 32bit floating points", [] {
        Vector a{3.0999999,  0.0249999985 , -2.0999999};
        Vector b{3.0999999, -0.00416667014, -2.0999999};
        TriangleSegment triangle{Vector{2.5, 0, -2}, Vector{3.5, 0, -2},
                                 Vector{2.5, 0, -3}};
        auto r = triangle.intersection(a, b);
        return test_that(cul::is_solution(r));
    });
});
describe<TriangleSegment>("TriangleSegment #check_for_side_crossing")([] {
    mark_it("accurately identifies which side a line segment cross a side of a triangle", [] {
        TriangleSegment seg{Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 1, 0}};
        auto outside = seg.closest_point(Vector{ -0.1, 0.5, 0 });
        auto inside  = seg.closest_point(Vector{  0.1, 0.5, 0 });
        auto side = seg.check_for_side_crossing(outside, inside).side;
        return test_that(TriangleSide::k_side_ca == side);
    });
});
return 1;
} ();
