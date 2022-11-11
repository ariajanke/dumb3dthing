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

template <typename ... Types>
SharedPtr<const TriangleLink> make_triangle_link(Types && ... args)
    { return make_shared<TriangleLink>(std::forward<Types>(args)...); }

auto make_finder (const SharedPtr<const TriangleLink> & link_ptr) {
    return [link_ptr](const WeakPtr<const TriangleLink> & wptr)
        { return link_ptr == wptr.lock(); };
}

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
    {
    using Entry = SpatialPartitionMap::Entry;
    using EntryContainer = SpatialPartitionMap::EntryContainer;
    set_context(suite, [] (TestSuite & suite, Unit & unit) {
        auto a_link = make_triangle_link();
        auto b_link = make_triangle_link();
        auto c_link = make_triangle_link();
        auto d_link = make_triangle_link();
        auto e_link = make_triangle_link();
        Entry a{0.  , 0.25, a_link};
        Entry b{0.2 , 0.45, b_link};
        Entry c{0.3 , 0.55, c_link};
        Entry d{0.5 , 0.6 , d_link};
        Entry e{0.55, 0.65, e_link};

        SpatialPartitionMap container{EntryContainer{a, b, c, d, e}};
        // make sure I can find all the links which overlap certain intervals
        // note: dupelicates are totally fine! It's the trade off we're making
        // must have at least one of b, c, and d
        auto mid_view = container.view_for(Interval{0.29, 0.4});
        // d and e
        auto high_view = container.view_for(Interval{0.56, k_inf});
        unit.start(mark(suite), [&] {
            auto res = std::find_if
                (mid_view.begin(), mid_view.end(), make_finder(b_link));
            return test(res != mid_view.end());
        });
        unit.start(mark(suite), [&] {
            auto res = std::find_if
                (mid_view.begin(), mid_view.end(), make_finder(c_link));
            return test(res != mid_view.end());
        });
        unit.start(mark(suite), [&] {
            auto res = std::find_if
                (mid_view.begin(), mid_view.end(), make_finder(d_link));
            return test(res != mid_view.end());
        });
        unit.start(mark(suite), [&] {
            auto res = std::find_if
                (high_view.begin(), high_view.end(), make_finder(d_link));
            return test(res != high_view.end());
        });
        unit.start(mark(suite), [&] {
            auto res = std::find_if
                (high_view.begin(), high_view.end(), make_finder(e_link));
            return test(res != high_view.end());
        });
    });
    }
    suite.start_series("ProjectedSpatialMap");

    return suite.has_successes_only();
#   undef mark
}
