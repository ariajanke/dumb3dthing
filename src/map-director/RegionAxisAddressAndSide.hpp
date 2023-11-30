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

#include "../Definitions.hpp"

enum class RegionSide {
    left  , // west
    right , // east
    bottom, // south
    top   , // north
    uninitialized
};

enum class RegionAxis { x_ways, z_ways, uninitialized };

class RegionAxisAddress final {
public:
    RegionAxisAddress() {}

    RegionAxisAddress(RegionAxis axis_, int value_):
        m_axis(axis_), m_value(value_) {}

    bool operator < (const RegionAxisAddress &) const;

    bool operator == (const RegionAxisAddress &) const;

    RegionAxis axis() const { return m_axis; }

    int compare(const RegionAxisAddress &) const;

    int value() const { return m_value; }

    std::size_t hash() const;

private:
    RegionAxis m_axis = RegionAxis::uninitialized;
    int m_value = 0;
};

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

inline bool RegionAxisAddress::operator < (const RegionAxisAddress & rhs) const
    { return compare(rhs) < 0; }

inline bool RegionAxisAddress::operator == (const RegionAxisAddress & rhs) const
    { return compare(rhs) == 0; }

inline int RegionAxisAddress::compare(const RegionAxisAddress & rhs) const {
    if (m_axis == rhs.m_axis)
        { return m_value - rhs.m_value; }
    return static_cast<int>(m_axis) - static_cast<int>(rhs.m_axis);
}

inline std::size_t RegionAxisAddress::hash() const
    { return std::hash<int>{}(m_value) ^ std::hash<RegionAxis>{}(m_axis); }

// ----------------------------------------------------------------------------

/* static */ inline std::array<RegionAxisAddressAndSide, 4>
    RegionAxisAddressAndSide::for_
    (const Vector2I & on_field, const Size2I & grid_size)
{
    using Side = RegionSide;
    using Axis = RegionAxis;
    auto bottom = on_field.y + grid_size.height;
    auto right  = on_field.x + grid_size.width ;
    // x_ways <-> z_ways was swapped here
    return {
        RegionAxisAddressAndSide{Axis::z_ways, on_field.x, Side::left  },
        RegionAxisAddressAndSide{Axis::z_ways, right     , Side::right },
        RegionAxisAddressAndSide{Axis::x_ways, on_field.y, Side::top   },
        RegionAxisAddressAndSide{Axis::x_ways, bottom    , Side::bottom}
    };
}
