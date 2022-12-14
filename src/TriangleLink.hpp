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

// a triangle link has a triangle, and is linked to other triangles
class TriangleLink final : public TriangleFragment {
public:
    using Side = TriangleSide;

    struct Transfer final {
        /// set if there is a valid transfer to be had
        SharedPtr<const TriangleLink> target;
        /// which side of the target did the tracker transfer to
        Side side = Side::k_inside;
        /// caller should flip normal vector of tracker
        bool inverts_normal = false;
        // true -> (1 - t)
        bool flips = false;
    };

    static bool has_matching_normals(const Triangle &, Side, const Triangle &, Side);

    static Real angle_of_rotation_for_left_to_right
        (const Vector & pivot, const Vector & left_opp, const Vector & right_opp,
         const VectorRotater & rotate_vec);

    TriangleLink() {}

    explicit TriangleLink(const Triangle &);

    TriangleLink(const Vector & a, const Vector & b, const Vector & c);

    // attempts all sides
    TriangleLink & attempt_attachment_to(const SharedPtr<const TriangleLink> &);

    TriangleLink & attempt_attachment_to(const SharedPtr<const TriangleLink> &, Side);

    bool has_side_attached(Side) const;

    Transfer transfers_to(Side) const;

    int sides_attached_count() const;

private:
    struct SideInfo final {
        WeakPtr<const TriangleLink> target;
        Side side = Side::k_inside;
        bool inverts = false;
        bool flip = false;
    };

    static Side verify_valid_side(const char * caller, Side);

    std::array<SideInfo, 3> m_triangle_sides;
};
