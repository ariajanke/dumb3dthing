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

#include "../../src/point-and-plane/SpatialPartitionMap.hpp"

#include <ariajanke/cul/TreeTestSuite.hpp>

#define mark_it mark_source_position(__LINE__, __FILE__).it

namespace {

using namespace cul::exceptions_abbr;
using Triangle = TriangleSegment;
using Interval = ProjectionLine::Interval;

template <typename ... Types>
SharedPtr<const TriangleLink> make_triangle_link(Types && ... args)
    { return make_shared<TriangleLink>(std::forward<Types>(args)...); }

auto make_finder(const SharedPtr<const TriangleLink> & link_ptr) {
    return [link_ptr](const WeakPtr<const TriangleLink> & wptr)
        { return link_ptr == wptr.lock(); };
}

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {
using namespace cul::tree_ts;

describe<ProjectionLine>("ProjectionLine")([] {
    mark_it("projects a triangle over the line", [] {
        ProjectionLine line{Vector{0, 0, 0}, Vector{1, 0, 0}};
        Triangle triangle{Vector{}, Vector{0.5, 0, 0}, Vector{0.25, 0.5, 0}};
        auto interval = line.interval_for(triangle);
        return test_that(   are_very_close(interval.min, 0)
                         && are_very_close(interval.max, 0.5));
    });
});

describe<SpatialDivisionContainer<int>>("SpatialDivisionContainer").
    depends_on<ProjectionLine>()([]
{
    using SamplePopulator = SpatialDivisionPopulator<int>;
    using SampleDivisions = SpatialDivisionContainer<int>;
    SamplePopulator populator{ std::vector<Tuple<Real, int>> {
        make_tuple(0.   , 0),
        make_tuple(0.33 , 1),
        make_tuple(0.67 , 3),
        make_tuple(k_inf, 5)
    } };
    SampleDivisions divs{std::move(populator)};
    mark_it("provides correct mid interval", [&] {
        Interval interval{0.4, 0.6};
        auto [low, high] = divs.pair_for(interval);
        return test_that(low == 1 && high == 3);
    });
    mark_it("provides correct early interval", [&] {
        Interval interval{0.0, 1.};
        auto [low, high] = divs.pair_for(interval);
        return test_that(low == 0 && high == 5);
    });
    mark_it("provides a low, high for an interval spanning all reals", [&] {
        Interval interval{-k_inf, k_inf};
        auto [low, high] = divs.pair_for(interval);
        return test_that(low == 0 && high == 5);
    });
    mark_it("provides a low, high contained in first two", [&] {
        Interval interval{0.1, 0.15};
        auto [low, high] = divs.pair_for(interval);
        return test_that(low == 0 && high == 1);
    });
    mark_it("provides a low, high contained all but first", [&] {
        Interval interval{0.34, 0.68};
        auto [low, high] = divs.pair_for(interval);
        return test_that(low == 1 && high == 5);
    });
});
describe<SpatialPartitionMapHelpers<int>>
    ("SpatialPartitionMapHelpers ::view_for_entries").
    depends_on<SpatialDivisionContainer<int>>()([]
{
    using Helpers = SpatialPartitionMapHelpers<int>;
    using Entry = Helpers::Entry;
    using EntryContainer = Helpers::EntryContainer;

    Entry a{0.  , 0.25, 0};
    Entry b{0.2 , 0.45, 1};
    Entry c{0.3 , 0.55, 2};
    Entry d{0.5 , 0.6 , 3};
    Entry e{0.55, 0.65, 4};
    EntryContainer container{a, b, c, d, e};

    mark_it("generates view not containing first element", [&] {
        auto view = Helpers::view_for_entries
            (container.begin(), container.end(), 0.29, 0.4);
        auto beg_idx = view.begin() - container.begin();
        auto end_idx = view.end  () - container.begin();
        return test_that(beg_idx == 1 && end_idx == 3);
    });
    mark_it("generates a valid view with a start before first element", [&] {
        auto view = Helpers::view_for_entries
            (container.begin(), container.end(), -0.1, 0.1);
        auto beg_idx = view.begin() - container.begin();
        auto end_idx = view.end  () - container.begin();
        return test_that(beg_idx == 0 && end_idx == 1);
    });
    mark_it("generates a valid view with an end beyond the last element", [&] {
        auto view = Helpers::view_for_entries
            (container.begin(), container.end(), 0.56, k_inf);
        auto beg_idx = view.begin() - container.begin();
        auto end_idx = view.end  () - container.begin();
        return test_that(beg_idx == 3 && end_idx == 5);
    });
    // | this failing to stop further tests indicates a problem with this
    // v "framework"
    // mark_it("fuck", [] { return test_that(false); });
});
describe<SpatialPartitionMap>("SpatialPartitionMap").
    depends_on<SpatialPartitionMapHelpers<int>>()([]
{
    using Entry = SpatialPartitionMap::Entry;
    using EntryContainer = SpatialPartitionMap::EntryContainer;

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
    // must have at least one of b, and c
    auto mid_view = container.view_for(Interval{0.29, 0.4});
    // d and e
    auto high_view = container.view_for(Interval{0.56, k_inf});

    mark_it("view for finding b link contains b", [&] {
        auto res = std::find_if
            (mid_view.begin(), mid_view.end(), make_finder(b_link));
        return test_that(res != mid_view.end());
    });
    mark_it("view for finding c link contains c", [&] {
        auto res = std::find_if
            (mid_view.begin(), mid_view.end(), make_finder(c_link));
        return test_that(res != mid_view.end());
    });

    mark_it("view for finding d link contains d", [&] {
        auto res = std::find_if
            (high_view.begin(), high_view.end(), make_finder(d_link));
        return test_that(res != high_view.end());
    });
    mark_it("view for finding e link contains e", [&] {
        auto res = std::find_if
            (high_view.begin(), high_view.end(), make_finder(e_link));
        return test_that(res != high_view.end());
    });
    mark_it("can find b link using a single point interval", [&] {
        auto view = container.view_for(Interval{0.23, 0.23});
        auto res = std::find_if
            (view.begin(), view.end(), make_finder(b_link));
        return test_that(res != view.end());
    });
});
describe<ProjectedSpatialMap>("ProjectedSpatialMap").
    depends_on<SpatialPartitionMap>() ([]
{
    auto a_link = make_triangle_link(Vector{0, 0, 0}, Vector{1, 0, 0}, Vector{1, 1, 0});
    auto b_link = make_triangle_link(Vector{1, 0, 0}, Vector{2, 0, 0}, Vector{2, 1, 0});
    auto c_link = make_triangle_link(Vector{2, 0, 0}, Vector{3, 0, 0}, Vector{3, 1, 0});
    auto d_link = make_triangle_link(Vector{3, 0, 0}, Vector{4, 0, 0}, Vector{4, 1, 0});
    auto e_link = make_triangle_link(Vector{4, 0, 0}, Vector{5, 0, 0}, Vector{5, 1, 0});
    std::vector link_container{a_link, b_link, c_link, d_link, e_link};
    ProjectedSpatialMap psm{ link_container };
    mark_it("finds a triangle with a normal finite interval", [&] {
        auto view = psm.view_for(Vector{1.5, 0, 0}, Vector{2.5, 0, 0});
        auto res = std::find_if(view.begin(), view.end(), make_finder(c_link));
        return test_that(res != view.end());
    });
    mark_it("finds a triangle with an infintesimal interval", [&] {
         auto view = psm.view_for(Vector{1.5, 0, 0}, Vector{1.5, 0, 0});
        auto res = std::find_if(view.begin(), view.end(), make_finder(b_link));
        return test_that(res != view.end());
    });
});
return 1;
} ();
