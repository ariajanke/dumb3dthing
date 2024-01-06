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

#include "SlopesTilesetTile.hpp"
#include "SplitWallGeometry.hpp"

class NorthWestOutCornerSplit final : public SplitWallGeometry {
public:
    static GeometryGenerationStrategy &
        choose_out_wall_strategy(CardinalDirection);

    NorthWestOutCornerSplit
        (const TileCornerElevations &,
         Real division_xz);

    void make_top(LinearStripTriangleCollection &) const final;

    void make_bottom(LinearStripTriangleCollection &) const final;

    void make_wall(LinearStripTriangleCollection &) const final;

private:
    // "control" points

    Vector north_west_corner() const;

    Vector north_west_floor() const;

    Vector north_west_top() const;

    Vector south_east() const;

    Vector north_east_corner() const;

    Vector north_east_floor() const;

    Vector north_east_top() const;

    Vector south_west_corner() const;

    Vector south_west_floor() const;

    Vector south_west_top() const;

    Real north_west_y() const;

    Real north_east_y() const;

    Real south_west_y() const;

    Real south_east_y() const;

    TileCornerElevations m_corner_elevations;
    Real m_division_xz;
};

// ----------------------------------------------------------------------------

class SouthWestOutCornerSplit final :
    public TransformedSplitWallGeometry<SplitWallGeometry::invert_z>
{
public:
    SouthWestOutCornerSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const SplitWallGeometry & original_split() const final
        { return m_nw_split; }

    NorthWestOutCornerSplit m_nw_split;
};

// ----------------------------------------------------------------------------

class NorthEastOutCornerSplit final :
    public TransformedSplitWallGeometry<SplitWallGeometry::invert_x>
{
public:
    NorthEastOutCornerSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const SplitWallGeometry & original_split() const final
        { return m_nw_split; }

    NorthWestOutCornerSplit m_nw_split;
};

// ----------------------------------------------------------------------------

class SouthEastOutCornerSplit final :
    public TransformedSplitWallGeometry<SplitWallGeometry::invert_xz>
{
public:
    SouthEastOutCornerSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const SplitWallGeometry & original_split() const final
        { return m_nw_split; }

    NorthWestOutCornerSplit m_nw_split;
};

// ----------------------------------------------------------------------------

class NorthWestOutWallGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const final
    {
        NorthWestOutCornerSplit nwocs{elevations, division_z};
        with_split_callback(nwocs);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {{},
             {},
             {},
             elevations.south_east()};
    }
};

// ----------------------------------------------------------------------------

class SouthEastOutWallGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const final
    {
        SouthEastOutCornerSplit seocs{elevations, division_z};
        with_split_callback(seocs);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {{},
             elevations.north_west(),
             {},
             {}};
    }
};

// ----------------------------------------------------------------------------

class NorthEastOutWallGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const final
    {
        NorthEastOutCornerSplit neocs{elevations, division_z};
        with_split_callback(neocs);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {{},
             {},
             elevations.south_west(),
             {}};
    }
};

// ----------------------------------------------------------------------------

class SouthWestOutWallGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const final
    {
        SouthWestOutCornerSplit swocs{elevations, division_z};
        with_split_callback(swocs);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {elevations.north_east(),
             {},
             {},
             {}};
    }
};
#if 0
// ----------------------------------------------------------------------------

class NullGeometryGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
    void with_splitter_do
        (const TileCornerElevations &,
         Real,
         const SplitWallGeometry::WithSplitWallGeometry &) const final
    {}

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations{};
    }
};
#endif
