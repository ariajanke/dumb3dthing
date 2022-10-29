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

class TriangleLinksAttn;

class TriangleLinks final {
public:
    friend class TriangleLinksAttn;
    using Side = TriangleSide;
    using Triangle = TriangleSegment;

    struct Transfer final {
        SharedPtr<const Triangle> target; // set if there is a valid transfer to be had
        Side side = Side::k_inside; // transfer on what side of target?
        bool inverts = false; // normal *= -1
        bool flips = false; // true -> (1 - t)
    };

    explicit TriangleLinks(const SharedPtr<const Triangle> &);

    // atempts all sides
    TriangleLinks & attempt_attachment_to(const SharedPtr<const Triangle> &);

    TriangleLinks & attempt_attachment_to(const SharedPtr<const Triangle> &, Side);

    bool has_side_attached(Side) const;

    std::size_t hash() const noexcept
        { return std::hash<const Triangle *>{}(m_segment.get()); }

    const Triangle & segment() const;

    SharedPtr<const Triangle> segment_ptr() const
        { return m_segment; }

    Transfer transfers_to(Side) const;

    int sides_attached_count() const;

    bool is_sole_owner() const noexcept
        { return m_segment.use_count() == 1; }

    int owner_count() const noexcept
        { return m_segment.use_count(); }

private:
    struct SideLinks final {
        WeakPtr<const Triangle> target;
        Side side = Side::k_inside;
        bool inverts = false;
        bool flip = false;
    };
    using SideLinksArray = std::array<SideLinks, 3>;

    static bool has_opposing_normals(const Triangle &, Side, const Triangle &, Side);

    static Side verify_valid_side(const char * caller, Side);

    SharedPtr<const Triangle> m_segment;

    SideLinksArray m_triangle_sides;
};

class WeakTriangleLinks;

class TriangleLinksAttn final {
    friend class WeakTriangleLinks;
    using Triangle = TriangleSegment;
    using SideLinks = TriangleLinks::SideLinks;
    using SideLinksArray = TriangleLinks::SideLinksArray;

    static TriangleLinks make_links
        (SharedPtr<const Triangle>, const SideLinksArray & links);

    static SharedPtr<const Triangle> get_triangle_pointer
        (const TriangleLinks & links)
    { return links.m_segment; }

    static SideLinksArray get_links_array(const TriangleLinks & links)
        { return links.m_triangle_sides; }
};

class WeakTriangleLinks final {
public:
    using Triangle = TriangleSegment;

    explicit WeakTriangleLinks(const TriangleLinks & links);

    TriangleLinks lock_links() const;

    bool expired() const noexcept
        { return m_segment.expired(); }

private:
    using SideLinksArray = TriangleLinksAttn::SideLinksArray;
    WeakPtr<const Triangle> m_segment;
    SideLinksArray m_links;
};
