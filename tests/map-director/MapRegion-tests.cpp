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

#include "../../src/map-director/MapRegion.hpp"
#include "../../src/TriangleLink.hpp"
#include "../../src/map-director/RegionLoadRequest.hpp"

#include "../test-helpers.hpp"

namespace {

struct ReceivedLoadRequest final {
    ReceivedLoadRequest() {}

    ReceivedLoadRequest
        (const RegionPositionFraming & framing_,
         const RectangleI & grid_scope_):
        framing(framing_),
        grid_scope(grid_scope_),
        hit(true) {}

    bool operator == (const ReceivedLoadRequest & rhs) const {
        return framing == rhs.framing && grid_scope == rhs.grid_scope &&
               hit == rhs.hit;
    }

    RegionPositionFraming framing;
    RectangleI grid_scope;
    bool hit = false;
};

struct TestMapRegion final : public MapRegion {
    TestMapRegion() {}

    explicit TestMapRegion(ReceivedLoadRequest & received_request):
        received(&received_request) {}

    void process_load_request
        (const RegionLoadRequest &,
         const RegionPositionFraming & framing,
         RegionLoadCollectorBase &,
         const Optional<RectangleI> & grid_scope) final
    { *received = ReceivedLoadRequest{framing, *grid_scope}; }

    ReceivedLoadRequest * received = nullptr;
};

class TestRegionLoadCollector final : public RegionLoadCollectorBase {
public:
    void collect_load_job
        (const SubRegionPositionFraming &, const ProducableSubGrid &) final {}
};

Vector2 to_field(Real x, Real y)
    { return Vector2{x, -y} + Vector2{-0.5, 0.5}; }

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;
using RectI = RectangleI;

// make sure that sub regions are hit as expected
// scale must propagate correctly
// - as you go down

describe<TiledMapRegion>("TiledMapRegion") ([] {

});

describe<CompositeMapRegion>("CompositeMapRegion")([] {
    ReceivedLoadRequest ne, nw, se, sw;
    CompositeMapRegion comp_map{
        Grid<MapSubRegion>{
            { MapSubRegion{RectI{0, 0, 2, 2}, make_shared<TestMapRegion>(nw)},
              MapSubRegion{RectI{0, 2, 2, 2}, make_shared<TestMapRegion>(ne)} },
            { MapSubRegion{RectI{2, 0, 2, 2}, make_shared<TestMapRegion>(sw)},
              MapSubRegion{RectI{2, 2, 2, 2}, make_shared<TestMapRegion>(se)} }
        },
        ScaleComputation{2, 1, 2}};
    const RegionLoadRequest request
        {to_field(0.1, 2.1), to_field(0.1, 3.9), to_field(2.1, 3), Size2I{1, 1}};
    const RegionPositionFraming framing;
    TestRegionLoadCollector test_collector;
    comp_map.process_load_request(request, framing, test_collector);
    mark_it("does not hit ne corner", [&ne] {
        return test_that(!ne.hit);
    }).
    mark_it("does not hit nw corner", [&nw] {
        return test_that(!nw.hit);
    }).
    mark_it("hits se corner", [&se] {
        return test_that(se.hit);
    }).
    mark_it("hits sw corner", [&sw] {
        return test_that(sw.hit);
    }).
    mark_it("se corner sends correct grid scope", [&se] {
        return test_that(se.grid_scope == RectI{2, 2, 2, 2});
    }).
    mark_it("sw corner sends correct grid scope", [&sw] {
        return test_that(sw.grid_scope == RectI{2, 0, 2, 2});
    });
});

describe<CompositeMapRegion>("CompositeMapRegion with an offset")([] {
    ReceivedLoadRequest ne, nw, se, sw;
    CompositeMapRegion comp_map{
        Grid<MapSubRegion>{
            { MapSubRegion{RectI{0, 0, 2, 2}, make_shared<TestMapRegion>(nw)},
              MapSubRegion{RectI{0, 2, 2, 2}, make_shared<TestMapRegion>(ne)} },
            { MapSubRegion{RectI{2, 0, 2, 2}, make_shared<TestMapRegion>(sw)},
              MapSubRegion{RectI{2, 2, 2, 2}, make_shared<TestMapRegion>(se)} }
        },
        ScaleComputation{2, 1, 2}};
    const RegionLoadRequest request
        {to_field(1.1, 1.1), to_field(1.1, 2.9), to_field(3.1, 2), Size2I{1, 1}};
    const auto framing = RegionPositionFraming{}.move(Vector2I{-1, 1});
    TestRegionLoadCollector test_collector;
    comp_map.process_load_request(request, framing, test_collector);

    mark_it("hits ne corner", [&ne] { return test_that(ne.hit); }).
    mark_it("hits nw corner", [&nw] { return test_that(nw.hit); }).
    mark_it("does not hit se corner", [&se] { return test_that(!se.hit); }).
    mark_it("does not hit sw corner", [&sw] { return test_that(!sw.hit); });
});

describe<CompositeMapRegion>
    ("CompositeMapRegion with irregularly split sub regions")([]
{
    ReceivedLoadRequest ne, nw, se, sw;
    CompositeMapRegion comp_map{
        Grid<MapSubRegion>{
            { MapSubRegion{RectI{0, 0, 2, 2}, make_shared<TestMapRegion>(nw)},
              MapSubRegion{RectI{0, 2, 2, 2}, make_shared<TestMapRegion>(ne)} },
            { MapSubRegion{RectI{2, 0, 2, 2}, make_shared<TestMapRegion>(sw)},
              MapSubRegion{RectI{2, 2, 2, 2}, make_shared<TestMapRegion>(se)} }
        },
        ScaleComputation{2, 1, 2}};
    const RegionLoadRequest request
        {to_field(0.1, 2.1), to_field(0.1, 3.9), to_field(2.1, 3), Size2I{1, 1}};
    const RegionPositionFraming framing;
    TestRegionLoadCollector test_collector;
    comp_map.process_load_request(request, framing, test_collector);
});

return [] {};

} ();
