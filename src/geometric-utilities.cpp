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

// ----------------------------------------------------------------------------

/* static */ Optional<TriangleLinkAttachment> TriangleLinkAttachment::find
    (const SharedPtr<const TriangleLink> & lhs,
     const SharedPtr<const TriangleLink> & rhs)
{
    auto point_match = PointMatchAdder::find_point_match(lhs, rhs);
    if (!point_match) return {};
    auto matching_normals = TriangleLink::has_matching_normals
        (lhs->segment(), point_match->left_side(),
         rhs->segment(), point_match->right_side());
    return TriangleLinkAttachment
        {lhs, rhs, point_match->left_side(), point_match->right_side(),
         matching_normals, point_match->sides_flip()};
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

/* private */
    TriangleLinkTransfer TriangleLinkAttachment::make_on_side
        (const SharedPtr<const TriangleLink> & link,
         TriangleSide side) const
    {
        return TriangleLinkTransfer
            {SharedPtr<const TriangleLink>{link}, side,
             m_has_matching_normals, m_flips_position};
    }
