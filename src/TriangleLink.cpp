/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "TriangleLink.hpp"

#include <iostream>

namespace {

using namespace cul::exceptions_abbr;
using Triangle = TriangleLink::Triangle;

} // end of <anonymous> namespace

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

/* static */ Real TriangleLink::angle_of_rotation_for_left
    (const Vector & pivot, const Vector & left_opp, const Vector & right_opp,
     const VectorRotater & rotate_vec)
{
    auto piv_to_left  = left_opp  - pivot;
    auto piv_to_right = right_opp - pivot;
#   if 1
    // only one of these solutions is correct, because we need the right
    // direction
    auto t0 = angle_between(piv_to_left, piv_to_right);
#   if 0
    auto t1 = t0 - k_pi;
#   endif
    auto t1 = k_pi*2 - t0;

    auto sol0 = rotate_vec(piv_to_left, t0);
    auto sol1 = rotate_vec(piv_to_left, t1);

    // greatest is closest
    if (dot(sol0, piv_to_right) > dot(sol1, piv_to_right)) {
        std::cout << "Choose t0: " << t0 << std::endl;
        return t0;
    }
    std::cout << "Choose t1: " << t1 << std::endl;
    return t1;
#   else
    auto dp = dot(piv_to_left, piv_to_right);
    if (dp < 0) {

    } else {

    }

#   endif
}

/* static */ bool TriangleLink::has_matching_normals
    (const Triangle & lhs, Side left_side, const Triangle & rhs, Side right_side)
{
    // assumption, sides "line up"
    assert(([&] {
        auto [la, lb] = lhs.side_points(left_side);
        auto [ra, rb] = rhs.side_points(right_side);
        return    (are_very_close(la, ra) && are_very_close(lb, rb))
               || (are_very_close(la, rb) && are_very_close(lb, ra));
    } ()));

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
    auto angle_for_lhs = angle_of_rotation_for_left
        (pivot, left_opp, right_opp, rotate_vec);
    auto rotated_lhs_normal = rotate_vec(lhs.normal(), angle_for_lhs);

    std::cout << "plane v " << plane_v << "\n"
              << "lo  " << left_opp << "\n"
              << "ro  " << right_opp << "\n"
              << "piv " << pivot << "\n"
              << "lnm " << lhs.normal() << "\n"
              << "rnm " << rhs.normal() << "\n"
              << "rot " << rotated_lhs_normal << "\n"
              << "res " << dot(rotated_lhs_normal, rhs.normal()) << std::endl;

    return dot(rotated_lhs_normal, rhs.normal()) > 0;
}

TriangleLink::TriangleLink(const Triangle & triangle):
    TriangleFragment(triangle)
{}

TriangleLink::TriangleLink(const Vector & a, const Vector & b, const Vector & c):
    TriangleFragment(a, b, c)
{}

TriangleLink & TriangleLink::attempt_attachment_to
    (const SharedPtr<const TriangleLink> & tptr)
{
    return  attempt_attachment_to(tptr, Side::k_side_ab)
           .attempt_attachment_to(tptr, Side::k_side_bc)
           .attempt_attachment_to(tptr, Side::k_side_ca);
}

TriangleLink & TriangleLink::attempt_attachment_to
    (const SharedPtr<const TriangleLink> & other, Side other_side)
{
    verify_valid_side("TriangleLinks::attempt_attachment_to", other_side);
    auto [oa, ob] = other->segment().side_points(other_side); {}
    for (auto this_side : { Side::k_side_ab, Side::k_side_bc, Side::k_side_ca }) {
        auto [ta, tb] = segment().side_points(this_side);
        bool has_flipped_points    = are_very_close(oa, tb) && are_very_close(ob, ta);
        bool has_nonflipped_points = are_very_close(oa, ta) && are_very_close(ob, tb);
        if (!has_flipped_points && !has_nonflipped_points) continue;

        auto & link_transfer = m_triangle_sides[this_side];
        link_transfer.flip = has_flipped_points;
        link_transfer.side = other_side;
        link_transfer.inverts = has_matching_normals
            (other->segment(), other_side, segment(), this_side);
        link_transfer.target = other;
        break;
    }
    return *this;
}

bool TriangleLink::has_side_attached(Side side) const {
    verify_valid_side("TriangleLinks::has_side_attached", side);
    return !!m_triangle_sides[side].target.lock();
}

TriangleLink::Transfer TriangleLink::transfers_to(Side side) const {
    verify_valid_side("TriangleLinks::transfers_to", side);
    const auto & info = m_triangle_sides[side];
    Transfer rv;
    rv.flips = info.flip;
    rv.inverts_normal = info.inverts;
    rv.side = info.side;
    rv.target = info.target.lock();
    return rv;
}

int TriangleLink::sides_attached_count() const {
    auto list = { Side::k_side_ab, Side::k_side_bc, Side::k_side_ca };
    return std::count_if(list.begin(), list.end(), [this](Side side)
        { return has_side_attached(side); });
}

/* private static */ TriangleSide TriangleLink::verify_valid_side
    (const char * caller, Side side)
{
    switch (side) {
    case Side::k_side_ab: case Side::k_side_bc: case Side::k_side_ca:
        return side;
    default: break;
    }
    throw InvArg{  std::string{caller}
                 + ": side must be valid value and not k_inside."};
}
