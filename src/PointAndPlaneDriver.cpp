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

#include "PointAndPlaneDriver.hpp"

#include <common/TestSuite.hpp>

#include <unordered_map>
#include <iostream>

namespace {

#define MACRO_MAKE_BAD_BRANCH_EXCEPTION() BadBranchException(__LINE__, __FILE__)

using namespace cul::exceptions_abbr;
using namespace point_and_plane;
using std::get;
using cul::find_smallest_diff, cul::is_solution, cul::project_onto,
      cul::sum_of_squares, cul::EnableIf;
#if 0
using SegmentTransfer = EventHandler::SegmentTransfer;
#endif
class DriverComplete final : public Driver {
public:
    void add_triangle(const TriangleLinks &) final;

    void remove_triangle(const TriangleLinks &) final;

    void clear_all_triangles() final;

    Driver & update() final;

    State operator () (const State &, const EventHandler &) const final;

private:
    // the job of each method here is to reduce displacement
    State handle_freebody(const InAir &, const EventHandler &) const;

    State handle_tracker(const OnSegment &, const EventHandler &) const;

    const TriangleLinks & find_links_for(SharedPtr<const Triangle>) const;

    // both containers are owning
    std::unordered_map<std::size_t, TriangleLinks> m_links;
    std::vector<SharedPtr<const Triangle>> m_triangles;
};

} // end of <anonymous> namespace

namespace point_and_plane {

OnSegment::OnSegment
    (SharedCPtr<Triangle> tri_, bool invert_norm_, Vector2 loc_, Vector2 dis_):
    segment(tri_), invert_normal(invert_norm_),
    location(loc_), displacement(dis_)
{
    // there's state validation...
    // surface; must not be nullptr
    //
    assert(tri_);
    if (!tri_->contains_point(loc_)) {
        std::cerr << loc_ << " " 
                  << tri_->point_a_in_2d() << " " 
                  << tri_->point_b_in_2d() << " " 
                  << tri_->point_c_in_2d() << std::endl;
    }
    assert(tri_->contains_point(loc_));
}

Vector location_of(const State & state) {
    auto * in_air = get_if<InAir>(&state);
    if (in_air) return in_air->location;
    auto & on_surf = std::get<OnSegment>(state);
    return on_surf.segment->point_at(on_surf.location);
}

/* static */ UniquePtr<EventHandler> EventHandler::make_test_handler() {
    class TestHandler final : public point_and_plane::EventHandler {
        Variant<Vector2, Vector>
            on_triangle_hit
            (const Triangle &, const Vector &, const Vector2 &, const Vector &) const final
        { return Vector2{}; }

        Variant<Vector, Vector2>
            on_transfer_absent_link
            (const Triangle &, const SideCrossing &, const Vector2 &) const final
        { return Vector{}; }

        Variant<Vector, Tuple<bool, Vector2>>
            on_transfer
            (const Triangle &, const Triangle::SideCrossing &,
             const Triangle &, const Vector &) const final
        { return make_tuple(true, Vector2{}); }

#       if 0
        Variant<Vector2, Vector> displacement_after_triangle_hit
            (const Triangle &, const Vector &,
             const Vector &, const Vector &) const final
        {
            // always land
            return Vector2{};
        }

        Variant<SegmentTransfer, Vector> pass_triangle_side
            (const Triangle &, const Triangle * to,
             const Vector &, const Vector &) const final
        {
            // always fall off
            if (!to) return Vector{};
            // always transfer
            return make_tuple(false, Vector2{});
        }

        bool cling_to_edge(const Triangle &, TriangleSide) const final
            { return false; }
#       endif
    };

    return make_unique<TestHandler>();
}

/* static */ UniquePtr<Driver> Driver::make_driver()
    { return UniquePtr<Driver>{make_unique<DriverComplete>()}; }

} // end of point_and_plane namespace


namespace {

template <typename Vec1, typename Vec2>
void verify_decreasing_displacement
    (EnableIf<cul::VectorTraits<Vec1>::k_is_vector_type, const Vec1 &> displc,
     EnableIf<cul::VectorTraits<Vec2>::k_is_vector_type, const Vec2 &> old_displacement,
     const char * caller);

inline std::size_t to_key(const SharedCPtr<Triangle> & tptr)
    { return std::hash<const Triangle *>{}(tptr.get()); }

// should/add remove fast
void DriverComplete::add_triangle(const TriangleLinks & links) {
    m_links.insert(std::make_pair( links.hash(), links ));
}

void DriverComplete::remove_triangle(const TriangleLinks & links) {
    m_links.erase(links.hash());
}

void DriverComplete::clear_all_triangles() {
    m_triangles.clear();
    m_links.clear();
}

Driver & DriverComplete::update() {
    m_triangles.clear();
    m_triangles.reserve(m_links.size());
    for (auto & pair : m_links) {
        m_triangles.emplace_back(pair.second.segment_ptr());
    }
    return *this;
}

State DriverComplete::operator ()
    (const State & state, const EventHandler & env) const
{
    // before returning, this must be true:
    //
    // are_very_close(/* something */.displacement, make_zero_vector<Vector>())

    auto next_state = [this](const State & state, const EventHandler & env) {
        if (auto * freebody = get_if<InAir>(&state)) {
            return handle_freebody(*freebody, env);
        }
        if (auto * tracker = get_if<OnSegment>(&state)) {
            return handle_tracker(*tracker, env);
        }
        throw MACRO_MAKE_BAD_BRANCH_EXCEPTION();
    };

    auto has_near_zero_displacement = [](const State & state) {
        if (auto * freebody = get_if<InAir>(&state)) {
            return are_very_close(freebody->displacement, Vector{});
        }
        if (auto * tracker = get_if<OnSegment>(&state)) {
            return are_very_close(tracker->displacement, Vector2{});
        }
        throw MACRO_MAKE_BAD_BRANCH_EXCEPTION();
    };

    auto cur_state = state;
    while (!has_near_zero_displacement(cur_state)) {
         cur_state = next_state(cur_state, env);
    }
    return cur_state;
}

/* private */ State DriverComplete::handle_freebody
    (const InAir & freebody, const EventHandler & env) const
{
    const auto * beg = &m_triangles.front();
    const auto * end = beg + m_triangles.size();
    auto new_loc = freebody.location + freebody.displacement;
    for (auto itr = beg; itr != end; ++itr) {
        auto & triangle = **itr;

#       if 0
        auto r = triangle.intersection(freebody.location, new_loc);

        if (!is_solution(r)) continue;
        auto gv = env.displacement_after_triangle_hit(
            triangle, freebody.location, new_loc, triangle.point_at(r));
        if (auto * disv2 = get_if<Vector2>(&gv)) {
            // displacement must be real, and decreasing
            // (decreasing by how much?
            triangle.intersection(freebody.location, new_loc);
            verify_decreasing_displacement<Vector2, Vector>(
                *disv2, freebody.displacement,
                "DriverComplete::handle_freebody");
            bool heads_against_normal = are_very_close(
                  normalize(project_onto(new_loc - freebody.location,
                                         triangle.normal()          ))
                - triangle.normal(), Vector{});
           return OnSegment{*itr, heads_against_normal, r, *disv2};
        } else if (auto * disv3 = get_if<Vector>(&gv)) {
            verify_decreasing_displacement<Vector, Vector>(
                *disv3, freebody.displacement,
                "DriverComplete::handle_freebody");
            // sigmoid false -> true
            auto [before, after] = find_smallest_diff<Real>([&triangle, &freebody](Real x)
                { return !is_solution( triangle.intersection( freebody.location, freebody.location + freebody.displacement*x) ); });
            return InAir{freebody.location + before*freebody.displacement,
                         freebody.displacement*(1 - after)               };
        }
        throw MACRO_MAKE_BAD_BRANCH_EXCEPTION();
#       else
        constexpr const auto k_caller_name = "DriverComplete::handle_freebody";
        auto liminx = triangle.limit_with_intersection(freebody.location, new_loc);
        if (!is_solution(liminx.intersection)) continue;
        auto gv = env.on_triangle_hit(triangle, liminx.limit, liminx.intersection, new_loc);
        if (auto * disv2 = get_if<Vector2>(&gv)) {
            // attach to segment
            verify_decreasing_displacement<Vector2, Vector>(
                *disv2, freebody.displacement, k_caller_name);
            bool heads_against_normal = are_very_close(
                  normalize(project_onto(new_loc - freebody.location,
                                         triangle.normal()          ))
                - triangle.normal(), Vector{});
            return OnSegment{*itr, heads_against_normal, liminx.intersection, *disv2};
        }
        auto * disv3 = get_if<Vector>(&gv);
        assert(disv3);
        verify_decreasing_displacement<Vector, Vector>(
            *disv3, freebody.displacement, k_caller_name);
#       if 0
        // sigmoid false -> true
        auto [before, after] = find_smallest_diff<Real>([&triangle, &freebody](Real x)
            { return !is_solution( triangle.intersection( freebody.location, freebody.location + freebody.displacement*x) ); });
#       endif
        return InAir{liminx.limit, *disv3};
#       endif
    }
    return InAir{freebody.location + freebody.displacement, Vector{}};
}

/* private */ State DriverComplete::handle_tracker
    (const OnSegment & tracker, const EventHandler & env) const
{
    assert(tracker.segment);
    // ground opens up from underneath

    const auto & triangle = *tracker.segment;

    // check collisions with other surfaces while traversing the "tracked" segment
    const TriangleLinks * beg = nullptr;
    const TriangleLinks * end = nullptr;
    // I think I may skip this for now, until I have some results

    // usual segment transfer
    const auto new_loc = tracker.location + tracker.displacement;
    auto crossing = triangle.check_for_side_crossing(tracker.location, new_loc);
    if (crossing.side == TriangleSide::k_inside) {
        if (!tracker.segment->contains_point(tracker.location + tracker.displacement)) {
            triangle.check_for_side_crossing(tracker.location, tracker.location + tracker.displacement);
        }
        assert(tracker.segment->contains_point(tracker.location + tracker.displacement));
        return OnSegment{tracker.segment, tracker.invert_normal,
                         tracker.location + tracker.displacement, Vector2{}};
    }

    const auto transfer = find_links_for(tracker.segment).transfers_to(crossing.side);
    constexpr const auto k_caller_name = "DriverComplete::handle_tracker";
    if (!transfer.target) {
#       if 0
        // without a callback I cannot cancel velocity
        if (env.cling_to_edge(*tracker.segment, gv.side)) {
            OnSegment rv{tracker};
            rv.location = gv.inside;
            rv.displacement = Vector2{};
            return rv;
        }
        return InAir{triangle.point_at( gv.outside ), Vector{}};
#       else
        auto abgv = env.on_transfer_absent_link(*tracker.segment, crossing, new_loc);
        if (auto * disv2 = get_if<Vector2>(&abgv)) {
            OnSegment rv{tracker};
            rv.location     = crossing.inside;
            rv.displacement = *disv2;
            verify_decreasing_displacement<Vector2, Vector2>(
                *disv2, tracker.displacement, k_caller_name);
            return rv;
        } else {
            auto * disv3 = get_if<Vector>(&abgv);
            assert(disv3);
            verify_decreasing_displacement<Vector, Vector2>(
                *disv3, tracker.displacement, k_caller_name);
            return InAir{triangle.point_at(crossing.outside), *disv3};
        }
#       endif
    }

    auto outside_pt = triangle.point_at(crossing.outside);
#   if 0
    auto rem_displc_var = env.pass_triangle_side(
        triangle, transfer.target.get(), outside_pt,
        outside_pt - triangle.point_at(tracker.location));
    if (auto * to_tri = get_if<SegmentTransfer>(&rem_displc_var)) {
        // you mean to transfer
        verify_decreasing_displacement<Vector2, Vector2>(
            to_tri->displacement, tracker.displacement, k_caller_name);
        return OnSegment{transfer.target, bool(transfer.inverts ^ to_tri->invert),
            transfer.target->closest_contained_point(outside_pt), to_tri->displacement};
    } else if (auto * disv3 = get_if<Vector>(&rem_displc_var)) {
        // you mean to "slip off"
        verify_decreasing_displacement<Vector, Vector2>(
            *disv3, tracker.displacement, k_caller_name);
        return InAir{outside_pt, *disv3};
    }
    throw MACRO_MAKE_BAD_BRANCH_EXCEPTION();
#   else
    auto stgv = env.on_transfer(*tracker.segment, crossing,
                                *transfer.target, tracker.segment->point_at(new_loc));
    if (auto * tup = get_if<Tuple<bool, Vector2>>(&stgv)) {
        auto [does_transfer, rem_displc] = *tup; {}
        verify_decreasing_displacement<Vector2, Vector2>(
            rem_displc, tracker.displacement, k_caller_name);
        if (does_transfer) {
            return OnSegment{
                transfer.target, bool(transfer.inverts),
                transfer.target->closest_contained_point(outside_pt), rem_displc};
        }
        OnSegment rv{tracker};
        rv.location = crossing.inside;
        rv.displacement = rem_displc;
        return rv;
    } else {
        auto * disv3 = get_if<Vector>(&stgv);
        assert(disv3);
        verify_decreasing_displacement<Vector, Vector2>(
            *disv3, tracker.displacement, k_caller_name);
        return InAir{outside_pt, *disv3};
    }
#   endif
}

/* private */ const TriangleLinks & DriverComplete::find_links_for
    (SharedPtr<const Triangle> tptr) const
{
    auto itr = m_links.find(std::hash<const Triangle *>{}(tptr.get()));
    if (itr == m_links.end()) {
        throw RtError{"Cannot find triangle links, was the triangle occupied "
                      "by the subject state added to the driver?"};
    }
    return itr->second;
}

// ----------------------------------------------------------------------------

template <typename Vec1, typename Vec2>
void verify_decreasing_displacement
    (EnableIf<cul::VectorTraits<Vec1>::k_is_vector_type, const Vec1 &> displc,
     EnableIf<cul::VectorTraits<Vec2>::k_is_vector_type, const Vec2 &> old_displacement,
     const char * caller)
{
    if (!is_real(displc)) {
        throw InvArg{std::string{caller} + ": new displacement must be a real vector."};
    }
    if (sum_of_squares(displc) > sum_of_squares(old_displacement)) {
        throw InvArg{std::string{caller} + ": new displacement must be decreasing."};
    }
}

} // end of <anonymous> namespace
