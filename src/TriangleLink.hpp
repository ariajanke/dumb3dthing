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

#pragma once

#include "TriangleSegment.hpp"

// can represent anything which has a triangle
class TriangleFragment {
public:
    using Triangle = TriangleSegment;

    virtual ~TriangleFragment() {}

    const Triangle & segment() const { return m_segment; }

protected:
    TriangleFragment() {}

    explicit TriangleFragment(const Triangle & triangle):
        m_segment(triangle) {}

    TriangleFragment(const Vector & a, const Vector & b, const Vector & c):
        m_segment(Triangle(a, b, c)) {}

private:
    Triangle m_segment;
};

class VectorRotater final {
public:
    explicit VectorRotater(const Vector & axis_of_rotation);

    Vector operator () (const Vector & v, Real angle) const;

private:
    Vector m_axis_of_rotation;
};

class TriangleLinkTransfer final {
public:
    using Side = TriangleSide;
    TriangleLinkTransfer() {}

    TriangleLinkTransfer(SharedPtr<const TriangleLink> && target_,
                         Side side_,
                         bool inverts_normal_,
                         bool flips_position_):
        m_target(std::move(target_)),
        m_side(side_),
        m_inverts_normal(inverts_normal_),
        m_flips(flips_position_) {}

    const SharedPtr<const TriangleLink> & target() const
        { return m_target; }

    Side target_side() const { return m_side; }

    bool inverts_normal() const { return m_inverts_normal; }
    // this is unused?!
    bool flips_position() const { return m_flips; }

private:
    /// set if there is a valid transfer to be had
    SharedPtr<const TriangleLink> m_target;
    /// which side of the target did the tracker transfer to
    Side m_side = Side::k_inside;
    /// caller should flip normal vector of tracker
    bool m_inverts_normal = false;
    // true -> (1 - t)
    bool m_flips = false;
};

// a triangle link has a triangle, and is linked to other triangles
class TriangleLink final : public TriangleFragment {
public:
    using Side = TriangleSide;
    using Transfer = TriangleLinkTransfer;

    static bool has_matching_normals(const Triangle &, Side, const Triangle &, Side);

    static Real angle_of_rotation_for_left_to_right
        (const Vector & pivot, const Vector & left_opp, const Vector & right_opp,
         const VectorRotater & rotate_vec);

    static void reattach
        (const SharedPtr<TriangleLink> & lhs, Side lhs_side,
         const SharedPtr<TriangleLink> & rhs, Side rhs_side,
         bool inverts_normal,
         bool flips_position);

    static void reattach_matching_points
        (const SharedPtr<TriangleLink> & lhs,
         const SharedPtr<TriangleLink> & rhs);

    // still 95% broken...
    static void attach_matching_points
        (const SharedPtr<TriangleLink> & lhs,
         const SharedPtr<TriangleLink> & rhs);

    TriangleLink() {}

    explicit TriangleLink(const Triangle &);

    TriangleLink(const Vector & a, const Vector & b, const Vector & c);

    // attempts all sides
    [[deprecated]] TriangleLink & attempt_attachment_to
        (const SharedPtr<const TriangleLink> &);

    [[deprecated]] TriangleLink & attempt_attachment_to
        (const SharedPtr<const TriangleLink> &, Side);

    void set_transfer(Side on_side, Transfer && transfer_to);

    bool has_side_attached(Side) const;

    Transfer transfers_to(Side) const;

    int sides_attached_count() const;

private:
    struct SideInfo final {
        SideInfo() {}

        SideInfo(const WeakPtr<const TriangleLink> & target_,
                 Side side_,
                 bool inverts_,
                 bool flip_):
            target(target_), side(side_), inverts(inverts_), flip(flip_) {}

        WeakPtr<const TriangleLink> target;
        Side side = Side::k_inside;
        bool inverts = false;
        bool flip = false;
    };

    static Side verify_valid_side(const char * caller, Side);

    std::array<SideInfo, 3> m_triangle_sides;
};
