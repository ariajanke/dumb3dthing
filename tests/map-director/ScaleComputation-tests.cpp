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

#include "../../src/map-director/ScaleComputation.hpp"

#include "../test-helpers.hpp"

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe("ScaleComputation::parse")([] {
    mark_it("nullptr argument fails to parse", [] {
        return test_that(!ScaleComputation::parse(nullptr).has_value());
    }).
    mark_it("too few arguments", [] {
        return test_that(!ScaleComputation::parse("").has_value());
    }).
    mark_it("too many arguments", [] {
        return test_that(!ScaleComputation::parse("1,1,1,1").has_value());
    }).
    mark_it("one argument", [] {
        auto res = ScaleComputation::parse("6");
        return test_that(res.has_value() &&
                         *res == ScaleComputation{6, 6, 6});
    }).
    mark_it("three arguments", [] {
        auto res = ScaleComputation::parse("1 , 2 ,  3");
        return test_that(res.has_value() &&
                         *res == ScaleComputation{1, 2, 3});
    }).
    mark_it("three arguments, not numeric fails to parse", [] {
        return test_that(!ScaleComputation::parse("a,a,a").has_value());
    });
});

return [] {};

} ();
