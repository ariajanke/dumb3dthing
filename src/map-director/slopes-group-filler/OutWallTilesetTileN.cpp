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

#include "OutWallTilesetTileN.hpp"

namespace {

using Triangle = TriangleSegment;

} // end of <anonymous> namespace

NorthWestOutCornerSplit::NorthWestOutCornerSplit
    (const TileCornerElevations & elevations,
     Real division_xz):
    m_corner_elevations(elevations),
    m_division_xz(division_xz) {}

void NorthWestOutCornerSplit::make_top
    (LinearStripTriangleCollection & col) const
{
    if (are_very_close(m_division_xz, 0.5)) return;
    auto se     = south_east();
    auto nw_top = north_west_top();
    // these are strips... they're like cut outs of flats
    // but how do I handle this?
    col.add_triangle(Triangle{nw_top, north_east_top(), se}, cut_y);
    col.add_triangle(Triangle{nw_top, south_west_top(), se}, cut_y);
}

void NorthWestOutCornerSplit::make_bottom
    (LinearStripTriangleCollection & col) const
{
    if (are_very_close(m_division_xz, -0.5)) return;

    auto nw_corner = north_west_corner();
    auto ne_floor = north_east_floor();
    auto sw_floor = south_west_floor();
    col.add_triangle(Triangle{nw_corner, north_east_corner(), ne_floor}, cut_y);
    if (!are_very_close(m_division_xz, 0.5)) {
        auto nw_floor = north_west_floor();
        col.add_triangle(Triangle{nw_corner, nw_floor , ne_floor}, cut_y);
        col.add_triangle(Triangle{nw_corner, nw_floor , sw_floor}, cut_y);
    }
    col.add_triangle(Triangle{nw_corner, south_west_corner(), sw_floor}, cut_y);
}

void NorthWestOutCornerSplit::make_wall
    (LinearStripTriangleCollection & col) const
{
    auto nw_top = north_west_top();
    auto nw_floor = north_west_floor();
    col.make_strip(north_east_floor(), north_east_top(), nw_floor, nw_top, 1);
    col.make_strip(south_west_floor(), south_west_top(), nw_floor, nw_top, 1);
}

/* private */ Vector NorthWestOutCornerSplit::north_west_corner() const
    { return Vector{-0.5, north_west_y(), 0.5}; }

/* private */ Vector NorthWestOutCornerSplit::north_west_floor() const
    { return Vector{m_division_xz, north_west_y(), -m_division_xz}; }

/* private */ Vector NorthWestOutCornerSplit::north_west_top() const
    { return Vector{m_division_xz, south_east_y(), -m_division_xz}; }

/* private */ Vector NorthWestOutCornerSplit::south_east() const
    { return Vector{0.5, south_east_y(), -0.5}; }

/* private */ Vector NorthWestOutCornerSplit::north_east_corner() const
    { return Vector{0.5, north_east_y(), 0.5}; }

/* private */ Vector NorthWestOutCornerSplit::north_east_floor() const
    { return Vector{0.5, north_east_y(), -m_division_xz}; }

/* private */ Vector NorthWestOutCornerSplit::north_east_top() const
    { return Vector{0.5, south_east_y(), -m_division_xz}; }

/* private */ Vector NorthWestOutCornerSplit::south_west_corner() const
    { return Vector{-0.5, south_west_y(), -0.5}; }

/* private */ Vector NorthWestOutCornerSplit::south_west_floor() const
    { return Vector{m_division_xz, south_west_y(), -0.5}; }

/* private */ Vector NorthWestOutCornerSplit::south_west_top() const
    { return Vector{m_division_xz, south_east_y(), -0.5}; }

/* private */ Real NorthWestOutCornerSplit::north_west_y() const
    { return *m_corner_elevations.north_west(); }

/* private */ Real NorthWestOutCornerSplit::north_east_y() const
    { return *m_corner_elevations.north_east(); }

/* private */ Real NorthWestOutCornerSplit::south_west_y() const
    { return *m_corner_elevations.south_west(); }

/* private */ Real NorthWestOutCornerSplit::south_east_y() const
    { return *m_corner_elevations.south_east(); }

// ----------------------------------------------------------------------------

SouthWestOutCornerSplit::SouthWestOutCornerSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_nw_split(TileCornerElevations{
        elevations.south_east(),
        elevations.south_west(),
        elevations.north_west(),
        *elevations.north_east()},
        division_z) {}

// ----------------------------------------------------------------------------

NorthEastOutCornerSplit::NorthEastOutCornerSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_nw_split(TileCornerElevations{
        elevations.north_west(),
        elevations.north_east(),
        elevations.south_east(),
        *elevations.south_west()},
        division_z) {}

// ----------------------------------------------------------------------------

SouthEastOutCornerSplit::SouthEastOutCornerSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_nw_split(TileCornerElevations{
        elevations.south_west(),
        *elevations.south_east(),
        elevations.north_east(),
        elevations.north_west()},
        division_z) {}
