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
            (are_very_close(segment.point_a(), Vector{-4.5, 0,   0}) &&
             are_very_close(segment.point_b(), Vector{ 8  , 0, -10}) &&
             are_very_close(segment.point_c(), Vector{ 8  , 0,  10})   );
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
        auto segment_a_x_ways =
            magnitude(segment_a.point_a().x - segment_a.point_b().x);
        auto segment_b_x_ways =
            magnitude(segment_b.point_a().x - segment_b.point_b().x);
        return test_that(segment_a_x_ways < segment_b_x_ways);
    }).
    mark_it("handles different facing correctly", [] {
        using std::sqrt;
        const Real bc_dist_from_origin = sqrt(10*10 + 8*8);
        auto segment = RlRequest::find_triangle
            (Vector{}, normalize(Vector{-1, 0, -1}), Vector{});
        auto expected_point_a = Vector{4.5/sqrt(2.), 0., 4.5/sqrt(2.)};
        return test_that
            (are_very_close(segment.point_a(), expected_point_a) &&
             are_very_close(magnitude(segment.point_b()), bc_dist_from_origin) &&
             are_very_close(magnitude(segment.point_c()), bc_dist_from_origin)   );
    });
});

struct RegionLoadRequestOverlapsWithFieldRectangle final {};

// needs more thorough testing for overlaps
describe<RegionLoadRequestOverlapsWithFieldRectangle>
    ("RegionLoadRequest::overlaps_with_field_rectangle")([]
{
    using Rectangle = cul::Rectangle<Real>;
    mark_it("overlaps with triangle nested inside", [] {
        RegionLoadRequest request{Vector2{1, 1}, Vector2{2, 1}, Vector2{1, 2}, Size2I{}};
        return test_that(request.overlaps_with_field_rectangle(Rectangle{0, 0, 3, 3}));
    }).
    mark_it("overlaps with rectangle nested inside", [] {
        RegionLoadRequest request{Vector2{0, 0}, Vector2{6, 0}, Vector2{0, 6}, Size2I{}};
        return test_that(request.overlaps_with_field_rectangle(Rectangle{1, 1, 1, 1}));
    }).
    mark_it("overlaps with only intersections", [] {
        RegionLoadRequest request{Vector2{1, -1}, Vector2{-1, 1}, Vector2{3, 1}, Size2I{}};
        return test_that(request.overlaps_with_field_rectangle(Rectangle{0, 0, 2, 2}));
    }).
    mark_it("does not overlap otherwise", [] {
        RegionLoadRequest request{Vector2{-1, -1}, Vector2{0, -1}, Vector2{-1, 0}, Size2I{}};
        return test_that(!request.overlaps_with_field_rectangle(Rectangle{0, 0, 1, 1}));
    }).
    mark_it("checks against a forgotten (caught after) triangle line", [] {
        RegionLoadRequest request{Vector2{-0.265199661,  -2.96625638},
                                  Vector2{10.0311489  , -10.736001  },
                                  Vector2{10.0311489  ,   4.80348873}, Size2I{}};
        return test_that(request.overlaps_with_field_rectangle(Rectangle{0, 0, 10, 10}));
    });
});

struct RegionLoadRequestOverlapsWith final {};

describe<RegionLoadRequestOverlapsWith>("RegionLoadRequest::overlaps_with").
    depends_on<RegionLoadRequestOverlapsWithFieldRectangle>()([]
{
    mark_it("adjusts given rectangle to detect overlap", [] {
        RegionLoadRequest request{Vector2{0.4, 0.4},
                                  Vector2{1.4, 0.4},
                                  Vector2{0.4,   0}, Size2I{}};
        return test_that(request.overlaps_with(RectangleI{0, 0, 10, 10}));
    }).
    mark_it("adjusts given rectangle to detect non overlap", [] {
        RegionLoadRequest request{Vector2{0, 0.6},
                                  Vector2{1, 0.6},
                                  Vector2{1, 1  }, Size2I{}};
        return test_that(!request.overlaps_with(RectangleI{0, 0, 10, 10}));
    });
});

return [] {};
} ();
