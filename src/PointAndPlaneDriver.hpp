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

#include "Defs.hpp"
#include "TriangleSegment.hpp"

namespace point_and_plane {

using Triangle = TriangleSegment;

class TriangleLinks final {
public:
    using Side = TriangleSide;

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

    static void run_tests();

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

struct OnSurface final {
    OnSurface() {}

    OnSurface(SharedCPtr<Triangle> tri_, bool invert_norm_,
              Vector2 loc_, Vector2 dis_):
        segment(tri_), invert_normal(invert_norm_),
        location(loc_), displacement(dis_)
    {
        // there's state validation...
        // surface; must not be nullptr
        //
        assert(tri_);
        assert(tri_->contains_point(loc_));
    }

    SharedCPtr<Triangle> segment;
    bool invert_normal = false;
    Vector2 location;
    Vector2 displacement;
};

struct InAir final {
    InAir() {}
    InAir(Vector loc_, Vector displac_):
        location(loc_), displacement(displac_) {}
    Vector location;
    Vector displacement;
};

using State = std::variant<InAir, OnSurface>;

Vector location_of(const State &);

class EventHandler {
public:
    struct SegmentTransfer {
        SegmentTransfer(const SegmentTransfer &) = default;
        SegmentTransfer(Tuple<bool, Vector2> && tup):
            invert(std::get<0>(tup)), displacement(std::get<1>(tup))
        {}
        bool invert = false;
        Vector2 displacement;
    };

    virtual ~EventHandler() {}

    // in either case: displacement is being returned
    // return a Vector2 -> you mean to attach to the triangle
    // return a Vector -> you mean to maintain a freebody state and fail to
    // attach to the triangle
    virtual Variant<Vector2, Vector> displacement_after_triangle_hit
        (const Triangle &, const Vector & location, const Vector & next,
         const Vector & intersection) const = 0;

    virtual Variant<SegmentTransfer, Vector> pass_triangle_side
        (const Triangle & from, const Triangle * to,
         const Vector & location, const Vector & remaining_displacement) const = 0;

    virtual bool cling_to_edge(const Triangle &, TriangleSide side_hit) const = 0;
};

class Driver {
public:
    static UniquePtr<Driver> make_driver();

    virtual ~Driver() {}

    virtual void add_triangle(const TriangleLinks &) = 0;

    void add_triangles(const std::vector<TriangleLinks> & links) {
        for (auto & link : links) add_triangle(link);
    }

    virtual void remove_triangle(const TriangleLinks &) = 0;

    virtual Driver & update() = 0;

    virtual void clear_all_triangles() = 0;

    // the point is to consume the displacement vector
    virtual State operator () (const State &, const EventHandler &) const = 0;

protected:
    Driver() {}
};

} // end of point_and_plane namespace

using PpState = point_and_plane::State;
using PpInAir = point_and_plane::InAir;
using PpOnSurface = point_and_plane::OnSurface;
using point_and_plane::TriangleLinks;
