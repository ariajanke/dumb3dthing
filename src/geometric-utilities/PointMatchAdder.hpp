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

#include "../TriangleSegment.hpp"

class TriangleLink;

enum class TrianglePoint { a, b, c };

class PointMatch;

class SideToSideMapping final {
public:
    static SideToSideMapping from_matches
        (const PointMatch & match_a, const PointMatch & match_b);

    SideToSideMapping();

    SideToSideMapping(TriangleSide left_, TriangleSide right_, bool flips):
        m_left(left_), m_right(right_), m_flips(flips) {}

    TriangleSide left_side () const { return m_left; }

    TriangleSide right_side() const { return m_right; }

    bool sides_flip() const { return m_flips; }

private:
    TriangleSide m_left;
    TriangleSide m_right;
    bool m_flips;
};

class PointMatch final {
public:
    template <TrianglePoint kt_lhs_addr, TrianglePoint kt_rhs_addr>
    static PointMatch make
        (const TriangleSegment & lhs, const TriangleSegment & rhs);

    template <TrianglePoint kt_addr>
    static Vector point_of(const TriangleSegment &);

    PointMatch() {}

    PointMatch(const Vector & lhs, TrianglePoint lhs_pt,
               const Vector & rhs, TrianglePoint rhs_pt);

    Optional<PointMatch> meeting_points() const;

    TrianglePoint left_point() const { return m_lhs_addr; }

    TrianglePoint right_point() const { return m_rhs_addr; }

private:
    bool is_matching() const;

    Vector m_lhs_pt;
    TrianglePoint m_lhs_addr;
    Vector m_rhs_pt;
    TrianglePoint m_rhs_addr;
};

class PointPairFlip final {
public:
    static PointPairFlip make(TrianglePoint a, TrianglePoint b);

    PointPairFlip(bool parameters_flipped_, TriangleSide side_);

    bool parameters_flipped() const { return m_parameters_flipped; }

    TriangleSide side() const { return m_side; }

private:
    bool m_parameters_flipped;
    TriangleSide m_side;
};

class PointMatchFinder final {
    static constexpr std::size_t k_pt_count = 3;
public:
    using MatchesArray = std::array<PointMatch, k_pt_count>;

    template <TrianglePoint kt_lhs_addr>
    static PointMatchFinder from_left_point
        (const TriangleSegment & lhs, const TriangleSegment & rhs);

    explicit PointMatchFinder(MatchesArray &&);

    Optional<PointMatch> operator () () const;

private:
    template <std::size_t kt_idx>
    Optional<PointMatch> find_meeting_points() const;

    MatchesArray m_possible_matches;
};

class PointMatchPair final {
public:
    PointMatchPair(TriangleSide left_, TriangleSide right_):
        m_left(left_), m_right(right_) {}

    TriangleSide left_side() const { return m_left; }

    TriangleSide right_side() const { return m_right; }

private:
    TriangleSide m_left, m_right;
};

class PointMatchAdder final {
public:
    static Optional<SideToSideMapping> find_point_match
        (const TriangleSegment & lhs, const TriangleSegment & rhs);

    PointMatchAdder();

    PointMatchAdder(const PointMatchAdder &) = delete;

    PointMatchAdder(PointMatchAdder &&) = delete;

    // information is getting lost here
    // I need to know if tracker position get flipped...
    PointMatchAdder & add(const Optional<PointMatch> &);

    Optional<SideToSideMapping> finish();

private:
    using ArrayType = std::array<PointMatch, 3>;

    ArrayType m_entries;
    ArrayType::iterator m_position;
};

// ------------------------- END OF PUBLIC INTERFACE --------------------------

template <TrianglePoint kt_lhs_addr, TrianglePoint kt_rhs_addr>
/* static */ PointMatch PointMatch::make
    (const TriangleSegment & lhs, const TriangleSegment & rhs)
{
    return PointMatch{point_of<kt_lhs_addr>(lhs), kt_lhs_addr,
                      point_of<kt_rhs_addr>(rhs), kt_rhs_addr};
}

template <TrianglePoint kt_addr>
/* static */ Vector PointMatch::point_of(const TriangleSegment & segment) {
    if constexpr (kt_addr == TrianglePoint::a)
        return segment.point_a();
    else if constexpr (kt_addr == TrianglePoint::b)
        return segment.point_b();
    return segment.point_c();
}

// ----------------------------------------------------------------------------

template <TrianglePoint kt_lhs_addr>
/* static */ PointMatchFinder PointMatchFinder::from_left_point
    (const TriangleSegment & lhs, const TriangleSegment & rhs)
{
    using Pt = TrianglePoint;
    return PointMatchFinder{{
        PointMatch::make<kt_lhs_addr, Pt::a>(lhs, rhs),
        PointMatch::make<kt_lhs_addr, Pt::b>(lhs, rhs),
        PointMatch::make<kt_lhs_addr, Pt::c>(lhs, rhs),
    }};
}
