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

#include "../../src/map-director/MapRegionContainer.hpp"
#include "../../src/TriangleLink.hpp"

#include "../test-helpers.hpp"

namespace {

using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

struct SingleLinkGrid final {
    static SingleLinkGrid make() {
        SingleLinkGrid rv;
        rv.link = make_shared<TriangleLink>();

        ViewGridTriangle::Inserter inserter{Size2I{1, 1}};
        inserter.push(rv.link);
        inserter.advance();
        rv.grid = ScaledTriangleViewGrid
            {make_shared<ViewGridTriangle>(inserter.finish()),
             ScaleComputation{}};
        return rv;
    }
    SharedPtr<TriangleLink> link;
    ScaledTriangleViewGrid grid;
};

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe<MapRegionContainer>("MapRegionContainer")([]
{
    // yuck, complicated setup
    MapRegionContainer container;
    auto test_region = SingleLinkGrid::make();
    container.set_region
        (Vector2I{1, 1}, test_region.grid, std::vector<Entity>{});

    mark_it("decays the link that was for a given region", [&] {
        struct Impl final : public MapRegionContainer::RegionDecayAdder {
            void add(const Vector2I &,
                     ScaledTriangleViewGrid && view_grid,
                     std::vector<Entity> &&) final
            { decayed_link = *view_grid.all_links().begin(); }

            SharedPtr<TriangleLink> decayed_link;
        };
        Impl impl;
        container.decay_regions(impl);
        container.decay_regions(impl);
        return test_that(impl.decayed_link == test_region.link);
    }).
    mark_it("decays the same region that was added", [&] {
        struct Impl final : public MapRegionContainer::RegionDecayAdder {
            void add(const Vector2I & r,
                     ScaledTriangleViewGrid && grid,
                     std::vector<Entity> &&) final
            {
                addresses.clear();
                for (auto & addr_and_side : grid.sides_and_addresses_at(r)) {
                    addresses.push_back(addr_and_side.address());
                }
                std::sort(addresses.begin(), addresses.end());
                // decayed_region = RectangleI{r, grid};
            }

            // RectangleI decayed_region;
            std::vector<RegionAxisAddress> addresses;
        };
        Impl impl;
        container.decay_regions(impl);
        container.decay_regions(impl);
        std::vector<RegionAxisAddress> expected_addresses;
        expected_addresses.emplace_back(RegionAxis::x_ways, 0);
        expected_addresses.emplace_back(RegionAxis::x_ways, 1);
        expected_addresses.emplace_back(RegionAxis::z_ways, 0);
        expected_addresses.emplace_back(RegionAxis::z_ways, 1);
        std::sort(expected_addresses.begin(), expected_addresses.end());
        return test_that(std::equal(impl.addresses.begin(), impl.addresses.end(),
                                    expected_addresses.begin(), expected_addresses.end())); //impl.decayed_region == RectangleI{1, 1, 1, 1});
    });
});

return [] {};

} ();
