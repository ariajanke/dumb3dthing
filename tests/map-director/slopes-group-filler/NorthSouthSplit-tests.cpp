/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "../../../src/map-director/slopes-group-filler/NorthSouthSplit.hpp"
#include "../../test-helpers.hpp"

#include "SplitWallGeometry.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::tree_ts;
using AddedTriangle = TestStripTriangleCollection::AddedTriangle;
const NorthSouthSplit k_test_split{0, 0, 1, 1, 0.25};

auto x = [] {

describe("NorthSouthSplit#make_top")([] {
    auto & tri_col =
        TestStripTriangleCollection::instance() =
        TestStripTriangleCollection{};
    k_test_split.make_top(tri_col);
    mark_it("contains one top triangle", [&] {
        return test_that(tri_col.has_triangle_added(TriangleSegment{
            Vector{-0.5, 1, -0.5 },
            Vector{ 0.5, 1, -0.5 },
            Vector{-0.5, 1, -0.25}}));
    });
});

describe("NorthSouthSplit#make_bottom")([] {
    auto & tri_col =
        TestStripTriangleCollection::instance() =
        TestStripTriangleCollection{};
    k_test_split.make_bottom(tri_col);
    mark_it("contains one bottom triangle", [&] {
        return test_that(tri_col.has_triangle_added(TriangleSegment{
            Vector{-0.5, 0, -0.25},
            Vector{ 0.5, 0, -0.25},
            Vector{-0.5, 0,  0.5 }}));
    });
});

describe("NorthSouthSplit#make_wall")([] {
    auto & tri_col =
        TestStripTriangleCollection::instance() =
        TestStripTriangleCollection{};
    k_test_split.make_wall(tri_col);
    mark_it("contains one bottom triangle", [&] {
        return test_that(tri_col.has_triangle_added(TriangleSegment{
            Vector{-0.5, 0, -0.25},
            Vector{ 0.5, 0, -0.25},
            Vector{ 0.5, 1, -0.25}}));
    });
});

return [] {};
} ();

} // end of <anonymous> namespace
