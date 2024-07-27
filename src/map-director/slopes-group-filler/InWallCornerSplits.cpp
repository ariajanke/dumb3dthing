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

#include "InWallCornerSplits.hpp"

namespace {

using Triangle = TriangleSegment;

} // end of <anonymous> namespace

/* static */ SplitWallGeometry::GeometryGenerationStrategy &
    NorthWestInCornerSplit::choose_in_wall_strategy(CardinalDirection direction)
{
    static NorthWestInWallGenerationStrategy nw_in_strat;
    static NorthEastInWallGenerationStrategy ne_in_strat;
    static SouthWestInWallGenerationStrategy sw_in_strat;
    static SouthEastInWallGenerationStrategy se_in_strat;
    switch (direction) {
    case CardinalDirection::north_west: return nw_in_strat;
    case CardinalDirection::north_east: return ne_in_strat;
    case CardinalDirection::south_west: return sw_in_strat;
    case CardinalDirection::south_east: return se_in_strat;
    default: break;
    }
    throw InvalidArgument{"bad direction"};
}

NorthWestInCornerSplit::NorthWestInCornerSplit
    (const TileCornerElevations & elevations,
     Real division_xz):
    m_elevations(elevations),
    m_division_xz(division_xz)
{
    if (   are_all_present()
        && (south_east_y() < north_west_y() ||
            south_east_y() < north_east_y() ||
            south_east_y() < south_west_y()))
    {
        throw InvalidArgument
            {"northwest_in_corner_split: south_east_y is assumed to be the "
             "top's elevation, method not explicitly written to handle south "
             "east *not* being the top"};
    }
}

void NorthWestInCornerSplit::make_top
    (LinearStripTriangleCollection & col) const
{
    // top is the relatively more complex shape here
    // four triangles
    auto ne = north_east();
    auto sw = south_west();
    auto ct = center_top();
    auto se = south_east();

    col.add_triangle(Triangle{ne, ct, se}, cut_y);
    col.add_triangle(Triangle{sw, ct, se}, cut_y);
    if (!are_very_close(m_division_xz, -0.5)) {
        col.add_triangle(Triangle{ne, ct, nw_ne_top()}, cut_y);
        col.add_triangle(Triangle{sw, ct, nw_sw_top()}, cut_y);
    }
}

void NorthWestInCornerSplit::make_bottom
    (LinearStripTriangleCollection & col) const
{
    auto nw = north_west();
    auto cf = center_floor();

    col.add_triangle(Triangle{nw, nw_ne_floor(), cf}, cut_y);
    col.add_triangle(Triangle{nw, nw_sw_floor(), cf}, cut_y);
}

void NorthWestInCornerSplit::make_wall
    (LinearStripTriangleCollection & col) const
{
    auto ct = center_top();
    auto cf = center_floor();

    col.make_strip(nw_ne_floor(), nw_ne_top(), cf, ct, 1);
    col.make_strip(nw_sw_floor(), nw_sw_top(), cf, ct, 1);
}

/* private */ Vector NorthWestInCornerSplit::north_west() const
    { return Vector{-0.5, north_west_y(), 0.5}; }

/* private */ Vector NorthWestInCornerSplit::center_floor() const
    { return Vector{m_division_xz, north_west_y(), -m_division_xz}; }

/* private */ Vector NorthWestInCornerSplit::center_top() const
    { return Vector{m_division_xz, south_east_y(), -m_division_xz}; }

/* private */ Vector NorthWestInCornerSplit::nw_ne_floor () const
    { return Vector{m_division_xz, north_west_y(), 0.5}; }

/* private */ Vector NorthWestInCornerSplit::nw_ne_top() const
    { return Vector{m_division_xz, north_east_y(), 0.5}; }

/* private */ Vector NorthWestInCornerSplit::nw_sw_floor () const
    { return Vector{-0.5, north_west_y(), -m_division_xz}; }

/* private */ Vector NorthWestInCornerSplit::nw_sw_top   () const
    { return Vector{-0.5, north_east_y(), -m_division_xz}; }

/* private */ Vector NorthWestInCornerSplit::south_east() const
    { return Vector{0.5, south_east_y(), -0.5}; }

/* private */ Vector NorthWestInCornerSplit::south_west() const
    { return Vector{-0.5, south_west_y(), -0.5}; }

/* private */ Vector NorthWestInCornerSplit::north_east() const
    { return Vector{0.5, north_east_y(), 0.5}; }

/* private */ bool NorthWestInCornerSplit::are_all_present() const {
    return can_make_top() && m_elevations.south_east().has_value();
}

/* private */ bool NorthWestInCornerSplit::can_make_top() const {
    return m_elevations.north_west().has_value() &&
           m_elevations.north_east().has_value() &&
           m_elevations.south_west().has_value();
}

// ----------------------------------------------------------------------------

SouthWestInCornerSplit::SouthWestInCornerSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_nw_split(TileCornerElevations{
        *elevations.south_east(),
        *elevations.south_west(),
        *elevations.north_west(),
        elevations.north_east()},
        division_z) {}

// ----------------------------------------------------------------------------

NorthEastInCornerSplit::NorthEastInCornerSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_nw_split(TileCornerElevations{
        *elevations.north_west(),
        *elevations.north_east(),
        *elevations.south_east(),
        elevations.south_west()},
        division_z) {}

// ----------------------------------------------------------------------------

SouthEastInCornerSplit::SouthEastInCornerSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_nw_split(TileCornerElevations{
        *elevations.south_west(),
        elevations.south_east(),
        *elevations.north_east(),
        *elevations.north_west()},
        division_z) {}

// ----------------------------------------------------------------------------

void NorthEastInWallGenerationStrategy::with_splitter_do
    (const TileCornerElevations & elevations,
     Real division_z,
     const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const
{
    NorthEastInCornerSplit neics{elevations, division_z};
    with_split_callback(neics);
}

TileCornerElevations
    NorthEastInWallGenerationStrategy::filter_to_known_corners
    (TileCornerElevations elevations) const
{
    return TileCornerElevations
        {{},
         elevations.north_west(),
         elevations.south_west(),
         elevations.south_east()};
}

// ----------------------------------------------------------------------------

void SouthWestInWallGenerationStrategy::with_splitter_do
    (const TileCornerElevations & elevations,
     Real division_z,
     const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const
{
    SouthWestInCornerSplit swics{elevations, division_z};
    with_split_callback(swics);
}

TileCornerElevations
    SouthWestInWallGenerationStrategy::filter_to_known_corners
    (TileCornerElevations elevations) const
{
    return TileCornerElevations
        {elevations.north_east(),
         elevations.north_west(),
         {},
         elevations.south_east()};
}

// ----------------------------------------------------------------------------

void NorthWestInWallGenerationStrategy::with_splitter_do
    (const TileCornerElevations & elevations,
     Real division_z,
     const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const
{
    NorthWestInCornerSplit nwocs{elevations, division_z};
    with_split_callback(nwocs);
}

TileCornerElevations
    NorthWestInWallGenerationStrategy::filter_to_known_corners
    (TileCornerElevations elevations) const
{
    return TileCornerElevations
        {elevations.north_east(),
         {},
         elevations.south_west(),
         elevations.south_east()};
}

// ----------------------------------------------------------------------------

void SouthEastInWallGenerationStrategy::with_splitter_do
    (const TileCornerElevations & elevations,
     Real division_z,
     const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const
{
    SouthEastInCornerSplit seics{elevations, division_z};
    with_split_callback(seics);
}

TileCornerElevations
    SouthEastInWallGenerationStrategy::filter_to_known_corners
    (TileCornerElevations elevations) const
{
    return TileCornerElevations
        {elevations.north_east(),
         elevations.north_west(),
         elevations.south_west(),
         {}};
}
