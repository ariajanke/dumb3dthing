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
#include "TwoWaySplit.hpp"

class NorthWestInCornerSplit final : public TwoWaySplit {
public:
    NorthWestInCornerSplit
        (const TileCornerElevations &,
         Real division_xz);

    void make_top(LinearStripTriangleCollection &) const final;

    void make_bottom(LinearStripTriangleCollection &) const final;

    void make_wall(LinearStripTriangleCollection &) const final;\

private:
    Vector north_west() const;

    Vector center_floor() const;

    Vector center_top() const;

    Vector nw_ne_floor() const;

    Vector nw_ne_top() const;

    Vector nw_sw_floor() const;

    Vector nw_sw_top() const;

    Vector south_east() const;

    Vector south_west() const;

    Vector north_east() const;

    Real north_west_y() const { return *m_elevations.north_west(); }

    Real north_east_y() const { return *m_elevations.north_east(); }

    Real south_east_y() const { return *m_elevations.south_east(); }

    Real south_west_y() const { return *m_elevations.south_west(); }

    bool are_all_present() const;

    bool can_make_top() const;

    TileCornerElevations m_elevations;
    Real m_division_xz;
};

// ----------------------------------------------------------------------------

class SouthWestInCornerSplit final :
    public TransformedTwoWaySplit<TwoWaySplit::invert_z>
{
public:
    SouthWestInCornerSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const TwoWaySplit & original_split() const final
        { return m_nw_split; }

    NorthWestInCornerSplit m_nw_split;
};

// ----------------------------------------------------------------------------

class NorthEastInCornerSplit final :
    public TransformedTwoWaySplit<TwoWaySplit::invert_x>
{
public:
    NorthEastInCornerSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const TwoWaySplit & original_split() const final
        { return m_nw_split; }

    NorthWestInCornerSplit m_nw_split;
};

// ----------------------------------------------------------------------------

class SouthEastInCornerSplit final :
    public TransformedTwoWaySplit<TwoWaySplit::invert_xz>
{
public:
    SouthEastInCornerSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const TwoWaySplit & original_split() const final
        { return m_nw_split; }

    NorthWestInCornerSplit m_nw_split;
};

// ----------------------------------------------------------------------------

class NorthEastInWallGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
    {
        NorthEastInCornerSplit neics{elevations, division_z};
        with_split_callback(neics);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {{},
             elevations.north_west(),
             elevations.south_west(),
             elevations.south_east()};
    }
};

// ----------------------------------------------------------------------------

class SouthWestInWallGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
    {
        SouthWestInCornerSplit swics{elevations, division_z};
        with_split_callback(swics);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {elevations.north_east(),
             elevations.north_west(),
             {},
             elevations.south_east()};
    }
};

// ----------------------------------------------------------------------------

class NorthWestInWallGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
    {
        NorthWestInCornerSplit nwocs{elevations, division_z};
        with_split_callback(nwocs);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {elevations.north_east(),
             {},
             elevations.south_west(),
             elevations.south_east()};
    }
};

// ----------------------------------------------------------------------------

class SouthEastInWallGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
    {
        SouthEastInCornerSplit seics{elevations, division_z};
        with_split_callback(seics);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {elevations.north_east(),
             elevations.north_west(),
             elevations.south_west(),
             {}};
    }
};
