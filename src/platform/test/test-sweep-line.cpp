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

#include "../../SweepLine.hpp"

#include <ariajanke/cul/TestSuite.hpp>

namespace {

using Triangle = TriangleSegment;
using Interval = SweepLine::Interval;

const Triangle & pass_triangle(const Triangle & t) { return t; }

} // end of <anonymous> namespace

bool run_sweep_line_tests() {
    using namespace cul::ts;
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    // tests...
    TestSuite suite;
    suite.start_series("Line Sweep");
    set_context(suite, [](TestSuite & suite, Unit & unit) {
        // sample becomes a point on the sweep line
        Triangle sample;
        SweepLineMap<Triangle> test_map;
        test_map.push_entry(sample);
        auto line = SweepLine::make_for(test_map, pass_triangle);
        test_map.update_as_triangles(line, pass_triangle);
        test_map.sort();

        unit.start(mark(suite), [&] {
            auto intv = line.interval_for(sample);
            return test(are_very_close(intv.max, intv.min));
        });
        auto low = line.point_for(Vector{});
        auto high = line.point_for(Vector{0.5, 0.5, 0.5});
        unit.start(mark(suite), [&] {
            auto view = test_map.view_for(low, high);
            return test((view.end() - view.begin()) > 0);
        });
        unit.start(mark(suite), [&] {
            auto view = test_map.view_for(low, high);
            if (view.begin() == view.end()) return test(false);
            auto tri = *view.begin();
            return test(are_very_close(sample.point_a(), tri.point_a()));
        });
    });
    set_context(suite, [](TestSuite & suite, Unit & unit) {
        // sample becomes a point on the sweep line
        Triangle sample;
        Triangle next_sample{Vector{}, Vector{1, 1, 1}, Vector{1, 0, 1}};
        SweepLineMap<Triangle> test_map;
        test_map.push_entry(sample);
        test_map.push_entry(next_sample);
        auto line = SweepLine::make_for(test_map, pass_triangle);
        test_map.update_as_triangles(line, pass_triangle);
        test_map.sort();

        // next... let's try a point interval, with a non-point triangle
        unit.start(mark(suite), [&] {
            auto intv = line.interval_for(next_sample);
            return test(!are_very_close(intv.max, intv.min));
        });
        unit.start(mark(suite), [&] {
            auto x = line.point_for(Vector{0.5, 0.5, 0.5});
            auto view = test_map.view_for(x);
            return test((view.end() - view.begin()) > 0);
        });



    });
    // lastly, expect a point interval, to interact with a point triangle
#   undef mark
    return suite.has_successes_only();
}
