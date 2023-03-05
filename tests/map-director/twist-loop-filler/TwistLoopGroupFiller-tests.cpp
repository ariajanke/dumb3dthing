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
#if 0
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
#endif
struct find_unavoidable_t_breaks_for_twisty_test final {};

describe<find_unavoidable_t_breaks_for_twisty_test>
    ("find_unavoidable_t_breaks_for_twisty")
    ([]//.depends_on<TwistyTileTValueLimits>()([]
{
    mark_it("is a minimum of two t breaks for the smallest possible twisty", [] {
        auto res = find_unavoidable_t_breaks_for_twisty(Size2I{1, 1});
        return test_that(res.size() == 2);
    });
});

struct spine_and_edge_x_offsets_test final {};

describe<spine_and_edge_x_offsets_test>
    ("TwistyStripSpineOffsets::spine_and_edge_x_offsets")([]
{
    using Offsets = TwistyStripSpineOffsets;
    mark_it("is spine origin and half tile width,"
            "if strip is right on the spine", []
    {
        auto [spine, edge] = Offsets::spine_and_edge_x_offsets(5, 2);
        return test_that(are_very_close(0, spine) && are_very_close(0.5, edge));
    })
    .mark_it("is spine and edge offsets, below the spine", []
    {
        ;
    });
});

describe<TwistyStripSpineOffsets>("TwistyStripSpineOffsets").
    depends_on<spine_and_edge_x_offsets_test>()([]
{
    ;
});

describe<TwistyStripRadii>("TwistyTileRadiiLimits").
    depends_on<TwistyStripSpineOffsets>()([]
{
    //mark_it("has spine rad 1, edge rad 2 ")
    mark_it("", [] { return test_that(true); });
});

describe<TwistyTileTValueLimits>("TwistyTileTValueLimits").
    depends_on<TwistyStripRadii>()([]
{
    static auto find_lims = [] (const Size2I & sz, const Vector2I & r)
        { return *TwistyTileTValueLimits::find(sz, r); };

    mark_it("has bounds [0 1] for 1x1 twisties", [] {
        auto lims = find_lims(Size2I{1, 1}, Vector2I{});
        return test_that(   lims.low_t_limit () == 0
                         && lims.high_t_limit() == 1);
    }).
    mark_it("has bounds [0 1] for 2x4 twisty at a collapse point", [] {
        auto lims = find_lims(Size2I{2, 4}, Vector2I{});
        return test_that(   lims.low_t_limit () == 0
                         && lims.high_t_limit() == 1);
    }).
    mark_it("has bound [k k] for 16x4, at (3, 3)", [] {
        auto lims = find_lims(Size2I{16, 4}, Vector2I{3, 3});
        return test_that
            (are_very_close(lims.low_t_limit(), lims.high_t_limit()));
    }).
    mark_it("has bounds [1/8 1/6), at (8, 4) for size 12x24", [] {
        auto lims = find_lims(Size2I{12, 24}, Vector2I{8, 4});
        return test_that(   are_very_close(lims.low_t_limit(), 1. / 8.)
                         && lims.high_t_limit() < 1. / 6.
                         && !are_very_close(lims.high_t_limit(), 1. / 6.));
    });
    ;
});
#if 0
describe<TwistyTilePointLimits>("TwistyTilePointLimits").
    depends_on<TwistyTileRadiiLimits>()([]
{

});
#endif

return [] {};

} ();
