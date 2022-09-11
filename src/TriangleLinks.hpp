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

class TriangleLinks final {
public:
    using Side = TriangleSide;
    using Triangle = TriangleSegment;

    struct Transfer final {
        SharedCPtr<Triangle> target; // set if there is a valid transfer to be had
        Side side = Side::k_inside; // transfer on what side of target?
        bool inverts = false; // normal *= -1
        bool flips = false; // true -> (1 - t)
    };

    explicit TriangleLinks(SharedCPtr<Triangle>);

    // atempts all sides
    TriangleLinks & attempt_attachment_to(SharedCPtr<Triangle>);

    TriangleLinks & attempt_attachment_to(SharedCPtr<Triangle>, Side);

    bool has_side_attached(Side) const;

    std::size_t hash() const noexcept
        { return std::hash<const Triangle *>{}(m_segment.get()); }

    const Triangle & segment() const;

    SharedCPtr<Triangle> segment_ptr() const
        { return m_segment; }

    Transfer transfers_to(Side) const;

    int sides_attached_count() const {
        auto list = { Side::k_side_ab, Side::k_side_bc, Side::k_side_ca };
        return std::count_if(list.begin(), list.end(), [this](Side side)
            { return has_side_attached(side); });
    }

    bool is_sole_owner() const noexcept
        { return m_segment.use_count() == 1; }

private:
    struct SideInfo final {
        WeakCPtr<Triangle> target;
        Side side = Side::k_inside;
        bool inverts = false;
        bool flip = false;
    };

    static bool has_opposing_normals(const Triangle &, Side, const Triangle &, Side);

    static Side verify_valid_side(const char * caller, Side);

    SharedCPtr<Triangle> m_segment;

    std::array<SideInfo, 3> m_triangle_sides;
};
