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

#include "TwoWayWallSplits.hpp"

#include "../../TriangleSegment.hpp"

namespace {

using Triangle = TriangleSegment;

class NorthGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const final
    {
        NorthSouthSplit nss{elevations, division_z};
        with_split_callback(nss);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {{},
             {},
             elevations.south_west(),
             elevations.south_east()};
    }
};

class SouthGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const final
    {
        SouthNorthSplit sns{elevations, division_z};
        with_split_callback(sns);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {elevations.north_east(),
             elevations.north_west(),
             {},
             {}};
    }
};

class EastGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const final
    {
        EastWestSplit ews{elevations, division_z};
        with_split_callback(ews);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {{},
             elevations.north_west(),
             elevations.south_west(),
             {}};
    }
};

class WestGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const SplitWallGeometry::WithSplitWallGeometry & with_split_callback) const final
    {
        WestEastSplit wes{elevations, division_z};
        with_split_callback(wes);
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elevations) const final
    {
        return TileCornerElevations
            {elevations.north_east(),
             {},
             {},
             elevations.south_east()};
    }
};

} // end of <anonymous> namespace

// ----------------------------------------------------------------------------

/* static */ SplitWallGeometry::GeometryGenerationStrategy &
    NorthSouthSplit::choose_geometry_strategy
    (CardinalDirection direction)
{
    static NorthGenerationStrategy north_gen_strat;
    static SouthGenerationStrategy south_gen_strat;
    static EastGenerationStrategy  east_gen_strat;
    static WestGenerationStrategy  west_gen_strat;
    switch (direction) {
    case CardinalDirection::north: return north_gen_strat;
    case CardinalDirection::south: return south_gen_strat;
    case CardinalDirection::east : return east_gen_strat ;
    case CardinalDirection::west : return west_gen_strat ;
    default: break;
    }
    // should probably return nullptr or something
    // (as this is based on user data)
    throw InvalidArgument{"bad direction"};
}

// ----------------------------------------------------------------------------

NorthSouthSplit::NorthSouthSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    NorthSouthSplit
        (elevations.north_west(),
         elevations.north_east(),
         *elevations.south_west(),
         *elevations.south_east(),
         division_z) {}

NorthSouthSplit::NorthSouthSplit
    (Optional<Real> north_west_y,
     Optional<Real> north_east_y,
     Real south_west_y,
     Real south_east_y,
     Real division_z):
    m_div_nw(-0.5, north_west_y.value_or(k_inf), -division_z),
    m_div_sw(-0.5, south_west_y, -division_z),
    m_div_ne( 0.5, north_east_y.value_or(k_inf), -division_z),
    m_div_se( 0.5, south_east_y, -division_z)
{
    if (division_z < -0.5 || division_z > 0.5) {
        throw InvalidArgument
            {"north_south_split: division must be in [-0.5 0.5]"};
    }
}

void NorthSouthSplit::make_top(LinearStripTriangleCollection & collection) const {
    Vector sw{-0.5, south_west_y(), -0.5};
    Vector se{ 0.5, south_east_y(), -0.5};
    collection.add_triangle(Triangle{sw      , se, m_div_sw}, cut_y);
    collection.add_triangle(Triangle{m_div_sw, se, m_div_se}, cut_y);
}

void NorthSouthSplit::make_bottom(LinearStripTriangleCollection & collection) const {
    check_non_top_assumptions();

    Vector nw{-0.5, north_west_y(), 0.5};
    Vector ne{ 0.5, north_east_y(), 0.5};
    collection.add_triangle(Triangle{m_div_nw, m_div_ne, nw}, cut_y);
    collection.add_triangle(Triangle{      nw, m_div_ne, ne}, cut_y);
}

void NorthSouthSplit::make_wall(LinearStripTriangleCollection & collection) const {
    check_non_top_assumptions();

    // both sets of y values' directions must be the same
    assert((north_east_y() - north_west_y())*
           (south_east_y() - south_west_y()) >= 0);
    collection.make_strip(m_div_nw, m_div_sw, m_div_ne, m_div_se, 1);
}

/* private */ void NorthSouthSplit::check_non_top_assumptions() const {
    using cul::is_real;
    if ((!is_real(north_west_y()) || !is_real(north_east_y()))) {
        throw InvalidArgument
            {"north_south_split: Northern elevations must be real numbers in "
             "top cases"};
    }
    if (south_west_y() < north_west_y() || south_east_y() < north_east_y()) {
        throw InvalidArgument
            {"north_south_split: method was designed assuming south is the top"};
    }
}

// ----------------------------------------------------------------------------

SouthNorthSplit::SouthNorthSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_ns_split(
        elevations.south_west(),
        elevations.south_east(),
        *elevations.north_west(),
        *elevations.north_east(),
        division_z) {}

// ----------------------------------------------------------------------------

WestEastSplit::WestEastSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    m_ns_split(
        elevations.north_west(),
        elevations.south_west(),
        *elevations.north_east(),
        *elevations.south_east(),
        division_z) {}

// ----------------------------------------------------------------------------

EastWestSplit::EastWestSplit
    (const TileCornerElevations & elevations,
     Real division_z):
     m_ns_split(
      elevations.south_east(),
      elevations.north_east(),
      *elevations.south_west(),
      *elevations.north_west(),
      division_z) {}
