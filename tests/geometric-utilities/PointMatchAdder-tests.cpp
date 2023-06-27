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

#include "../src/TasksController.hpp"

#include "../src/geometric-utilities/PointMatchAdder.hpp"

#include "../test-helpers.hpp"

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;
using Side = TriangleSide;
using Point = TrianglePoint;

describe<PointPairFlip>("PointPairFlip::make")([] {    
    mark_it("k_side_ab from a, b", [] {
        auto flip = PointPairFlip::make(Point::a, Point::b);
        return test_that(flip.side() == Side::k_side_ab &&
                         !flip.parameters_flipped());
    }).
    mark_it("k_side_ab from b, a", [] {
        auto flip = PointPairFlip::make(Point::b, Point::a);
        return test_that(flip.side() == Side::k_side_ab &&
                         flip.parameters_flipped());
    }).
    mark_it("k_side_bc from c, b", [] {
        auto flip = PointPairFlip::make(Point::c, Point::b);
        return test_that(flip.side() == Side::k_side_bc &&
                         flip.parameters_flipped());
    }).
    mark_it("k_side_ca from a, c", [] {
        // is this correct for triangle side ca?
        auto flip = PointPairFlip::make(Point::a, Point::c);
        return test_that(flip.side() == Side::k_side_ca &&
                         flip.parameters_flipped());
    }).
    mark_it("k_side_ca from c, a", [] {
        auto flip = PointPairFlip::make(Point::c, Point::a);
        return test_that(flip.side() == Side::k_side_ca &&
                         !flip.parameters_flipped());
    });
});

describe<PointMatchFinder>("PointMatchFinder::from_left_point")([] {
    mark_it("from b find lhs ab attached to rhs ca", [&] {
        TriangleSegment lhs
            {Vector{}, Vector{1, 0, 0}, Vector{1, 0, 1}};
        TriangleSegment rhs
            {Vector{}, Vector{1, 0, -1}, Vector{1, 0, 0}};
        auto finder = PointMatchFinder::from_left_point<Point::b>(lhs, rhs);
        auto match = finder();

        if (!match) return test_that(false);
        return test_that(match->left_point () == Point::b &&
                         match->right_point() == Point::c);
    }).
    mark_it("finds no match if starting point a, "
            "does not match any point on right", []
    {
        // start from a
        TriangleSegment lhs
            {Vector{}, Vector{1, 0, 0}, Vector{1, 0, 1}};
        // onto a triangle that *does* link
        TriangleSegment rhs
            {Vector{5, 0, 5}, Vector{1, 0, 1}, Vector{1, 0, 0}};
        auto finder = PointMatchFinder::from_left_point<Point::a>(lhs, rhs);
        return test_that(!finder().has_value());
    });
});

describe<SideToSideMapping>("SideToSideMapping::from_matches")([] {
    mark_it("accurately finds when points are flipped", [] {
        PointMatch a_match{Vector{       }, Point::a,
                           Vector{       }, Point::b};
        PointMatch b_match{Vector{1, 0, 0}, Point::b,
                           Vector{1, 0, 0}, Point::a};
        auto mapping = SideToSideMapping::from_matches(a_match, b_match);
        return test_that(mapping.sides_flip());
    }).
    mark_it("finds side of lhs triangle", [] {
        PointMatch a_match{Vector{       }, Point::a,
                           Vector{       }, Point::c};
        PointMatch b_match{Vector{1, 0, 0}, Point::b,
                           Vector{1, 0, 0}, Point::a};
        auto mapping = SideToSideMapping::from_matches(a_match, b_match);
        return test_that(mapping.left_side() == Side::k_side_ab);
    }).
    mark_it("finds side of rhs triangle", [] {
        PointMatch a_match{Vector{       }, Point::a,
                           Vector{       }, Point::c};
        PointMatch b_match{Vector{1, 0, 0}, Point::b,
                           Vector{1, 0, 0}, Point::a};
        auto mapping = SideToSideMapping::from_matches(a_match, b_match);
        return test_that(mapping.right_side() == Side::k_side_ca);
    });
});

describe<PointMatchAdder>("PointMatchAdder::add")([] {
    PointMatch a_match{Vector{       }, Point::a,
                       Vector{       }, Point::b};
    PointMatch b_match{Vector{1, 0, 0}, Point::b,
                       Vector{1, 0, 0}, Point::a};
    PointMatch c_match{Vector{0, 0, 1}, Point::c,
                       Vector{0, 0, 1}, Point::b};
    mark_it("adding only one, does not finish a side to side matching", [&] {
        return test_that(!PointMatchAdder{}.add(a_match).finish().has_value());
    }).
    mark_it("adding three, does not finish a side to side matching", [&] {
        return test_that(!PointMatchAdder{}.
            add(a_match).
            add(b_match).
            add(c_match).
            finish().has_value());
    }).
    mark_it("adding two, produces a valid side to side matching", [&] {
        auto side_to_side = *PointMatchAdder{}.
            add(a_match).
            add(b_match).
            finish();
        return test_that(side_to_side.left_side () == Side::k_side_ab &&
                         side_to_side.right_side() == Side::k_side_ab &&
                         side_to_side.sides_flip());
    });
});

describe<PointMatchAdder>("PointMatchAdder::find_point_match")([] {
    //
    // left side: ab
    //   a: -0.5, 3, -18.5
    //   b:  0.5, 3, -19.5
    //   c:  0.5, 3, -18.5
    // right side: ca
    //   a: -0.5, 3, -18.5
    //   b: -0.5, 3, -19.5
    //   c:  0.5, 3, -19.5
    // a demo time scraped test
    mark_it("finds corrects side for matching points", [] {
        TriangleSegment lhs{Vector{-0.5, 3, -18.5},
                            Vector{ 0.5, 3, -19.5},
                            Vector{ 0.5, 3, -18.5}};
        TriangleSegment rhs{Vector{-0.5, 3, -18.5},
                            Vector{-0.5, 3, -19.5},
                            Vector{ 0.5, 3, -19.5}};
        auto res = PointMatchAdder::find_point_match(lhs, rhs);
        if (!res) return test_that(false);
        return test_that(res->left_side () == Side::k_side_ab &&
                         res->right_side() == Side::k_side_ca   );
    });
});

return [] {};

} ();
