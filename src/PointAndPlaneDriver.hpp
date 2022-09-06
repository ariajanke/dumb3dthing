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
#include "TriangleLinks.hpp"

namespace point_and_plane {

using Triangle = TriangleSegment;

struct OnSegment final {
    OnSegment() {}

    OnSegment(SharedCPtr<Triangle> tri_, bool invert_norm_,
              Vector2 loc_, Vector2 dis_);

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

using State = Variant<InAir, OnSegment>;

Vector location_of(const State &);

inline Vector segment_displacement_to_v3(const State & state) {
    using std::get;
    auto pt_at = [&state](Vector2 r)
        { return get<OnSegment>(state).segment->point_at(r); };
    auto dis = get<OnSegment>(state).displacement;
    auto loc = get<OnSegment>(state).location;
    return pt_at(loc + dis) - pt_at(loc);
};

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

    static UniquePtr<EventHandler> make_test_handler();

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
using PpOnSegment = point_and_plane::OnSegment;
