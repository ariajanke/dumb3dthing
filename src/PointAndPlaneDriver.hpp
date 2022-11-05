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
#include "TriangleLink.hpp"

namespace point_and_plane {

using Triangle = TriangleSegment;

struct OnSegment final {
    OnSegment() {}

    OnSegment(const SharedPtr<const TriangleFragment> & tri_, bool invert_norm_,
              Vector2 loc_, Vector2 dis_);

    SharedPtr<const TriangleFragment> fragment;
    const Triangle * segment = nullptr;
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
    using SideCrossing = Triangle::SideCrossing;

    static UniquePtr<EventHandler> make_test_handler();

    virtual ~EventHandler() {}

    /** @brief Called when a PpState hits a triangle segment.
     *
     *  @param tri triangle the PpState is colliding against
     *  @param ouside position immediately before hiting the triangle
     *  @param inside position on the triangle segment that's been hit
     *         (parameter tri)
     *  @param next what the end location of the PpState would be if the
     *         triangle segment wasn't there
     *  @return
     */
    virtual Variant<Vector2, Vector>
        on_triangle_hit
        (const Triangle & tri, const Vector & ouside, const Vector2 & inside,
         const Vector & next) const = 0;

    /** @brief Called when a failed transfer occured due to the absence of a
     *         linked segment
     *
     *  @param tri triangle presently occupied by the PpState
     *  @param cross describes the side crossing occuring
     *  @param projected_new_location what the new location of the PpState
     *         would be if the segment extended forever
     *
     *  @return indicates the new remaining displacement
     *          - (3D) Vector to mean an InAir state
     *          - Vector2 to mean "stay on this segment" indicating the new
     *          If 3D vector is indicated then the new PpState's location will
     *          be decidely outside the triangle segment. If 2D, the location
     *          will be on the inside of the segment.
     */
    virtual Variant<Vector, Vector2>
        on_transfer_absent_link
        (const Triangle & tri, const SideCrossing & cross,
         const Vector2 & projected_new_location) const = 0;

    /** @brief Called when a transfer from one segment to another may occur
     *
     *  @param original the original triangle segment which the PpState is on
     *  @param cross describes the side crossing occuring
     *  @param next the segment the PpState may transfer to
     *  @param projected_new_loaction what the new location would be if the
     *         PpState were to stay on the segment and if that segment were
     *         infinite
     *  @return indicates the new remaining displacement
     *          - (3D) Vector to mean an InAir state
     *          - Vector2 to mean "commence with the transfer" indicating the
     *          If 3D vector is indicated then the new PpState's location will
     *          be decidely outside the triangle segment. If 2D, the location
     *          will be on the inside of the segment.
     */
    virtual Variant<Vector, Tuple<bool, Vector2>>
        on_transfer
        (const Triangle & original, const SideCrossing & cross,
         const Triangle & next, const Vector & projected_new_loaction) const = 0;
};

class Driver {
public:
    static UniquePtr<Driver> make_driver();

    virtual ~Driver() {}

    virtual void add_triangle(const SharedPtr<const TriangleLink> &) = 0;

    void add_triangles(const std::vector<SharedPtr<TriangleLink>> & links) {
        for (auto & link : links) add_triangle(link);
    }

    void add_triangles(const std::vector<SharedPtr<const TriangleLink>> & links) {
        for (auto & link : links) add_triangle(link);
    }

    virtual void remove_triangle(const SharedPtr<const TriangleLink> &) = 0;

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
