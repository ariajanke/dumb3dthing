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


#include "SlopesTilesetTileN.hpp"

namespace {

Optional<Real> first_of(Optional<Real> a, Optional<Real> b, Optional<Real> c) {
    if (a) return a;
    if (b) return b;
    return c;
}

} // end of <anonymous> namespace

TileCornerElevations TileCornerElevations::NeighborElevations::elevations() const {
    using Cd = CardinalDirection;
    const auto & northern_tile = elevations_from(Cd::n);
    const auto & southern_tile = elevations_from(Cd::s);
    const auto & eastern_tile = elevations_from(Cd::e);
    const auto & western_tile = elevations_from(Cd::w);

    auto north_west = first_of
        (northern_tile.south_west(),
         elevations_from(Cd::nw).south_east(),
         western_tile.north_east());
    auto south_west = first_of
        (western_tile.south_east(),
         elevations_from(Cd::sw).north_east(),
         southern_tile.north_west());
    auto south_east = first_of
        (southern_tile.north_east(),
         elevations_from(Cd::se).north_west(),
         eastern_tile.south_west());
    auto north_east = first_of
        (eastern_tile.north_west(),
         elevations_from(Cd::ne).south_west(),
         northern_tile.south_east());
    return TileCornerElevations
        {north_east, north_west, south_west, south_east};
}
