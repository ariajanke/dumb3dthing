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

#include "../../../src/map-director/twist-loop-filler/TwistLoopGroupFiller.hpp"

#include "../../test-helpers.hpp"

using namespace cul::tree_ts;

[[maybe_unused]] static auto s_add_describes = [] {

// I need an assortment of test data that hits every branch

describe<TwistyTileRadiiLimits>
    ("TwistyTileRadiiLimits ::low_high_x_edges_and_low_dir")([]
{

});

describe<TwistyTileRadiiLimits>
    ("TwistyTileRadiiLimits ::spine_and_edge_side_x")([]
{
    using std::get;
    auto spine_and_edge_side_x = []
        (Real twisty_width, Real tile_pos_x)
    {
        return TwistyTileRadiiLimits::spine_and_edge_side_x
            (twisty_width, tile_pos_x);
    };

    mark_it("computes tile bounds on the spine side away from origin", [&] {
        auto spine = get<0>(spine_and_edge_side_x(4, 3));
        return test_that(are_very_close(spine, 2.5));
    }).mark_it("computes tile bounds on the edge side away from origin", [&] {
        auto edge = get<1>(spine_and_edge_side_x(4, 3));
        return test_that(are_very_close(edge, 3.5));
    }).mark_it("computes tile bounds on the spine side close to origin", [&] {
        auto spine = get<0>(spine_and_edge_side_x(3, 0));
        return test_that(are_very_close(spine, 0.5));
    }).mark_it("computes tile bounds on the edge side close to origin", [&] {
        auto edge = get<1>(spine_and_edge_side_x(3, 0));
        return test_that(are_very_close(edge, -0.5));
    })
    // at the center the spine is treated as if it were the "high" side
    // need to make sure this behavior is consistent across classes/functions
    .mark_it("computes tile bounds on the spine side at spine", [&] {
        auto spine = get<0>(spine_and_edge_side_x(5, 2));
        return test_that(are_very_close(spine, 2.5));
    }).mark_it("computes tile bounds on the edge side at spine", [&] {
        auto edge = get<1>(spine_and_edge_side_x(5, 2));
        return test_that(are_very_close(edge, 1.5));
    });
});
#if 0
describe<TwistyTileRadiiLimits>("TwistyTileRadiiLimits")([] {

});

describe<TwistyTileTValueLimits>("TwistyTileTValueLimits").
    depends_on<TwistyTileRadiiLimits>()([]
{

});

describe<TwistyTilePointLimits>("TwistyTilePointLimits").
    depends_on<TwistyTileRadiiLimits>()([]
{

});
#endif
class Dummy final {};
return Dummy{};

} ();
