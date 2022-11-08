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
#include "SpatialPartitionMap.hpp"

#include <ariajanke/cul/TestSuite.hpp>

#include <unordered_map>
#include <iostream>

namespace {

#define MACRO_MAKE_BAD_BRANCH_EXCEPTION() BadBranchException(__LINE__, __FILE__)

using namespace cul::exceptions_abbr;
using namespace point_and_plane;
using std::get;
using cul::find_smallest_diff, cul::is_solution, cul::project_onto,
      cul::sum_of_squares, cul::EnableIf;

// this can become a bottle neck in performance
// (as can entity component accessors)
// so triangles are then sorted along an arbitrary axis
// I should like to chose a line wherein triangles are most widely and evenly
// distrubuted to reduce load.
class DriverComplete final : public Driver {
public:
    void add_triangle(const SharedPtr<const TriangleLink> &) final;

    void remove_triangle(const SharedPtr<const TriangleLink> &) final;

    void clear_all_triangles() final;

    Driver & update() final;

    State operator () (const State &, const EventHandler &) const final;

private:
    // the job of each method here is to reduce displacement
    State handle_freebody(const InAir &, const EventHandler &) const;

    State handle_tracker(const OnSegment &, const EventHandler &) const;

    std::vector<SharedPtr<const TriangleLink>> m_links;
    bool m_spm_dirty = false;
    ProjectedSpatialMap m_spm;
};

} // end of <anonymous> namespace

namespace point_and_plane {

OnSegment::OnSegment
    (const SharedPtr<const TriangleFragment> & frag_,
     bool invert_norm_, Vector2 loc_, Vector2 dis_):
    fragment(frag_), segment(&frag_->segment()),
    invert_normal(invert_norm_), location(loc_), displacement(dis_)
{
    if (!segment->contains_point(loc_)) {
        std::cerr << loc_ << " " 
                  << segment->point_a_in_2d() << " "
                  << segment->point_b_in_2d() << " "
                  << segment->point_c_in_2d() << std::endl;
    }
    assert(segment->contains_point(loc_));
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

// should/add remove fast
void DriverComplete::add_triangle(const SharedPtr<const TriangleLink> & link) {
    m_links.push_back(link);
    m_spm_dirty = true;
}

void DriverComplete::remove_triangle(const SharedPtr<const TriangleLink> &) {
    throw RtError{"unimplemented"};
}

void DriverComplete::clear_all_triangles() {
    m_links.clear();
}

Driver & DriverComplete::update() {
    auto new_end = std::remove_if(m_links.begin(), m_links.end(),
        [](const SharedPtr<const TriangleLink> & link)
    { return link.use_count() < 2; });

    if (new_end != m_links.end()) {
        auto triangles_dropped = m_links.end() - new_end;
        std::cout << "Dropeed " << triangles_dropped << " triangles." << std::endl;
        m_spm_dirty = true;
    }
    m_links.erase(new_end, m_links.end());

    if (m_spm_dirty) {
        m_spm_dirty = false;
        m_spm.populate(m_links);
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

    auto new_loc = freebody.location + freebody.displacement;
    auto view = m_spm.view_for(freebody.location, new_loc);
    const auto beg = view.begin();//m_links.begin();
    const auto end = view.end();//m_links.end();
    for (auto itr = beg; itr != end; ++itr) {
        auto & triangle = (*itr).lock()->segment();

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
            return OnSegment{(*itr).lock(), heads_against_normal, liminx.intersection, *disv2};
        }
        auto * disv3 = get_if<Vector>(&gv);
        assert(disv3);
        verify_decreasing_displacement<Vector, Vector>(
            *disv3, freebody.displacement, k_caller_name);
        return InAir{liminx.limit, *disv3};
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
    auto beg = m_links.begin();
    auto end = m_links.end();

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
    }

    auto outside_pt = triangle.point_at(crossing.outside);
    auto stgv = env.on_transfer(*tracker.segment, crossing,
                                transfer.target->segment(), tracker.segment->point_at(new_loc));
    if (auto * tup = get_if<Tuple<bool, Vector2>>(&stgv)) {
        auto [does_transfer, rem_displc] = *tup; {}
        verify_decreasing_displacement<Vector2, Vector2>(
            rem_displc, tracker.displacement, k_caller_name);
        if (does_transfer) {
            return OnSegment{
                transfer.target, transfer.inverts,
                transfer.target->segment().closest_contained_point(outside_pt), rem_displc};
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
