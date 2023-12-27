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

#include "FlatTilesetTileN.hpp"

class LinearStripTriangleCollection {
public:
    void make_strip
        (const Vector & a_start, const Vector & a_last,
         const Vector & b_start, const Vector & b_last,
         int steps_count);

    virtual void add_triangle(const TriangleSegment &) = 0;
};

// ----------------------------------------------------------------------------

class NorthSouthSplit final {
public:
    NorthSouthSplit
        (Real north_west_y,
         Real north_east_y,
         Real south_west_y,
         Real south_east_y,
         Real division_z);

    void make_top(LinearStripTriangleCollection &) const;

    void make_bottom(LinearStripTriangleCollection &) const;

    void make_wall(LinearStripTriangleCollection &) const;

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

class WallTilesetTile final : public SlopesTilesetTile {
public:
    void load
        (const MapTilesetTile &,
         const TilesetTileTexture &,
         PlatformAssetsStrategy & platform) final;

    TileCornerElevations corner_elevations() const final;

    void make
        (const TileCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const;
};
