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

#include "../../src/TasksController.hpp"

#include "../../src/map-director/RegionLoadRequest.hpp"
#include "../../src/TriangleSegment.hpp"

#include "../test-helpers.hpp"

Real perimeter_of(const TriangleSegment & segment) {
    return magnitude(segment.point_a() - segment.point_b()) +
           magnitude(segment.point_b() - segment.point_c()) +
           magnitude(segment.point_c() - segment.point_a());
}

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;
describe<RectanglePoints>("RectanglePoints")([] {
    RectanglePoints points{cul::Rectangle<Real>{ 1, 2, 2, 3 }};
    mark_it("assigns correct top left", [points] {
        return test_that(are_very_close(points.top_left(), Vector2{1, 2}));
    }).
    mark_it("assigns correct top right", [points] {
        return test_that(are_very_close(points.top_right(), Vector2{3, 2}));
    }).
    mark_it("assigns correct bottom left", [points] {
        return test_that(are_very_close(points.bottom_left(), Vector2{1, 5}));
    }).
    mark_it("assigns correct bottom right", [points] {
        return test_that(are_very_close(points.bottom_right(), Vector2{3, 5}));
    });
});

struct RegionLoadRequestFindTriangle final {};

describe<RegionLoadRequestFindTriangle>("RegionLoadRequest::find_triangle")([] {
    using RlRequest = RegionLoadRequest;
    mark_it("has good values, for zero velocity and no facing", [] {
        auto segment = RlRequest::find_triangle(Vector{}, {}, Vector{});
        return test_that
            (are_very_close(segment.point_a(), Vector{-4, 0,  0}) &&
             are_very_close(segment.point_b(), Vector{ 6, 0, -8}) &&
             are_very_close(segment.point_c(), Vector{ 6, 0,  8})   );
    }).
    mark_it("has same area, regardless of velocity", [] {
        auto segment_a = RlRequest::find_triangle(Vector{}, {}, Vector{2, 0, 0});
        auto segment_b = RlRequest::find_triangle(Vector{}, {}, Vector{6, 0, 0});
        return test_that(are_very_close(segment_a.area(), segment_b.area()));
    }).
    mark_it("has same area, regardless of position", [] {
        auto segment_a = RlRequest::find_triangle(Vector{}, {}, Vector{});
        auto segment_b = RlRequest::find_triangle(Vector{100, 5, 100}, {}, Vector{});
        return test_that(are_very_close(segment_a.area(), segment_b.area()));
    }).
    mark_it("has same area, even if no facing is passed", [] {
        auto segment_a = RlRequest::find_triangle(Vector{}, {}, Vector{});
        auto segment_b = RlRequest::find_triangle(Vector{}, Vector{0, 0, -1}, Vector{});

        return test_that(are_very_close(segment_a.area(), segment_b.area()));
    }).
    mark_it("gets longer, the higher the velocity", [] {
        // maybe based on the wrong thing?
        auto segment_a = RlRequest::find_triangle(Vector{}, {}, Vector{2, 0, 0});
        auto segment_b = RlRequest::find_triangle(Vector{}, {}, Vector{6, 0, 0});
        return test_that(perimeter_of(segment_a) < perimeter_of(segment_b));
    });
});

return [] {};
} ();
