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

#include "../../src/map-director/RegionPositionFraming.hpp"
#include "../../src/map-director/RegionLoadRequest.hpp"

#include "../test-helpers.hpp"

#include <set>
#include <unordered_set>

// tiled map
//   has a scale
// what is a tile?
// - actual geometry and visuals
//   - scaled by map alone
//   - positioned by map and framing device
// - map (sub) region
//   - scaled by map
//     - its geometry and visuals scaled by itself AND framing device
// positioning
// scale compounding
//
// On scaling?
//
// example (a)
// composite map scale factor of 4
// - has a map region scale factor of 2
//
// What is the scaling factor of the geometry?
//
// Each loaded in map sub region is a tile, therefore considered to be 4x4
// tiles.
// If the map region scale factor is 2. Each map region then would be 2x2.
// We can take the composite map region scaling factor and "divide" it by the
// local, map sub region. We would then get a correct scaling factor.
//
// example (b)
// composite map scale factor of 6
// - has a map region scale factor of 2
//
// Therefore we would like a set of 3x3 tiles of geometry/visuals in the map
// sub region.
//
// Additional conclusion
// - geometry/visuals ONLY consider the most local scaling factor
//
// On positioning?
//
// example (c)
// Map Region is placed at (1, 3)
// - has a map sub region, placed at tile position (2, 5)
//
// Where does the map sub region tiles start?
// It would be additive(?), therefore starts at (3, 8)
//
// On positioning AND scaling?
//
// example (d)
// Map Region is placed at (1, 3)
// - has a scaling factor of 6
// - has a map sub region, placed at tile position (2, 5)
//   - map sub region itself has a scale factor of 2
//
// What is both is scaling factor and starting position for the geometry and
// visuals on the map sub region?
//
// On positioning, start at (1, 3), moving to tile position (2, 5), with
// scaling factor 6. Each "tile" for the composite map is "6" units.
// So we start at position (1, 3) + (2, 5)*6 = (1, 3) + (12, 30) = (13, 33)
//
// On scaling, each map sub region "tile" is 2 units.
// Each composite map tile is 3x3 tiles of map sub region.
//
// example (e)
// From example (d), how do we find the starting position for geometry/visuals
// from the perspective of the map sub region, at its (1, 2) position?
//
// We know we start generally from (13, 33). Its own scaling factor is 2. To
// move to position (1, 2) would be (13, 33) + 2*(1, 2) = (13, 33) + (2, 4) =
// (15, 37).
//
// Additional conclusion:
// Tests should reflect exactly this.

namespace {

class TestRegionLoadRequest final : public RegionLoadRequestBase {
public:
    static TestRegionLoadRequest & instance() {
        static TestRegionLoadRequest inst;
        return inst;
    }

    static void set_always_overlaps() { s_overlaps = true; }

    static void set_never_overlaps() { s_overlaps = false; }

    static void set_max_region_size(Size2I sz) { s_max_size = sz; }

    bool overlaps_with(const RectangleI &) const final { return s_overlaps; }

    Size2I max_region_size() const final { return s_max_size; }

private:
    static Size2I s_max_size;
    static bool s_overlaps;
};

struct RectHash final {
    std::size_t operator () (const RectangleI & rect) const {
        using IntHash = std::hash<int>;
        return IntHash{}(rect.height) ^ IntHash{}(rect.left ) ^
               IntHash{}(rect.top   ) ^ IntHash{}(rect.width);
    }
};

/* private static */ Size2I TestRegionLoadRequest::s_max_size{2, 1};
/* private static */ bool TestRegionLoadRequest::s_overlaps = true;
} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe<TilePositionFraming>("TilePositionFraming")([] {
    mark_it("#transform: scales then translates a triangle", [] {
        static constexpr const Vector v_a{1, 1, 1};
        TriangleSegment tri{v_a, Vector{}, Vector{1, 0, 0}};
        TilePositionFraming framing
            {ScaleComputation{2, 3, 4}, Vector2I{1, 2}};
        auto pa = framing.transform(tri).point_a();
        return test_that(are_very_close(
            Vector{v_a.x*2, v_a.y*3, v_a.z*4} + Vector{1, 0, -2}, pa));
    }).
    mark_it("#advance_with: advances inserter", []
    {
        ViewGridInserter<int> inst{1, 1};
        TilePositionFraming
            {ScaleComputation{2, 3, 4}, Vector2I{1, 2}}.
            advance_with(inst);
        return test_that(inst.filled());
    }).
    mark_it("#advance_with: reinstantiates with scaled position", [] {
        ViewGridInserter<int> inst{2, 1};
        auto framing = TilePositionFraming
            {ScaleComputation{2, 3, 4}, Vector2I{1, 2}}.
            advance_with(inst);
        return test_that(are_very_close(
            framing.model_translation().value,
            Vector{1, 0, -2} + Vector{2, 0, 0}));
    });
});

describe<RegionPositionFraming>("RegionPositionFraming#for_each_overlap_")([] {
    RegionPositionFraming framing
        {ScaleComputation{2, 0, 2},
         Vector2I{1, 2}};
    TestRegionLoadRequest request;
    SharedPtr<Size2I> region_size = make_shared<Size2I>();
    std::unordered_set<RectangleI, RectHash> expected_rects;
    auto remove_expected_rects = [&] {
        framing.for_each_overlap
            (*region_size,
             TestRegionLoadRequest::instance(),
             [&](const RegionPositionFraming &, const RectangleI & rect)
        {
            auto itr = expected_rects.find(rect);
            if (itr == expected_rects.end()) {
                throw RuntimeError{"found unexpected rectangle"};
            }
            expected_rects.erase(itr);
        });
    };
    mark_it("covers with sub regions that fit region load request", [&] {
        TestRegionLoadRequest::set_max_region_size(Size2I{2, 1});
        *region_size = Size2I{2, 2};
        expected_rects = { RectangleI{0, 0, 2, 1}, RectangleI{0, 1, 2, 1} };
        remove_expected_rects();
        return test_that(expected_rects.empty());
    }).
    mark_it("covers with sub regions that fit unevenly the region load request", [&] {
        TestRegionLoadRequest::set_max_region_size(Size2I{2, 2});
        *region_size = Size2I{5, 2};
        expected_rects = {
            RectangleI{0, 0, 2, 2},
            RectangleI{2, 0, 2, 2},
            RectangleI{4, 0, 1, 2},
        };
        remove_expected_rects();
        return test_that(expected_rects.empty());
    }).
    mark_it("covers nothing in load request never overlaps", [&] {
        TestRegionLoadRequest::set_max_region_size(Size2I{2, 1});
        *region_size = Size2I{2, 2};
        TestRegionLoadRequest::set_never_overlaps();
        remove_expected_rects();
        return test_that(expected_rects.empty());
    });
});

return [] {};

} ();
