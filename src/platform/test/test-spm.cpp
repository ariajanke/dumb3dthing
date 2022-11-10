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
    mark(suite).test([] {
        SamplePopulator populator{ {
            make_tuple(0.   , 0),
            make_tuple(0.33 , 1),
            make_tuple(0.67 , 3),
            make_tuple(k_inf, 5)
        } };
        SampleDivisions divs{std::move(populator)};
        Interval interval{0.4, 0.6};
        auto [low, high] = divs.pair_for(interval);
        return test(low == 1 && high == 5);
    });
    }
    suite.start_series("SpatialPartitionMapHelpers");
    suite.start_series("SpatialPartitionMap");
    suite.start_series("ProjectedSpatialMap");

    return suite.has_successes_only();
#   undef mark
}
