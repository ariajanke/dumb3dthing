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

#include <set>

namespace {

class TestProducableTile final : public ProducableTile {
public:
    void operator () (ProducableTileCallbacks &) const final {}
};

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe("ProducableTileGridStacker")([] {
    mark_it("makes a producable tile view grid", [&] {
        TestProducableTile a, b;
        Grid<ProducableTile *> grid_a { { &a } };
        Grid<ProducableTile *> grid_b { { &b } };
        auto stacker = StackableProducableTileGrid{std::move(grid_a), {}}.
            stack_with(ProducableTileGridStacker{});
        stacker = StackableProducableTileGrid{std::move(grid_b), {}}.
            stack_with(std::move(stacker));
        auto producables = stacker.to_producables();
        auto view = producables.make_subgrid()(Vector2I{0, 0});
        std::set<ProducableTile *> tiles = { &a, &b };
        for (auto & tile : view) {
            auto itr = tiles.find(tile);
            if (itr == tiles.end()) {
                return test_that(false);
            }
            tiles.erase(itr);
        }
        return test_that(tiles.empty());
    });
});

return [] {};

} ();
