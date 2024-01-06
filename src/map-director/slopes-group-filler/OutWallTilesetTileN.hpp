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

#include "SlopesTilesetTileN.hpp"
#include "TwoWaySplit.hpp"

class NorthWestOutCornerSplit final : public TwoWaySplit {
public:
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
    public TransformedTwoWaySplit<TwoWaySplit::invert_z>
{
public:
    SouthWestOutCornerSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const TwoWaySplit & original_split() const final
        { return m_nw_split; }

    NorthWestOutCornerSplit m_nw_split;
};

class NorthEastOutCornerSplit final :
    public TransformedTwoWaySplit<TwoWaySplit::invert_x>
{
public:
    NorthEastOutCornerSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const TwoWaySplit & original_split() const final
        { return m_nw_split; }

    NorthWestOutCornerSplit m_nw_split;
};

class SouthEastOutCornerSplit final :
    public TransformedTwoWaySplit<TwoWaySplit::invert_xz>
{
public:
    SouthEastOutCornerSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const TwoWaySplit & original_split() const final
        { return m_nw_split; }

    NorthWestOutCornerSplit m_nw_split;
};

class NorthWestOutWallGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
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

class SouthEastOutWallGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
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

class NorthEastOutWallGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
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

class SouthWestOutWallGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const TwoWaySplit::WithTwoWaySplit & with_split_callback) const final
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


class NullGeometryGenerationStrategy final :
    public TwoWaySplit::GeometryGenerationStrategy
{
    void with_splitter_do
        (const TileCornerElevations &,
         Real,
         const TwoWaySplit::WithTwoWaySplit &) const final
    {}

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations{};
    }
};


class OutWallTilesetTile final : public SlopesTilesetTile {
public:
    void load
        (const MapTilesetTile &,
         const TilesetTileTexture &,
         PlatformAssetsStrategy & platform) final;

    TileCornerElevations corner_elevations() const final;

    void make
        (const NeighborCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const;
};
