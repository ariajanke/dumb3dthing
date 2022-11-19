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

#include "test-functions.hpp"
#include "../../TriangleSegment.hpp"

#include <ariajanke/cul/TestSuite.hpp>

namespace {

inline TriangleSegment make_flat_test()
    { return TriangleSegment{Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 1, 0}}; }

inline TriangleSegment make_not_flat_test()
    { return TriangleSegment{Vector{0, 0, 0}, Vector{0, 1, 1}, Vector{1, 1, 2}}; }

} // end of <anonymous> namespace

bool run_triangle_segment_tests() {
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    using namespace cul::ts;
    using Side = TriangleSegment::Side;
    TestSuite suite;
    suite.start_series("TriangleSegment");

    // testing position_on method
    // proper origin?
    mark(suite).test([] {
        TriangleSegment ts;
        auto a = Vector{1, 2, 3};
        ts = TriangleSegment{a, Vector{4, 5, 6}, Vector{7, 8, 8}};
        return test(are_very_close(ts.point_at(Vector2()), a));
    });

    // test normal
    mark(suite).test([] {
        auto ts = make_flat_test();
        return test(are_very_close(ts.normal(), Vector(0., 0., 1.)));
    });

    mark(suite).test([] {
        auto ts = make_not_flat_test();
        Real compval = 1. / std::sqrt(3.);
        return test(are_very_close(ts.normal(), Vector(compval, compval, -compval)));
    });

    // test along basis i (both z = 0, z varies)
    // this is done using point_at
    mark(suite).test([] {
        // basis i here should look like: (0, 1, 0)
        auto ts = make_flat_test();
        return test(are_very_close(ts.point_at(Vector2(0.5, 0.)), Vector{0.5, 0., 0.}));
    });

    mark(suite).test([] {
        // basis i here should look like: (0, 1 / sqrt(2), 1 / sqrt(2))
        auto ts = make_not_flat_test();
        Real on_triangle_val = 0.5;
        Real compval = (1. / std::sqrt(2.))*on_triangle_val;
        return test(are_very_close(ts.point_at(Vector2(on_triangle_val, 0.)),
                             Vector(0., compval, compval)             ));
    });

    // test along basis j (both z = 0, z varies)
    mark(suite).test([] {
        // I want basis j to favor positive values as being "more inside" the
        // triangle
        auto ts = make_flat_test();
        std::cout << ts.point_at(Vector2(0., 0.5)) << std::endl;
        return test(are_very_close(ts.point_at(Vector2(0., 0.5)), Vector(0., 0.5, 0.)));
    });

    // test along both (both z = 0, z varies)

    // closest_point
    // is found via projection, it maybe inside or not
    mark(suite).test([] {
        // generally everything projected on the flat test surface should end up on
        // the xy plane
        auto ts = make_flat_test();
        auto p = ts.closest_point(Vector(0.5, 0.5, 0.5));
        std::cout << p << "; " << ts.point_at(p) << std::endl;
        return test(are_very_close(ts.point_at(p), Vector(0.5, 0.5, 0.)));
    });

    mark(suite).test([] {
        auto ts = make_flat_test();
        auto p = ts.closest_point(Vector(-0.5, -0.5, -0.5));
        std::cout << p << "; " << ts.point_at(p) << std::endl;
        return test(are_very_close(ts.point_at(p), Vector(-0.5, -0.5, 0.)));
    });

    mark(suite).test([] {
        auto ts = make_flat_test();
        auto p = ts.closest_point(Vector(10., -10., -123.));
        return test(are_very_close(ts.point_at(p), Vector(10., -10., 0.)));
    });
    // generally: a is very close to ts.closest_point(ts.point_at(a))

    // intersection
    // two regular intersections
    // one outside

    // point_region
    // two inside
    // one in each region: k_out_of_ab, k_out_of_bc, k_out_of_ca

    // points in 2d
    set_context(suite, [](TestSuite & suite, Unit & unit) {
        auto ts = make_flat_test();
        unit.start(mark(suite), [&] {
            return test(are_very_close(ts.point_at(ts.point_a_in_2d()), ts.point_a()));
        });
        unit.start(mark(suite), [&] {
            return test(are_very_close(ts.point_at(ts.point_b_in_2d()), ts.point_b()));
        });
        unit.start(mark(suite), [&] {
            return test(are_very_close(ts.point_at(ts.point_c_in_2d()), ts.point_c()));
        });
    });

    set_context(suite, [](TestSuite & suite, Unit & unit) {
        auto ts = make_not_flat_test();
        unit.start(mark(suite), [&] {
            return test(are_very_close(ts.point_at(ts.point_a_in_2d()), ts.point_a()));
        });
        unit.start(mark(suite), [&] {
            return test(are_very_close(ts.point_at(ts.point_b_in_2d()), ts.point_b()));
        });
        unit.start(mark(suite), [&] {
            return test(are_very_close(ts.point_at(ts.point_c_in_2d()), ts.point_c()));
        });
    });

    mark(suite).test([] {
        TriangleSegment seg{Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 1, 0}};
        auto outside = seg.closest_point(Vector{ -0.1, 0.5, 0 });
        auto inside  = seg.closest_point(Vector{  0.1, 0.5, 0 });
        auto side = seg.check_for_side_crossing(outside, inside).side;
        return test(TriangleSide::k_side_ca == side);
    });

#   if 0 // made invalid with change of interface
         // specifically: edges are inside the triangle now
    mark(suite).test([] {
        Vector2 old{0.9999863376435526, 0.47360747242417789};
        Vector2 new_{1.00024374529933, 0.50750195015971578};
        TriangleSegment tri{Vector{5.5, 0, -3.5}, Vector{5.5, 0, -4.5},
                            Vector{6.5, 0, -4.5}};
        auto side = tri.check_for_side_crossing(old, new_).side;
        return test(side != Side::k_inside);
    });
#   endif
    // fails?!
    // <x: 0.6, y: 0.6> <x: 0, y: 0> <x: 1.4142, y: 0> <x: 0.70711, y: 0.70711>

    mark(suite).test([] {
        Vector2 pt{0.6, 0.6};
        TriangleSegment triangle{Vector{0, 0, 0}, Vector{1.4142, 0, 0}, Vector{0.70711, 0.70711, 0}};
        return test(triangle.contains_point(pt));
    });

    // issues with 32bit floats
    mark(suite).test([] {
        Vector a{3.0999999,  0.0249999985 , -2.0999999};
        Vector b{3.0999999, -0.00416667014, -2.0999999};
        TriangleSegment triangle{Vector{2.5, 0, -2}, Vector{3.5, 0, -2},
                                 Vector{2.5, 0, -3}};
        auto r = triangle.intersection(a, b);
        return test(cul::is_solution(r));
    });

    // must collide with on edge of triangle
    mark(suite).test([] {
        Vector a{0.5,  0.1, 0};
        Vector b{0.5, -0.1, 0};
        TriangleSegment triangle{Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{0, 0, 1}};
        auto res = triangle.intersection(a, b);
        return test(cul::is_solution(res));
    });

    mark(suite).test([] {
        TriangleSegment triangle{Vector{-0.25, 1, 0.25}, Vector{-0.25, 1, 0.5}, Vector{0.5, 1, -0.5}};
        return test(are_very_close(triangle.point_at(triangle.point_c_in_2d()), triangle.point_c()));
    });

    mark(suite).test([] {
        TriangleSegment triangle_a;
        auto triangle_b = triangle_a.flip();
        auto ang = angle_between(triangle_a.normal(), triangle_b.normal());
        return test(are_very_close(ang, k_pi));
    });

#   undef mark
    return suite.has_successes_only();
}
