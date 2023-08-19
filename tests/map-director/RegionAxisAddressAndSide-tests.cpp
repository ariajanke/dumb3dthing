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

#include "../../src/map-director/RegionAxisAddressAndSide.hpp"

#include "../test-helpers.hpp"

bool operator == (const RegionAxisAddressAndSide & lhs,
                  const RegionAxisAddressAndSide & rhs)
{
    return lhs.address() == rhs.address() &&
           lhs.side   () == rhs.side   ();
}

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe<RegionAxisAddressAndSide>("RegionAxisAddressAndSide::for_")([]
{
    using AddrAndSide = RegionAxisAddressAndSide;
    using Axis        = RegionAxis;
    using Side        = RegionSide;
    mark_it("contains correct north side", [] {
        auto res = AddrAndSide::for_(Vector2I{1, 1}, Size2I{1, 1});
        AddrAndSide expected{Axis::x_ways, 1, Side::top};
        return test_that(std::find(res.begin(), res.end(), expected));
    }).
    mark_it("contains correct south side", [] {
        auto res = AddrAndSide::for_(Vector2I{1, 1}, Size2I{1, 2});
        AddrAndSide expected{Axis::x_ways, 3, Side::bottom};
        return test_that(std::find(res.begin(), res.end(), expected));
    }).
    mark_it("contains correct west side", [] {
        auto res = AddrAndSide::for_(Vector2I{2, 1}, Size2I{1, 1});
        AddrAndSide expected{Axis::z_ways, 2, Side::left};
        return test_that(std::find(res.begin(), res.end(), expected));
    }).
    mark_it("contains correct east side", [] {
        auto res = AddrAndSide::for_(Vector2I{2, 1}, Size2I{3, 1});
        AddrAndSide expected{Axis::z_ways, 5, Side::right};
        return test_that(std::find(res.begin(), res.end(), expected));
    });
});

return [] {};

} ();
