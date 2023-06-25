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

#include "../src/geometric-utilities.hpp"
#include "test-helpers.hpp"

#include <ariajanke/cul/TreeTestSuite.hpp>

Vector2 force_rt_zero_vec() {
    volatile Real * ip = nullptr;
    Vector2 rv;
    ip = &rv.x;
    *ip = 0;
    return rv;
}

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe<LineSegmentIntersection<Vector>>("LineSegmentIntersection::find")([] {
    // test line segments:
    // diagonal
    // x   x
    //  . .
    //   o
    //  . .
    // x   x
    // not sure about static assertions... could try with integers I suppose

    static_assert
        (::find_intersection(Vector2I{}, Vector2I{3, 3},
                             Vector2I{0, 3}, Vector2I{3, 0}).has_value());

    static_assert
        (::find_intersection(Vector2I{}, Vector2I{4, 4},
                             Vector2I{0, 4}, Vector2I{4, 0}).value() == Vector2I{2, 2});

    mark_it("finds intersection for two line segments", [] {

        auto finder = ::find_intersection(force_rt_zero_vec(), Vector2{4, 4},
                                          Vector2{0, 4}, Vector2{4, 0});
        static_assert(std::is_same_v
                          <LineSegmentIntersection<Vector2>::VectorType,
                           Vector2>);
        auto intx = finder.value();
        return test_that(are_very_close(intx, Vector2{2, 2}));
    });
    // there maybe four additional tests
    // intx outside one segment and the other
    // parallel line segments that overlap and do not
});

return [] {};
} ();
