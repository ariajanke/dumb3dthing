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

// northwest_out_corner_split

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
