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

#include "OutRampPropertiesLoader.hpp"

namespace {

using Orientation = OutRampPropertiesLoader::Orientation;

} // end of <anonymous> namespace

TileCornerElevations OutRampPropertiesLoader::elevation_offsets_for
    (CardinalDirection direction) const
{
    using Cd = CardinalDirection;
    switch (direction) {
    case Cd::north_east: return TileCornerElevations{0, 0, 1, 0};
    case Cd::north_west: return TileCornerElevations{0, 0, 0, 1};
    case Cd::south_east: return TileCornerElevations{0, 1, 0, 0};
    case Cd::south_west: return TileCornerElevations{1, 0, 0, 0};
    default:
        throw InvalidArgument{"direction bad"};
    }
}

Orientation OutRampPropertiesLoader::orientation_for
    (CardinalDirection direction) const
{
    using Cd = CardinalDirection;
    switch (direction) {
    case Cd::north_east: case Cd::south_west:
        return Orientation::nw_to_se_elements;
    case Cd::north_west: case Cd::south_east:
        return Orientation::sw_to_ne_elements;
    default:
        throw InvalidArgument{"direction bad"};
    }
}
