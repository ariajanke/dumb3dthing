/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "test-functions.hpp"

#include "../../SpatialPartitionMap.hpp"

#include <ariajanke/cul/TestSuite.hpp>

namespace {

using Triangle = TriangleSegment;
using Interval = ProjectionLine::Interval;

struct Sample final {
public:
    Sample() {}
    explicit Sample(int i_): value(i_) {}

    int value = 0;
};

} // end of <anonymous> namespace

bool run_spm_tests() {
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    using namespace cul::ts;
    TestSuite suite;
    suite.start_series("ProjectionLine");
    mark(suite).test([] {
        ProjectionLine line{Vector{0, 0, 0}, Vector{1, 0, 0}};
        Triangle triangle{Vector{}, Vector{0.5, 0, 0}, Vector{0.25, 0.5, 0}};
        auto interval = line.interval_for(triangle);
        return test(   are_very_close(interval.min, 0)
                    && are_very_close(interval.max, 0.5));
    });
    suite.start_series("SpatialDivisionContainer");
    {
    using SamplePopulator = SpatialDivisionPopulator<int>;
    using SampleDivisions = SpatialDivisionContainer<int>;
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        SamplePopulator populator{ {
            make_tuple(0.   , 0),
            make_tuple(0.33 , 1),
            make_tuple(0.67 , 3),
            make_tuple(k_inf, 5)
        } };
        SampleDivisions divs{std::move(populator)};

        unit.start(mark(suite), [&] {
            Interval interval{0.4, 0.6};
            auto [low, high] = divs.pair_for(interval);
            return test(low == 1 && high == 3);
        });
        unit.start(mark(suite), [&] {
            Interval interval{0.0, 1.};
            auto [low, high] = divs.pair_for(interval);
            return test(low == 0 && high == 5);
        });
        unit.start(mark(suite), [&] {
            Interval interval{-k_inf, k_inf};
            auto [low, high] = divs.pair_for(interval);
            return test(low == 0 && high == 5);
        });
        unit.start(mark(suite), [&] {
            Interval interval{0.1, 0.15};
            auto [low, high] = divs.pair_for(interval);
            return test(low == 0 && high == 1);
        });
        unit.start(mark(suite), [&] {
            Interval interval{0.34, 0.68};
            auto [low, high] = divs.pair_for(interval);
            return test(low == 1 && high == 5);
        });
    });

    }
    suite.start_series("SpatialPartitionMapHelpers");
    {
    using Helpers = SpatialPartitionMapHelpers<int>;
    using Entry = Helpers::Entry;
    using EntryContainer = Helpers::EntryContainer;
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        Entry a{0.  , 0.25, 0};
        Entry b{0.2 , 0.45, 1};
        Entry c{0.3 , 0.55, 2};
        Entry d{0.5 , 0.6 , 3};
        Entry e{0.55, 0.65, 4};
        EntryContainer container{a, b, c, d, e};
        unit.start(mark(suite), [&] {
            auto view = Helpers::view_for_entries(container.begin(), container.end(), 0.29, 0.4);
            auto beg_idx = view.begin() - container.begin();
            auto end_idx = view.end  () - container.begin();
            return test(beg_idx == 1 && end_idx == 3);
        });
        unit.start(mark(suite), [&] {
            auto view = Helpers::view_for_entries(container.begin(), container.end(), -0.1, 0.1);
            auto beg_idx = view.begin() - container.begin();
            auto end_idx = view.end  () - container.begin();
            return test(beg_idx == 0 && end_idx == 1);
        });
        unit.start(mark(suite), [&] {
            auto view = Helpers::view_for_entries(container.begin(), container.end(), 0.56, k_inf);
            auto beg_idx = view.begin() - container.begin();
            auto end_idx = view.end  () - container.begin();
            return test(beg_idx == 3 && end_idx == 5);
        });
    });
    }
    suite.start_series("SpatialPartitionMap");
    suite.start_series("ProjectedSpatialMap");

    return suite.has_successes_only();
#   undef mark
}
