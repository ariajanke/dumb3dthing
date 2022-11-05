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
    explicit TriangleFragment(const Triangle & triangle):
        m_segment(triangle) {}

    TriangleFragment(const Vector & a, const Vector & b, const Vector & c):
        m_segment(Triangle(a, b, c))
    {}

private:
    Triangle m_segment;
};

// a triangle link has a triangle, and is linked to other triangles
class TriangleLink final : public TriangleFragment {
public:
    using Side = TriangleSide;

    struct Transfer final {
        SharedPtr<const TriangleLink> target; // set if there is a valid transfer to be had
        Side side = Side::k_inside; // transfer on what side of target?
        bool inverts = false; // normal *= -1
        bool flips = false; // true -> (1 - t)
    };

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

    static bool has_opposing_normals(const Triangle &, Side, const Triangle &, Side);

    static Side verify_valid_side(const char * caller, Side);

    std::array<SideInfo, 3> m_triangle_sides;
};
