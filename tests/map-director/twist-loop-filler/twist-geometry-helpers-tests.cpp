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

#include "../../../src/map-director/twist-loop-filler/twist-geometry-helpers.hpp"

#include "../../test-helpers.hpp"

using namespace cul::tree_ts;

[[maybe_unused]] static auto s_add_describes = [] {

return [] {}; // TODO re-enable

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
    }).
    mark_it("is negative spine and edge offsets, below the spine", [] {
        auto [spine, edge] = Offsets::spine_and_edge_x_offsets(6, 1);
        return test_that(are_very_close(-1, spine) && are_very_close(-2, edge));
    }).
    mark_it("is positive spine and edge offsets, above the spine", [] {
        auto [spine, edge] = Offsets::spine_and_edge_x_offsets(7, 4);
        return test_that(are_very_close(0.5, spine) && are_very_close(1.5, edge));
    }).
    mark_it("is spine 0, edge 1 for strip adjacent to the spine", [] {
        auto [spine, edge] = Offsets::spine_and_edge_x_offsets(4, 2);
        return test_that(are_very_close(0, spine) && are_very_close(1, edge));
    });
});

describe<TwistyStripSpineOffsets>("TwistyStripSpineOffsets::find").
    depends_on<spine_and_edge_x_offsets_test>()([]
{
    using Offsets = TwistyStripSpineOffsets;
    mark_it("is no solution if t_value is invalid", [] {
        return test_that(!Offsets::find(1, 0, 1.1).has_value());
    }).
    mark_it("is no solution if the spine offset is beyond the maximum bounds "
            "of the twisty", []
    {
        return test_that(!Offsets::find(3, 5, 0.05).has_value());
    }).
    mark_it("no solution if the spine offset is beyond the maximum bounds in "
            "the negative direction", []
    {
        return test_that(!Offsets::find(10, 0, 0.25).has_value());
    }).
    mark_it("is clipped edge if twisty bounds reaches out of the strip", [] {
        auto offsets = Offsets::find(4, 2, 0.1);
        auto edge = offsets->edge();
        return test_that(are_very_close(edge, 1));
    }).
    mark_it("is edge if twisty bounds are contained by the strip", [] {
        auto offsets = Offsets::find(3, 2, 0.1);
        auto edge = offsets->edge();
        return test_that(edge > 0.5 && edge < 1.5);
    }).
    mark_it("clips bounds in the negative direction", [] {
        auto offsets = Offsets::find(5, 1, 0.1);
        auto edge = offsets->edge();
        return test_that(are_very_close(edge, -1.5));
    });
});

describe<TwistyStripRadii>("TwistyStripRadii::find").
    depends_on<TwistyStripSpineOffsets>()([]
{
    using Radii = TwistyStripRadii;
    // very simple, it's just division
    mark_it("is not a solution if there are no given offsets", [] {
        return test_that( !Radii::find({}, 0).has_value() );
    }).
    mark_it("is expected radii", [] {
        auto radii = Radii::find(3, 2, 0.1);
        auto edge = radii->edge();
        auto spine = radii->spine();
        return test_that(   are_very_close( edge, 1.5 )
                         && spine < 1.0 && spine > 0.6  );
    });
});
#if 0
return [] {};
#endif
} ();

class F final {
public:
    static constexpr Real k_lower_alternative = -1094854.1234;
    static constexpr Real k_higher_alternative = 12345.241;

    class ReturnWhat final {
    public:
        ~ReturnWhat() {}

        static ReturnWhat lower()
            { return ReturnWhat{&returns_lower, nullptr}; }

        static ReturnWhat higher()
            { return ReturnWhat{nullptr, &returns_higher}; }

        static ReturnWhat both()
            { return ReturnWhat{&returns_lower, &returns_higher}; }

    private:
        ReturnWhat(bool * lower, bool * higher):
            m_lower(lower), m_higher(higher)
        {
            if (m_lower ) *m_lower  = true;
            if (m_higher) *m_higher = true;
        }

        bool * m_lower = nullptr;
        bool * m_higher = nullptr;
    };

    static Optional<Real> f
        (const Size2I &, int strip_x, const TwistyTileTValueRange &)
    {
        if (strip_x == 0 && returns_lower) {
            return k_lower_alternative;
        } else if (strip_x == 1 && returns_higher) {
            return k_higher_alternative;
        }
        return {};
    }

private:
    static bool returns_lower;
    static bool returns_higher;
};


bool F::returns_lower  = false;
bool F::returns_higher = false;

using TestFinder = TwistyTileTValueLimits::ClosestAlternateFinder<F::f>;

[[maybe_unused]] static auto s_add_describes1 = [] {

return [] {}; // TODO re-enable

describe
    <TwistyTileTValueLimits::ClosestAlternateFinder
        <TwistyTileTValueLimits::intersecting_t_value>>
    ("ClosestAlternateFinder")([]
{
    mark_it("is lower alternate if it is the only defined alternate", [] {
        auto _ = F::ReturnWhat::lower();
        return test_that
            (TestFinder{Size2I{}, Vector2I{}}(1.) == F::k_lower_alternative);
    }).
    mark_it("is higher alternate if it is the only defined alternate", [] {
        auto _ = F::ReturnWhat::higher();
        return test_that
            (TestFinder{Size2I{}, Vector2I{}}(1.) == F::k_higher_alternative);
    }).
    mark_it("is lower alternate if it is closer", [] {
        auto _ = F::ReturnWhat::both();
        auto value_closer_to_lower =
            F::k_lower_alternative +
            (F::k_lower_alternative + F::k_higher_alternative)*0.1;
        return test_that(TestFinder{Size2I{}, Vector2I{}}(value_closer_to_lower));
    }).
    mark_it("is higher alternate if it is closer", [] {
        auto _ = F::ReturnWhat::both();
        auto value_closer_to_lower =
            F::k_lower_alternative +
            (F::k_lower_alternative + F::k_higher_alternative)*0.9;
        return test_that(TestFinder{Size2I{}, Vector2I{}}(value_closer_to_lower));
    });
});

describe<TwistyTileTValueLimits>("TwistyTileTValueLimits").
    depends_on<TwistyStripRadii>()([]
{
    static auto find_lims = [] (const Size2I & sz, const Vector2I & r)
        { return *TwistyTileTValueLimits::find(sz, r); };

    mark_it("has bounds [0 1] for 1x1 twisties", [] {
        auto lims = find_lims(Size2I{1, 1}, Vector2I{});
        return test_that(   lims.low_t_limit () == 0
                         && are_very_close(lims.high_t_limit(), 1));
    }).
    mark_it("has bounds [0 1] for 2x4 twisty at a collapse point", [] {
        auto lims = find_lims(Size2I{2, 4}, Vector2I{});
        return test_that(   lims.low_t_limit () == 0
                         && are_very_close(lims.high_t_limit(), 0.25));
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
#if 0
return [] {};
#endif
} ();
