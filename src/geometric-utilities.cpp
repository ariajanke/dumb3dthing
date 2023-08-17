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

#include "geometric-utilities.hpp"

#include "TriangleLink.hpp"

#include "geometric-utilities/PointMatchAdder.hpp"

Vector next_in_direction(const Vector & r, const Vector & dir) {
    return Vector{std::nextafter(r.x, r.x + dir.x),
                  std::nextafter(r.y, r.y + dir.y),
                  std::nextafter(r.z, r.z + dir.z)};
}

Vector2 next_in_direction(const Vector2 & r, const Vector2 & dir) {
    return Vector2{std::nextafter(r.x, r.x + dir.x),
                   std::nextafter(r.y, r.y + dir.y)};
}

// ----------------------------------------------------------------------------

VectorRotater::VectorRotater(const Vector & axis_of_rotation):
    m_axis_of_rotation(normalize(axis_of_rotation)) {}

Vector VectorRotater::operator () (const Vector & v, Real angle) const {
    // and so follows... Rodrigues' formula
    using std::sin, std::cos;
    auto cos_t = cos(angle);
    return   v*cos_t
           + cross(m_axis_of_rotation, v)*sin(angle)
           + m_axis_of_rotation*dot(m_axis_of_rotation, v)*(1 - cos_t);
}

// ----------------------------------------------------------------------------

/* static */ Optional<TriangleLinkAttachment> TriangleLinkAttachment::find
    (const SharedPtr<const TriangleLink> & lhs,
     const SharedPtr<const TriangleLink> & rhs)
{
    auto point_match = PointMatchAdder::find_point_match
        (lhs->segment(), rhs->segment());
    if (!point_match) return {};
    auto left_side = point_match->left_side();
    auto right_side = point_match->right_side();
    auto matching_normals = has_matching_normals
        (lhs->segment(), left_side, rhs->segment(), right_side);
    return TriangleLinkAttachment
        {lhs, rhs, left_side, right_side,
         matching_normals, point_match->sides_flip()};
}

/* static */ bool TriangleLinkAttachment::has_matching_normals
    (const TriangleSegment & lhs, TriangleSide left_side,
     const TriangleSegment & rhs, TriangleSide right_side)
{
#   ifdef MACRO_DEBUG
    // assumption, sides "line up"
    assert(([&] {
        auto [la, lb] = lhs.side_points(left_side);
        auto [ra, rb] = rhs.side_points(right_side);
        return    (are_very_close(la, ra) && are_very_close(lb, rb))
               || (are_very_close(la, rb) && are_very_close(lb, ra));
    } ()));
#   endif

    // Taking the two triangles, lined up. Then projecting onto the plane where
    // the joining line is used as the normal for that plane.
    auto [la, lb] = lhs.side_points(left_side);

    // v doesn't necessarily need to be a normal vector
    auto plane_v = lb - la;

    // first... project everything relevant onto plane orthogonal to rotation
    // axis
    // this gives us three vectors, two line segments
    // we want the angles between them
    // the pivot is where they join
    auto left_opp  = project_onto_plane(lhs.opposing_point(left_side ), plane_v);
    auto right_opp = project_onto_plane(rhs.opposing_point(right_side), plane_v);
    auto pivot     = project_onto_plane(la                            , plane_v);

    // get possible rotations
    // Now there's a possible problem here... what if the previous projection
    // ends up landing right on the pivot?...
    VectorRotater rotate_vec{plane_v};

    // do I even need a directed rotation for this?
    // YES but it doesn't matter which solution we choose
    auto angle_for_lhs = angle_of_rotation_for_left_to_right
        (pivot, left_opp, right_opp, rotate_vec);
    auto rotated_lhs_normal = rotate_vec(lhs.normal(), angle_for_lhs);
#   if 0
    std::cout << "plane v " << plane_v << "\n"
              << "lo  " << left_opp << "\n"
              << "ro  " << right_opp << "\n"
              << "piv " << pivot << "\n"
              << "lnm " << lhs.normal() << "\n"
              << "rnm " << rhs.normal() << "\n"
              << "rot " << rotated_lhs_normal << "\n"
              << "res " << dot(rotated_lhs_normal, rhs.normal()) << std::endl;
#   endif
    return dot(rotated_lhs_normal, rhs.normal()) > 0;
}

/* static */ Real TriangleLinkAttachment::angle_of_rotation_for_left_to_right
    (const Vector & pivot, const Vector & left_opp, const Vector & right_opp,
     const VectorRotater & rotate_vec)
{
    auto piv_to_left  = left_opp  - pivot;
    auto piv_to_right = right_opp - pivot;

    // only one of these solutions is correct, because we need the right
    // direction
    auto t0 = angle_between(piv_to_left, piv_to_right);
    auto t1 = -t0;

    auto sol0 = rotate_vec(piv_to_left, t0);
    auto sol1 = rotate_vec(piv_to_left, t1);

    // greatest is closest
    if (dot(sol0, piv_to_right) > dot(sol1, piv_to_right)) {
#       if 0
        std::cout << "Choose t0: " << t0 << std::endl;
#       endif
        return t0;
    }
#   if 0
    std::cout << "Choose t1: " << t1 << std::endl;
#   endif
    return t1;
}

TriangleLinkAttachment::TriangleLinkAttachment() {}

TriangleLinkAttachment::TriangleLinkAttachment
    (SharedPtr<const TriangleLink> lhs_,
     SharedPtr<const TriangleLink> rhs_,
     TriangleSide lhs_side_,
     TriangleSide rhs_side_,
     bool has_matching_normals_,
     bool flips_position_):
    m_lhs(lhs_),
    m_rhs(rhs_),
    m_lhs_side(lhs_side_),
    m_rhs_side(rhs_side_),
    m_has_matching_normals(has_matching_normals_),
    m_flips_position(flips_position_) {}

TriangleLinkTransfer TriangleLinkAttachment::left_transfer() const
    { return make_on_side(m_lhs, m_rhs_side); }

TriangleLinkTransfer TriangleLinkAttachment::right_transfer() const
    { return make_on_side(m_rhs, m_lhs_side); }

TriangleSide TriangleLinkAttachment::left_side() const { return m_lhs_side; }

TriangleSide TriangleLinkAttachment::right_side() const { return m_rhs_side; }

/* private */ TriangleLinkTransfer TriangleLinkAttachment::make_on_side
    (const SharedPtr<const TriangleLink> & link, TriangleSide side) const
{
    return TriangleLinkTransfer
        {SharedPtr<const TriangleLink>{link}, side,
         m_has_matching_normals, m_flips_position};
}
