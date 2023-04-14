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

#include "../../../src/TasksController.hpp"

#include "../../../src/map-director/twist-loop-filler/TwistLoopGroupFiller.hpp"

#include "../../test-helpers.hpp"

using namespace cul::tree_ts;

static auto make_grid_pred(const Grid<bool> & grid) {
    return [&grid] (const Vector2I & r) {
        if (grid.has_position(r))
            return grid(r);
        return false;
    };
}

[[maybe_unused]] static auto s_add_describes = [] {

return [] {}; // TODO re-enable

describe("get_rectangular_group_of")([] {

    mark_it("is 1x1 rectangle when only one point is true", [] {
        auto res = get_rectangular_group_of
            (Vector2I{}, [] (const Vector2I & r) { return r == Vector2I{}; });
        return test_that(res == RectangleI{0, 0, 1, 1});
    }).
    mark_it("is empty rectangle if start is false", [] {
        auto res = get_rectangular_group_of
            (Vector2I{}, [] (const Vector2I &) { return false; });
        return test_that(res.height == 0 && res.width == 0);
    }).
    mark_it("stops moving downward immediately on differently sized row", [] {
        Grid<bool> grid{
            { true , true , true  },
            { true , true , false },
            { false, false, false }
        };
        auto res = get_rectangular_group_of(Vector2I{}, make_grid_pred(grid));
        return test_that(res == RectangleI{0, 0, 3, 1});
    }).
    mark_it("includes another row of at least the same size", [] {
        Grid<bool> grid{
            { true , true , false },
            { true , true , true  },
            { true , false, false }
        };
        auto res = get_rectangular_group_of(Vector2I{}, make_grid_pred(grid));
        return test_that(res == RectangleI{0, 0, 2, 2});
    }).
    mark_it("include correct offset", [] {
        static constexpr const Vector2I k_start{1, 1};
        Grid<bool> grid{
            { true , true , false },
            { true , true , true  },
            { true , false, false }
        };
        auto res = get_rectangular_group_of(k_start, make_grid_pred(grid));
        return test_that(res.left == 1 && res.top == 1);
    });
});
#if 0
return [] {};
#endif
} ();
