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

#include "../../src/map-director/CompositeMapRegion.hpp"
#include "../../src/map-director/RegionLoadRequest.hpp"

#include "../test-helpers.hpp"

#include <set>

namespace {

using namespace cul::tree_ts;
using RectI = RectangleI;

class RequestAllRegions final : public RegionLoadRequestBase {
public:
    RequestAllRegions():
        m_max_region_size(k_default_max_region_size) {}

    explicit RequestAllRegions(const Size2I & max_region_size_):
        m_max_region_size(max_region_size_) {}

    bool overlaps_with(const RectangleI &) const { return true; }

    Size2I max_region_size() const { return m_max_region_size; }

private:
    static constexpr auto k_max_int = std::numeric_limits<int>::max();
    static constexpr Size2I k_default_max_region_size{k_max_int, k_max_int};

    Size2I m_max_region_size;
};

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
        (const RegionLoadRequestBase &,
         const RegionPositionFraming & framing,
         RegionLoadCollectorBase &,
         const Optional<RectangleI> & grid_scope) final
    { *received = ReceivedLoadRequest{framing, *grid_scope}; }

    Size2I size2() const final { throw ""; }

    ReceivedLoadRequest * received = nullptr;
};

class TestRegionLoadCollector final : public RegionLoadCollectorBase {
public:
    void collect_load_job
        (const SubRegionPositionFraming &, const ProducableSubGrid &) final {}
};

using MapSubRegionOwnersMap = SubRegionGridStacker::MapSubRegionOwnersMap;
using MapSubRegionViewGrid = CompositeMapRegion::MapSubRegionViewGrid;

MapSubRegionViewGrid to_view_grid(const Grid<MapSubRegion> & grid) {
    MapSubRegionViewGrid::Inserter inserter{grid.size2()};
    for (; !inserter.filled(); inserter.advance()) {
        inserter.push(&grid(inserter.position()));
    }
    return inserter.finish();
}

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

describe<CompositeMapRegion>("CompositeMapRegion")([] {
    // example (b)
    ReceivedLoadRequest ne, nw, se, sw;
    auto subregion_grid = make_shared<Grid<MapSubRegion>>(Grid<MapSubRegion>{
        { MapSubRegion{RectI{0, 0, 2, 2}, make_shared<TestMapRegion>(nw)},
          MapSubRegion{RectI{0, 2, 2, 2}, make_shared<TestMapRegion>(ne)} },
        { MapSubRegion{RectI{2, 0, 2, 2}, make_shared<TestMapRegion>(sw)},
          MapSubRegion{RectI{2, 2, 2, 2}, make_shared<TestMapRegion>(se)} }
    });
    MapSubRegionOwnersMap owners{nullptr};
    owners.insert(subregion_grid, std::monostate{});
    CompositeMapRegion comp_map{
        make_tuple(to_view_grid(*subregion_grid), owners),
        ScaleComputation{6, 1, 6}};
    TestRegionLoadCollector test_collector;
    const RegionPositionFraming framing{ScaleComputation{}, Vector2I{1, 3}};

    mark_it("se framing in correct on field position", [&] {
        comp_map.process_load_request
            (RequestAllRegions{}, framing, test_collector);
        return test_that(se.framing ==
                         RegionPositionFraming{ScaleComputation{6, 1, 6},
                                               Vector2I{6, 6} + Vector2I{1, 3}});
    }).
    mark_it("se framing in correct on field position, 1x1 max region request", [&] {
        comp_map.process_load_request
            (RequestAllRegions{Size2I{1, 1}}, framing, test_collector);
        return test_that(se.framing ==
                         RegionPositionFraming{ScaleComputation{6, 1, 6},
                                               Vector2I{6, 6} + Vector2I{1, 3}});
    }).
    mark_it("hits subregion targetted by a grid scope correctly", [&] {
        comp_map.process_load_request
            (RequestAllRegions{}, framing, test_collector,
             RectangleI{1, 1, 1, 1});
        return test_that(!sw.hit && se.hit && !nw.hit);
    });
});

return [] {};

} ();
