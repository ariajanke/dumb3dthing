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

#pragma once

#include "RegionEdgeConnectionsContainer.hpp"

class RegionAxisAddressAndSide final {
public:
    static std::array<RegionAxisAddressAndSide, 4> for_
        (const Vector2I & on_field, const Size2I & grid_size);

    RegionAxisAddressAndSide() {}

    RegionAxisAddressAndSide(RegionAxis axis_, int value_, RegionSide side_):
        m_address(axis_, value_), m_side(side_) {}

    RegionAxisAddress address() const { return m_address; }

    RegionSide side() const { return m_side; }

private:
    RegionAxisAddress m_address;
    RegionSide m_side;
};

// ----------------------------------------------------------------------------

/* static */ inline std::array<RegionAxisAddressAndSide, 4>
    RegionAxisAddressAndSide::for_
    (const Vector2I & on_field, const Size2I & grid_size)
{
    using Side = RegionSide;
    using Axis = RegionAxis;
    auto bottom = on_field.y + grid_size.height;
    auto right  = on_field.x + grid_size.width ;
    return {
        RegionAxisAddressAndSide{Axis::x_ways, on_field.x, Side::left  },
        RegionAxisAddressAndSide{Axis::x_ways, right     , Side::right },
        RegionAxisAddressAndSide{Axis::z_ways, on_field.y, Side::top   },
        RegionAxisAddressAndSide{Axis::z_ways, bottom    , Side::bottom}
    };
}
