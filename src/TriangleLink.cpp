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

#include "geometric-utilities.hpp"

#include <iostream>

namespace {

using namespace cul::exceptions_abbr;
using Triangle = TriangleLink::Triangle;

void reattach
    (const SharedPtr<TriangleLink> & lhs,
     const SharedPtr<TriangleLink> & rhs,
     const TriangleLinkAttachment & attachment)
{
    // semantically icky, will need to be fixed
    lhs->set_transfer(attachment.left_side (), attachment.right_transfer());
    rhs->set_transfer(attachment.right_side(), attachment. left_transfer());
}

void attach
    (const SharedPtr<TriangleLink> & lhs,
     const SharedPtr<TriangleLink> & rhs,
     const TriangleLinkAttachment & attachment)
{
    if (lhs->has_side_attached(attachment.left_side ()) ||
        rhs->has_side_attached(attachment.right_side()))
    { return; }
    reattach(lhs, rhs, attachment);
}

} // end of <anonymous> namespace

// ----------------------------------------------------------------------------

/* static */ void TriangleLink::reattach
    (const SharedPtr<TriangleLink> & lhs, Side lhs_side,
     const SharedPtr<TriangleLink> & rhs, Side rhs_side,
     bool inverts_normal,
     bool flips_position)
{
    Transfer lhs_transfer
        {SharedPtr<TriangleLink>{lhs}, rhs_side, inverts_normal, flips_position};
    Transfer rhs_transfer
        {SharedPtr<TriangleLink>{rhs}, lhs_side, inverts_normal, flips_position};
    lhs->set_transfer(lhs_side, std::move(lhs_transfer));
    rhs->set_transfer(rhs_side, std::move(rhs_transfer));
}

/* static */ void TriangleLink::reattach_matching_points
    (const SharedPtr<TriangleLink> & lhs,
     const SharedPtr<TriangleLink> & rhs)
{
    if (auto attachment = TriangleLinkAttachment::find(lhs, rhs))
        { ::reattach(lhs, rhs, *attachment); }
}

/* static */ void TriangleLink::attach_matching_points
    (const SharedPtr<TriangleLink> & lhs,
     const SharedPtr<TriangleLink> & rhs)
{
    if (auto attachment = TriangleLinkAttachment::find(lhs, rhs))
        { attach(lhs, rhs, *attachment); }
}

TriangleLink::TriangleLink(const Triangle & triangle):
    TriangleFragment(triangle) {}

TriangleLink::TriangleLink(const Vector & a, const Vector & b, const Vector & c):
    TriangleFragment(a, b, c) {}

void TriangleLink::set_transfer(Side on_side, Transfer && transfer_to) {
    m_triangle_sides[on_side] =
        SideInfo{std::move(transfer_to.target()),
                 transfer_to.target_side(),
                 transfer_to.inverts_normal(),
                 transfer_to.flips_position()};
}

bool TriangleLink::has_side_attached(Side side) const {
    verify_valid_side("TriangleLinks::has_side_attached", side);
    return !!m_triangle_sides[side].target.lock();
}

TriangleLink::Transfer TriangleLink::transfers_to(Side side) const {
    verify_valid_side("TriangleLinks::transfers_to", side);
    const auto & info = m_triangle_sides[side];
    auto ptr = info.target.lock();
    if (!ptr) return Transfer{};
    return Transfer{std::move(ptr), info.side, info.inverts, info.flip};
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
