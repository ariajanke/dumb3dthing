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

#include "TwoWaySplit.hpp"

#include "../../TriangleSegment.hpp"

namespace {

/* <! auto breaks BFS ordering !> */ auto
    make_get_next_for_dir_split_v
    (Vector end, Vector step)
{
    return [end, step] (Vector east_itr) {
        auto cand_next = east_itr + step;
        if (are_very_close(cand_next, end)) return cand_next;

        if (are_very_close(normalize(end - east_itr ),
                           normalize(end - cand_next)))
        { return cand_next; }
        return end;
    };
}

/* <! auto breaks BFS ordering !> */ auto make_step_factory(int step_count) {
    return [step_count](const Vector & start, const Vector & last) {
        auto diff = last - start;
        auto step = magnitude(diff) / Real(step_count);
        if (are_very_close(diff, Vector{})) return Vector{};
        return step*normalize(diff);
    };
}

} // end of <anonymous> namespace

void LinearStripTriangleCollection::make_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     int steps_count)
{
    using Triangle = TriangleSegment;
    if (   are_very_close(a_start, a_last)
        && are_very_close(b_start, b_last))
    { return; }

    const auto make_step = make_step_factory(steps_count);

    auto itr_a = a_start;
    const auto next_a = make_get_next_for_dir_split_v(
        a_last, make_step(a_start, a_last));

    auto itr_b = b_start;
    const auto next_b = make_get_next_for_dir_split_v(
        b_last, make_step(b_start, b_last));

    while (   !are_very_close(itr_a, a_last)
           && !are_very_close(itr_b, b_last))
    {
        const auto new_a = next_a(itr_a);
        const auto new_b = next_b(itr_b);
        if (!are_very_close(itr_a, itr_b))
            add_triangle(Triangle{itr_a, itr_b, new_a});
        if (!are_very_close(new_a, new_b))
            add_triangle(Triangle{itr_b, new_a, new_b});
        itr_a = new_a;
        itr_b = new_b;
    }

    // at this point we are going to generate at most one triangle
    if (are_very_close(b_last, a_last)) {
        // here we're down to three points
        // there is only one possible triangle
        if (   are_very_close(itr_a, a_last)
            || are_very_close(itr_a, itr_b))
        {
            // take either being true:
            // in the best case: a line, so nothing
            return;
        }

        add_triangle(Triangle{itr_a, itr_b, a_last});
        return;
    }
    // a reminder from above
    assert(   are_very_close(itr_a, a_last)
           || are_very_close(itr_b, b_last));

    // here we still haven't ruled any points out
    if (   are_very_close(itr_a, itr_b)
        || (   are_very_close(itr_a, a_last)
            && are_very_close(itr_b, b_last)))
    {
        // either are okay, as they are "the same" pt
        return;
    } else if (!are_very_close(itr_a, a_last)) {
        // must exclude itr_b
        add_triangle(Triangle{itr_a, b_last, a_last});
        return;
    } else if (!are_very_close(itr_b, b_last)) {
        // must exclude itr_a
        add_triangle(Triangle{itr_b, a_last, b_last});
        return;
    }
}

// ----------------------------------------------------------------------------

/* static */ void TwoWaySplit::choose_on_direction_
    (CardinalDirection direction,
     const TileCornerElevations & elevations,
     Real division_z,
     const WithTwoWaySplit & with_split_callback)
{
    switch (direction) {
    case CardinalDirection::north: {
        NorthSouthSplit nss{elevations, division_z};
        with_split_callback(nss);
        }
        return;
    case CardinalDirection::south: {
        SouthNorthSplit sns{elevations, division_z};
        with_split_callback(sns);
        }
        return;
    case CardinalDirection::east: {
        EastWestSplit ews{elevations, division_z};
        with_split_callback(ews);
        }
        return;
    case CardinalDirection::west: {
        WestEastSplit wes{elevations, division_z};
        with_split_callback(wes);
        }
        return;
    default: break;
    }
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
    using cul::is_real;
#   if 0
    if (!is_real(*south_west_y) || !is_real(*south_east_y)) {
        throw InvalidArgument
            {"north_south_split: Southern elevations must be real numbers in "
             "all cases"};
    } else
#   endif
#   if 0
    if (!south_west_y || !south_east_y) {
        throw InvalidArgument
            {"north_south_split: Southern elevations must be real numbers in "
             "all cases"};
    }
#   endif
    if (division_z < -0.5 || division_z > 0.5) {
        throw InvalidArgument
            {"north_south_split: division must be in [-0.5 0.5]"};
    }
}

void NorthSouthSplit::make_top(LinearStripTriangleCollection & collection) const {
    Vector sw{-0.5, south_west_y(), -0.5};
    Vector se{ 0.5, south_east_y(), -0.5};
    collection.make_strip(m_div_sw, sw, m_div_se, se, 1);
}

void NorthSouthSplit::make_bottom(LinearStripTriangleCollection & collection) const {
    check_non_top_assumptions();

    Vector nw{-0.5, north_west_y(), 0.5};
    Vector ne{ 0.5, north_east_y(), 0.5};
    collection.make_strip(nw, m_div_nw, ne, m_div_ne, 1);
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
    TransformedNorthSouthSplit<TwoWaySplit::invert_z>(NorthSouthSplit{
        elevations.south_west(),
        elevations.south_east(),
        *elevations.north_west(),
        *elevations.north_east(),
        division_z}) {}

// ----------------------------------------------------------------------------

WestEastSplit::WestEastSplit
    (const TileCornerElevations & elevations,
     Real division_z):
    TransformedNorthSouthSplit<TwoWaySplit::invert_x_swap_xz>(NorthSouthSplit{
        elevations.south_west(),
        elevations.south_east(),
        *elevations.north_west(),
        *elevations.north_east(),
        division_z}) {}

// ----------------------------------------------------------------------------

EastWestSplit::EastWestSplit
    (const TileCornerElevations & elevations,
     Real division_z):
     TransformedNorthSouthSplit<TwoWaySplit::xz_swap_roles>(NorthSouthSplit{
      elevations.south_east(),
      elevations.north_east(),
      *elevations.south_west(),
      *elevations.north_west(),
      division_z}) {}
