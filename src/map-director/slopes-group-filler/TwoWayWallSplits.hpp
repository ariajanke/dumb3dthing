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

#pragma once

#include "SplitWallGeometry.hpp"

class NorthSouthSplit final : public SplitWallGeometry {
public:
    static GeometryGenerationStrategy &
        choose_geometry_strategy(CardinalDirection);

    NorthSouthSplit
        (const TileCornerElevations &,
         Real division_z);

    NorthSouthSplit
        (Optional<Real> north_west_y,
         Optional<Real> north_east_y,
         Real south_west_y,
         Real south_east_y,
         Real division_z);

    void make_top(LinearStripTriangleCollection &) const final;

    void make_bottom(LinearStripTriangleCollection &) const final;

    void make_wall(LinearStripTriangleCollection &) const final;

private:
    void check_non_top_assumptions() const;

    Real south_west_y() const { return m_div_sw.y; }

    Real south_east_y() const { return m_div_se.y; }

    Real north_west_y() const { return m_div_nw.y; }

    Real north_east_y() const { return m_div_ne.y; }

    Vector m_div_nw;
    Vector m_div_sw;
    Vector m_div_ne;
    Vector m_div_se;
};

// ----------------------------------------------------------------------------

class SouthNorthSplit final :
    public TransformedSplitWallGeometry<SplitWallGeometry::invert_z>
{
public:
    SouthNorthSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const SplitWallGeometry & original_split() const final
        { return m_ns_split; }

    NorthSouthSplit m_ns_split;
};

// ----------------------------------------------------------------------------

class WestEastSplit final :
    public TransformedSplitWallGeometry<SplitWallGeometry::invert_x_swap_xz>
{
public:
    WestEastSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const SplitWallGeometry & original_split() const final
        { return m_ns_split; }

    NorthSouthSplit m_ns_split;
};

// ----------------------------------------------------------------------------

class EastWestSplit final :
    public TransformedSplitWallGeometry<SplitWallGeometry::xz_swap_roles>
{
public:
    EastWestSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const SplitWallGeometry & original_split() const final
        { return m_ns_split; }

    NorthSouthSplit m_ns_split;
};
