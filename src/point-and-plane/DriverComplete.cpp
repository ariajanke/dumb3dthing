/******************************************************************************

    GPLv3 License
    Copyright (c) 2023 Aria Janke

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

#include "DriverComplete.hpp"

namespace {

#define MACRO_MAKE_BAD_BRANCH_EXCEPTION() BadBranchException(__LINE__, __FILE__)

using namespace cul::exceptions_abbr;
using namespace point_and_plane;
using std::get;
using cul::find_smallest_diff, cul::is_solution, cul::project_onto,
      cul::sum_of_squares, cul::EnableIf;
using LinkTransfer = TriangleLink::Transfer;

template <typename Vec1, typename Vec2>
void verify_decreasing_displacement
    (EnableIf<cul::VectorTraits<Vec1>::k_is_vector_type, const Vec1 &> displc,
     EnableIf<cul::VectorTraits<Vec2>::k_is_vector_type, const Vec2 &> old_displacement,
     const char * caller);

bool new_invert_normal
    (const LinkTransfer & transfer, const OnSegment & tracker)
{
    // if  trans &  tracker then false
    // if !trans &  tracker then true
    // if  trans & !tracker then true
    // if !trans & !tracker then false
    //return transfer.inverts_normal ^ tracker.invert_normal;
    return transfer.inverts_normal ? !tracker.invert_normal : tracker.invert_normal;
}

} // end of <anonymous> namespace

namespace point_and_plane {

// should/add remove fast
void DriverComplete::add_triangle(const SharedPtr<const TriangleLink> & link) {
    m_frametime_link_container.defer_addition_of(link);
}

void DriverComplete::remove_triangle(const SharedPtr<const TriangleLink> & link) {
    m_frametime_link_container.defer_removal_of(link);
}

void DriverComplete::clear_all_triangles() {
    m_frametime_link_container.clear();
}

Driver & DriverComplete::update() {
    m_frametime_link_container.update();
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

    const auto new_loc = freebody.location + freebody.displacement;
    auto view = m_frametime_link_container.view_for(freebody.location, new_loc);
    const auto beg = view.begin();
    const auto end = view.end();

    using LimitIntersection = Triangle::LimitIntersection;
    SharedPtr<const TriangleLink> candidate;
    LimitIntersection candidate_intx;

    constexpr const auto k_caller_name = "DriverComplete::handle_freebody";
    for (auto itr = beg; itr != end; ++itr) {
        auto link_ptr = *itr;
        const auto & triangle = link_ptr->segment();

        auto liminx = triangle.limit_with_intersection(freebody.location, new_loc);
        if (!is_solution(liminx.intersection)) continue;
        if (!candidate) {
            candidate = link_ptr;
            candidate_intx = liminx;
            continue;
        }
        if (magnitude(liminx.limit         - freebody.location) <
            magnitude(candidate_intx.limit - freebody.location)  )
        {
            candidate = link_ptr;
            candidate_intx = liminx;
        }
    }
    if (candidate) {
        const auto & triangle = candidate->segment();
        const auto & intx = candidate_intx.intersection;
        auto gv = env.on_triangle_hit
            (triangle, candidate_intx.limit, intx, new_loc);
        if (auto * disv2 = get_if<Vector2>(&gv)) {
            // need to convert remaining displacement into the same units as
            // freebody's displacement
            auto disv3 =   triangle.point_at(intx + *disv2)
                         - triangle.point_at(intx);
            verify_decreasing_displacement<Vector, Vector>
                (disv3, freebody.displacement, k_caller_name);
            auto displacement_on_normal =
                project_onto(freebody.displacement, triangle.normal());
            bool heads_with_normal =
                dot(triangle.normal(), displacement_on_normal) > 0;
            return OnSegment{candidate, heads_with_normal, intx, *disv2};
        }
        auto * disv3 = get_if<Vector>(&gv);
        assert(disv3);
        verify_decreasing_displacement<Vector, Vector>
            (*disv3, freebody.displacement, k_caller_name);
        return InAir{candidate_intx.limit, *disv3};
    }
    return InAir{freebody.location + freebody.displacement, Vector{}};
}

/* private */ State DriverComplete::handle_tracker
    (const OnSegment & tracker, const EventHandler & env) const
{
    // this function is a little heavy, can we split it? (defer)
    assert(tracker.segment);
    // ground opens up from underneath

    const auto & triangle = *tracker.segment;

    // check collisions with other surfaces while traversing the "tracked" segment
#   if 0 // <- might not be ready to delete yet?
    auto beg = m_links.begin();
    auto end = m_links.end();
#   endif
    // usual segment transfer
    const auto new_loc = tracker.location + tracker.displacement;
    auto crossing = triangle.check_for_side_crossing(tracker.location, new_loc);
    if (crossing.side == TriangleSide::k_inside) {
        auto new_tracker_location = tracker.location + tracker.displacement;
        if (!tracker.segment->contains_point(new_tracker_location)) {
            triangle.check_for_side_crossing(
                tracker.location, new_tracker_location);
        }
        assert(tracker.segment->contains_point(new_tracker_location));
        return OnSegment{tracker.fragment, tracker.invert_normal,
                         new_tracker_location, Vector2{}};
    }

    const auto transfer = std::dynamic_pointer_cast<const TriangleLink>
        (tracker.fragment)->transfers_to(crossing.side);
    constexpr const auto k_caller_name = "DriverComplete::handle_tracker";
    if (!transfer.target) {
        auto abgv = env.on_transfer_absent_link(*tracker.segment, crossing, new_loc);
        if (auto * disv2 = get_if<Vector2>(&abgv)) {
            OnSegment rv{tracker};
            rv.location     = crossing.inside;
            rv.displacement = *disv2;
            verify_decreasing_displacement<Vector2, Vector2>
                (*disv2, tracker.displacement, k_caller_name);
            return rv;
        } else {
            auto * disv3 = get_if<Vector>(&abgv);
            assert(disv3);
            verify_decreasing_displacement<Vector, Vector2>
                (*disv3, tracker.displacement, k_caller_name);
            return InAir{triangle.point_at(crossing.outside), *disv3};
        }
    }

    auto outside_pt = triangle.point_at(crossing.outside);
    auto stgv = env.on_transfer
        (*tracker.segment, crossing, transfer.target->segment(),
         tracker.segment->point_at(new_loc));
    if (auto * res = get_if<EventHandler::TransferOnSegment>(&stgv)) {

        verify_decreasing_displacement<Vector2, Vector2>
            (res->displacement, tracker.displacement, k_caller_name);
        if (res->transfer_to_next) {
            auto seg_loc = transfer.target->segment()
                .closest_contained_point(outside_pt);
#           if 0
            std::cout << (new_invert_normal(transfer, tracker) ? "invert" : "regular") << std::endl;
            OnSegment new_tracker
                {transfer.target, new_invert_normal(transfer, tracker),
                 seg_loc, res->displacement};
            auto norm = transfer.target->segment().normal()*(new_tracker.invert_normal ? -1 : 1);
            std::cout << norm << std::endl;
            std::cout << "on: " << transfer.target->segment() << std::endl;
#           endif
            return OnSegment
                {transfer.target, new_invert_normal(transfer, tracker),
                 seg_loc, res->displacement};
        }
        OnSegment rv{tracker};
        rv.location = crossing.inside;
        rv.displacement = res->displacement;
        return rv;
    } else {
        auto * disv3 = get_if<Vector>(&stgv);
        assert(disv3);
        verify_decreasing_displacement<Vector, Vector2>
            (*disv3, tracker.displacement, k_caller_name);
        return InAir{outside_pt, *disv3};
    }
}

} // end of point_and_plane namespace

namespace {

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
