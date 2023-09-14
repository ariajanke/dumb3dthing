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
         const RegionPositionFraming &,
         RegionLoadCollectorBase &) final
    {
        throw std::runtime_error{"must not be called"};
    }

    void process_limited_load_request
        (const RegionLoadRequest &,
         const RegionPositionFraming & framing,
         const RectangleI & grid_scope,
         RegionLoadCollectorBase &) final
    { *received = ReceivedLoadRequest{framing, grid_scope}; }

    ReceivedLoadRequest * received = nullptr;
};

class TestRegionLoadCollector final : public RegionLoadCollectorBase {
public:
    void collect_load_job
        (const SubRegionPositionFraming &, const ProducableSubGrid &) final {}
};

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
        ScaleComputation{}};
    const RegionLoadRequest request
        {Vector2{0.1, 3.1}, Vector2{0.1, 4.9}, Vector2{2.1, 4}, Size2I{1, 1}};
    const RegionPositionFraming framing{Vector2I{1, 1}};
    TestRegionLoadCollector test_collector;
    comp_map.process_load_request(request, framing, test_collector);
    mark_it("does not hit ne corner", [&ne] {
        return test_that(!ne.hit);
    }).
    mark_it("does not hit nw corner", [&nw] {
        return test_that(!nw.hit);
    });
});

return [] {};

} ();
