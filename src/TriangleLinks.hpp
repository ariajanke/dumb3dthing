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

private:
    Triangle m_segment;
};

// what is a triangle link more generally... such that you only know about the
// triangle segment?
class TriangleLink final : public TriangleFragment {
public:
    using Side = TriangleSide;
#   if 0
    using Triangle = TriangleSegment;
#   endif
    struct Transfer final {
        SharedPtr<const TriangleLink> target; // set if there is a valid transfer to be had
        Side side = Side::k_inside; // transfer on what side of target?
        bool inverts = false; // normal *= -1
        bool flips = false; // true -> (1 - t)
    };

    explicit TriangleLink(const Triangle &);

    // atempts all sides
    TriangleLink & attempt_attachment_to(const SharedPtr<const TriangleLink> &);

    TriangleLink & attempt_attachment_to(const SharedPtr<const TriangleLink> &, Side);

    bool has_side_attached(Side) const;
#   if 0
    std::size_t hash() const noexcept
        { return std::hash<const Triangle *>{}(m_segment.get()); }
#   endif
#   if 0
    const Triangle & segment() const;
#   endif
#   if 0
    SharedPtr<Triangle> segment_ptr() const
        { return m_segment; }
#   endif
    Transfer transfers_to(Side) const;

    int sides_attached_count() const;
#   if 0
    // I need to be able to drop triangles, when their "true" owner is destroyed

    bool is_sole_owner() const noexcept
        { return m_segment.use_count() == 1; }

    int owner_count() const noexcept
        { return m_segment.use_count(); }
#   endif
private:
    struct SideInfo final {
        WeakPtr<const TriangleLink> target;
        Side side = Side::k_inside;
        bool inverts = false;
        bool flip = false;
    };

    static bool has_opposing_normals(const Triangle &, Side, const Triangle &, Side);

    static Side verify_valid_side(const char * caller, Side);
#   if 0
    Triangle m_segment;
#   endif
    std::array<SideInfo, 3> m_triangle_sides;
};
