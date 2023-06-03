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

#include "../test-helpers.hpp"

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
    mark_it("has good values, for zero velocity and no facing", [] {

        return test_that(false);
    }).
    mark_it("has same area, regardless of velocity", [] {
        return test_that(false);
    }).
    mark_it("has same area, regardless of position", [] {
        return test_that(false);
    }).
    mark_it("has same area, even if no facing is passed", [] {
        return test_that(false);
    }).
    mark_it("gets longer, the higher the velocity", [] {
        return test_that(false);
    });
});

return [] {};
} ();
