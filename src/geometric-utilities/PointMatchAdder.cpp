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

#include "PointMatchAdder.hpp"

#include "../TriangleLink.hpp"

/* static */ SideToSideMapping SideToSideMapping::from_matches
    (const PointMatch & match_a, const PointMatch & match_b)
{
    auto pair_flip_left  = PointPairFlip::make(match_a.left_point (), match_b.left_point ());
    auto pair_flip_right = PointPairFlip::make(match_a.right_point(), match_b.right_point());
    bool flip_position =
        pair_flip_left.parameters_flipped() != pair_flip_right.parameters_flipped();
    return SideToSideMapping{pair_flip_left.side(), pair_flip_right.side(), flip_position};
}

// ----------------------------------------------------------------------------

PointMatch::PointMatch
    (const Vector & lhs, TrianglePoint lhs_pt,
     const Vector & rhs, TrianglePoint rhs_pt):
    m_lhs_pt  (lhs),
    m_lhs_addr(lhs_pt),
    m_rhs_pt  (rhs),
    m_rhs_addr(rhs_pt) {}

Optional<PointMatch> PointMatch::meeting_points() const
    { return is_matching() ? *this : Optional<PointMatch>{}; }

/* private */ bool PointMatch::is_matching() const
    { return are_very_close(m_lhs_pt, m_rhs_pt); }

// ----------------------------------------------------------------------------

/* static */ PointPairFlip PointPairFlip::make
    (TrianglePoint a, TrianglePoint b)
{
    using Pt = TrianglePoint;
    using Side = TriangleSide;
    using namespace cul::exceptions_abbr;
    bool flip = static_cast<int>(a) > static_cast<int>(b);
    if (flip) {
        std::swap(a, b);
    }

    switch (a) {
    case Pt::a:
        switch (b) {
        case Pt::b: return PointPairFlip{ flip, Side::k_side_ab};
        case Pt::c: return PointPairFlip{!flip, Side::k_side_ca};
        default: break;
        }
        break;
    case Pt::b:
        if (b == Pt::c) { return PointPairFlip{flip, Side::k_side_bc}; }
        break;
    default: break;
    }
    throw RtError{":c"};
}

PointPairFlip::PointPairFlip(bool parameters_flipped_, TriangleSide side_):
    m_parameters_flipped(parameters_flipped_),
    m_side(side_) {}

// ----------------------------------------------------------------------------

PointMatchFinder::PointMatchFinder(MatchesArray && matches_):
    m_possible_matches(std::move(matches_)) {}

Optional<PointMatch> PointMatchFinder::operator () () const
    { return find_meeting_points<0>(); }

template <>
/* private */ Optional<PointMatch> PointMatchFinder::find_meeting_points
    <PointMatchFinder::k_pt_count>() const
    { return {}; }

template <std::size_t kt_idx>
/* private */ Optional<PointMatch> PointMatchFinder::find_meeting_points() const
{
    if (auto rv = m_possible_matches[kt_idx].meeting_points())
        return rv;
    return find_meeting_points<kt_idx + 1>();
}

// ----------------------------------------------------------------------------

/* static */ Optional<SideToSideMapping> PointMatchAdder::find_point_match
    (const TriangleSegment & lhs, const TriangleSegment & rhs)
{
    return PointMatchAdder{}.
        add(PointMatchFinder::from_left_point<TrianglePoint::a>(lhs, rhs)()).
        add(PointMatchFinder::from_left_point<TrianglePoint::b>(lhs, rhs)()).
        add(PointMatchFinder::from_left_point<TrianglePoint::c>(lhs, rhs)()).
        finish();
}

PointMatchAdder::PointMatchAdder():
    m_position(m_entries.begin()) {}

PointMatchAdder & PointMatchAdder::add(const Optional<PointMatch> & match) {
    using namespace cul::exceptions_abbr;
    if (!match) return *this;
    if (m_position == m_entries.end()) {
        throw RtError{"used wrong"};
    }
    *m_position++ = *match;
    return *this;
}

Optional<SideToSideMapping> PointMatchAdder::finish() {
    if (m_position - m_entries.begin() != 2) return {};
    return SideToSideMapping::from_matches(m_entries[0], m_entries[1]);
}
