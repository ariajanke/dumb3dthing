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

#include "Defs.hpp"
#include "TriangleSegment.hpp"

#include <ariajanke/cul/VectorUtils.hpp>

/** Gets the "nextafter" vector following a given direction
 *  @param r starting position
 *  @param dir direction to go in
 *  @return vector with the smallest possible difference from r in the
 *          direction of dir
 *  @note gotcha here with TriangleSurface::point_at it may not necessarily be
 *        the case that for some given TriangleSurface ts, Vector r, Vector dir
 *        that:
 *        (ts.point_at(r) - ts.point_at(next_in_direction(r, dir) == Vector(0, 0, 0))
 */
Vector next_in_direction(Vector r, Vector dir);

/** @copydoc next_in_direction(Vector,Vector) */
Vector2 next_in_direction(Vector2 r, Vector2 dir);

template <typename T>
using EnableBoolIfVec = std::enable_if_t<cul::k_is_vector_type<T>, bool>;
// didn't need to move this...
template <typename Vec>
EnableBoolIfVec<Vec> are_parallel(const Vec & a, const Vec & b);

template <typename VecT>
class LineSegmentIntersection final {
public:
    using Scalar = typename cul::VectorTraits<VecT>::ScalarType;
    using VectorType = VecT;

    static constexpr
        cul::EnableIf<cul::VectorTraits<VecT>::k_dimension_count == 2,
                      LineSegmentIntersection>
        find
        (const VecT & a_first, const VecT & a_second,
         const VecT & b_first, const VecT & b_second);

    constexpr LineSegmentIntersection() {}

    constexpr LineSegmentIntersection
        (Scalar denominator,
         Scalar numerator,
         const VectorType & first,
         const VectorType & second);

    constexpr VectorType value() const;

    constexpr bool has_value() const { return m_is_defined; }

    constexpr VectorType operator * () const { return value(); }

    constexpr explicit operator bool() const { return m_is_defined; }

private:
    using Helpers = cul::VecOpHelpers<VectorType>;

    bool m_is_defined = false;
    Scalar m_denom = 0, m_numer = 0;
    VectorType m_first, m_second;
};

template <typename Vec>
constexpr cul::EnableIf<
    cul::VectorTraits<Vec>::k_dimension_count == 2,
    LineSegmentIntersection<Vec>>
    find_intersection
    (const Vec & a_first, const Vec & a_second,
     const Vec & b_first, const Vec & b_second);

class TriangleLinkTransfer;

class TriangleLinkAttachment final {
public:
    static Optional<TriangleLinkAttachment> find
        (const SharedPtr<const TriangleLink> & lhs,
         const SharedPtr<const TriangleLink> & rhs);

    TriangleLinkAttachment();

    TriangleLinkAttachment
        (SharedPtr<const TriangleLink> lhs_,
         SharedPtr<const TriangleLink> rhs_,
         TriangleSide lhs_side_,
         TriangleSide rhs_side_,
         bool has_matching_normals_,
         bool flips_position_);

    TriangleLinkTransfer left_transfer() const;

    TriangleLinkTransfer right_transfer() const;

    TriangleSide left_side() const;

    TriangleSide right_side() const;

private:
    TriangleLinkTransfer make_on_side
        (const SharedPtr<const TriangleLink> & link,
         TriangleSide side) const;

    SharedPtr<const TriangleLink> m_lhs;
    SharedPtr<const TriangleLink> m_rhs;
    TriangleSide m_lhs_side, m_rhs_side;
    bool m_has_matching_normals;
    bool m_flips_position;
};

// ------------------------- END OF PUBLIC INTERFACE --------------------------

template <typename Vec>
EnableBoolIfVec<Vec> are_parallel(const Vec & a, const Vec & b) {
#   if 0 // gdb *really* doesn't like me short circutting functions :c
    auto mag = magnitude(normalize(a) + normalize(b));
    return are_very_close(mag, 0) || are_very_close(mag, 2);
#   elif 1
    auto mk_zero = [] {
        constexpr auto k_dim = cul::VectorTraits<Vec>::k_dimension_count;
        static_assert(k_dim == 2 || k_dim == 3,
            "<anonymous>::are_parallel: must be used only with 2 or 3 dimensions.");
        if constexpr (k_dim == 2) return 0;
        else return cul::make_zero_vector<Vec>();
    };
    return are_very_close(mk_zero(), cross(a, b));
#   else
    auto frac = (dot(a, b)*dot(a, b)) / (sum_of_squares(a)*sum_of_squares(b));
    return are_very_close(magnitude(frac), 1);
#   endif
}

template <typename Vec>
constexpr cul::EnableIf<
    cul::VectorTraits<Vec>::k_dimension_count == 2,
    LineSegmentIntersection<Vec>>
    find_intersection
    (const Vec & a_first, const Vec & a_second,
     const Vec & b_first, const Vec & b_second)
{
    return LineSegmentIntersection<Vec>::find
        (a_first, a_second, b_first, b_second);
}

// ----------------------------------------------------------------------------

template <typename VecT>
/* static */ constexpr
    cul::EnableIf<cul::VectorTraits<VecT>::k_dimension_count == 2,
                  LineSegmentIntersection<VecT>>
    LineSegmentIntersection<VecT>::find
    (const VecT & a_first, const VecT & a_second,
     const VecT & b_first, const VecT & b_second)
{
    auto no_intersection = [] { return LineSegmentIntersection{}; };
    auto sub = [](auto && ... args)
        { return Helpers::template sub<0>(args...); };

    auto p = a_first;
    auto r = sub(a_second, p);

    auto q = b_first;
    auto s = sub(b_second, q);

    auto r_cross_s = cross(r, s);
    if (r_cross_s == 0) return no_intersection();

    auto q_sub_p = sub(q, p);
    auto between_a_first_second = cross(q_sub_p, s);

    auto outside_0_1 = [](const Scalar & num, const Scalar & denom)
        { return num*denom < 0 || magnitude(num) > magnitude(denom); };

    // overflow concerns?
    if (outside_0_1(between_a_first_second, r_cross_s))
        return no_intersection();

    auto between_b_first_second = cross(q_sub_p, r);
    if (outside_0_1(between_b_first_second, r_cross_s))
        return no_intersection();

    return LineSegmentIntersection{r_cross_s, between_a_first_second, p, r};
}

template <typename VecT>
constexpr LineSegmentIntersection<VecT>::LineSegmentIntersection
    (Scalar denom,
     Scalar numer,
     const VectorType & first,
     const VectorType & second):
    m_is_defined(true),
    m_denom(denom),
    m_numer(numer),
    m_first(first),
    m_second(second) {}

template <typename VecT>
constexpr typename LineSegmentIntersection<VecT>::VectorType
    LineSegmentIntersection<VecT>::value() const
{
    using cul::exceptions_abbr::RtError;
    if (!m_is_defined) {
        throw RtError{"intersection is undefined (there is none)"};
    }
    return Helpers::template plus<0>(
        m_first,
        Helpers::template div<0>
            (Helpers::template mul<0>(m_second, m_numer), m_denom));
}
