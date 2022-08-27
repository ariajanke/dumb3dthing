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

#include "Systems.hpp"

#include <common/TestSuite.hpp>

namespace {

using SegmentTransfer = point_and_plane::EventHandler::SegmentTransfer;

} // end of <anonymous> namespace

void PlayerControlToVelocity::operator ()
    (PpState & state, Velocity & velocity, PlayerControl & control,
     Camera & camera) const
{
    Vector player_loc = point_and_plane::location_of(state);
    Vector forward = normalize(cul::project_onto_plane(player_loc - camera.position, k_up));
    Vector left    = normalize(cross(k_up, forward));
    // +y is forward, +x is right
    auto willed_dir = normalize_if_nonzero(control.heading().y*forward - control.heading().x*left);
#   if 0
    velocity = are_very_close(willed_dir, Vector{}) ? Velocity{} : Velocity{willed_dir*2.5};
#   else
    // more likely to flip-flop
    velocity = find_new_velocity_from_willed(velocity, willed_dir, m_seconds);
#   endif
}

/* static */ Velocity PlayerControlToVelocity::find_new_velocity_from_willed
    (const Velocity & velocity,
     const Vector & willed_dir, Real seconds)
{
    constexpr const Real k_max_willed_speed = 5;
    constexpr const Real k_max_acc = 10; // u/s^2
    constexpr const Real k_min_acc = 2;
    constexpr const Real k_unwilled_acc = 3;
    assert(   are_very_close(magnitude(willed_dir), 1)
           || are_very_close(magnitude(willed_dir), 0));
    // unwilled deceleration
    if (are_very_close(magnitude(willed_dir), 0)) {
        // return velocity;
        auto new_speed = magnitude(velocity.value) - k_unwilled_acc*seconds;
        if (new_speed <= 0) return Velocity{};
        return Velocity{new_speed*normalize(velocity.value)};
    }

    auto dir_boost = are_very_close(velocity.value, Vector{}) ? 0 : (angle_between(velocity.value, willed_dir) / k_pi);
    auto low_speed_boost = 1 - std::min(magnitude(velocity.value) / k_max_willed_speed, Real(0));
    auto t = dir_boost*low_speed_boost;
    auto acc = (1 - t)*k_min_acc + t*k_max_acc;

    auto new_vel = velocity.value + willed_dir*seconds*acc;
    if (   magnitude(velocity.value) > k_max_willed_speed
        && magnitude(new_vel) >= magnitude(velocity.value))
    { return velocity; }

    if (magnitude(new_vel) > k_max_willed_speed)
        { return Velocity{normalize(new_vel)*(k_max_willed_speed*0.9995)}; }

    return Velocity{new_vel};
}

// ----------------------------------------------------------------------------

void VelocitiesToDisplacement::operator ()
    (PpState & state, Velocity & velocity, Opt<JumpVelocity> jumpvel) const
{
    auto displacement =   velocity*m_seconds
                        + (jumpvel ? *jumpvel : JumpVelocity{})*m_seconds
                        + (1. / 2)*k_gravity*m_seconds*m_seconds;
    if (auto * in_air = get_if<PpInAir>(&state)) {
        in_air->displacement = displacement;
    } else if (auto * on_segment = get_if<PpOnSurface>(&state)) {
        on_segment->displacement =
            find_on_segment_displacement(*on_segment, displacement);
    }
}

/* static */ Vector2 VelocitiesToDisplacement::find_on_segment_displacement
    (const PpOnSurface & on_segment, const Vector & dis_in_3d)
{
    auto & triangle = *on_segment.segment;
    auto displacement = cul::project_onto_plane(dis_in_3d, triangle.normal());
    auto pt_v3 = triangle.point_at(on_segment.location);
    auto new_pos = triangle.closest_point(pt_v3 + displacement);
    return new_pos - on_segment.location;
}

// ----------------------------------------------------------------------------

void CheckJump::operator ()
    (PpState & state, PlayerControl & control, JumpVelocity & vel) const
{
    constexpr auto k_jump_vel = k_up*10;

    // begins a jump
    auto * on_segment = get_if<PpOnSurface>(&state);
    if (on_segment && control.is_starting_jump()) {
        auto & triangle = *on_segment->segment;
        auto dir = (on_segment->invert_normal ? -1 : 1)*triangle.normal()*0.1;
        vel = k_jump_vel;
        state = PpInAir{triangle.point_at(on_segment->location) + dir, Vector{}};
    }

    // cuts jump velocity
    auto * in_air = get_if<PpInAir>(&state);
    if (   in_air && control.is_ending_jump()
        && !are_very_close(vel.value, Vector{}))
    {
        vel = normalize(vel.value)*std::sqrt(magnitude(vel.value));
    }
}

// ----------------------------------------------------------------------------

Variant<Vector2, Vector> UpdatePpState::EventHandler::
    displacement_after_triangle_hit
    (const TriangleSegment & triangle, const Vector & /*location*/,
     const Vector & new_, const Vector & intersection) const
{
    // for starters:
    // always attach, entirely consume displacement
    if (m_vel) {
        *m_vel = project_onto_plane(m_vel->value, triangle.normal());
    }
    auto diff = new_ - intersection;
    auto rem_displc = project_onto_plane(diff, triangle.normal());

    auto rv =  triangle.closest_point(rem_displc + intersection)
             - triangle.closest_point(intersection);
    return rv;
}

Variant<SegmentTransfer, Vector> UpdatePpState::EventHandler::
    pass_triangle_side
    (const TriangleSegment &, const TriangleSegment * to,
     const Vector &, const Vector &) const
{
    // always transfer to the other surface
    if (!to) {
        return make_tuple(false, Vector2{});
    }
    // may I know the previous inversion value?
    return make_tuple(false, Vector2{});
}

bool UpdatePpState::EventHandler::
    cling_to_edge
    (const TriangleSegment & triangle, TriangleSide side) const
{
    if (!m_vel) return true;
    auto [sa, sb] = triangle.side_points(side); {}
    *m_vel = project_onto(m_vel->value, sa - sb);
    return true;
}

// ----------------------------------------------------------------------------

void run_system_tests() {
    using namespace cul::ts;
    using Triangle = TriangleSegment;
    TestSuite suite;
    suite.start_series("Systems Tests");
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    mark(suite).test([] {
        using std::get;
        using VtoD = VelocitiesToDisplacement;
        auto a = make_shared<Triangle>(Vector{19.5, 1., -.5}, Vector{19.5, 0, -1.5}, Vector{20.5, 0, -1.5});
        auto b = make_shared<Triangle>(Vector{19.5, 0, -1.5}, Vector{20.5, 0, -2.5}, Vector{20.5, 0, -1.5});
        auto pdriver = [a, b] {
            // need links too
            TriangleLinks links_a{a};
            TriangleLinks links_b{b};
            links_a.attempt_attachment_to(b);
            links_b.attempt_attachment_to(a);

            auto pdriver = point_and_plane::Driver::make_driver();
            pdriver->add_triangle(links_a);
            pdriver->add_triangle(links_b);
            return pdriver;
        } ();

        Vector displacement{-0.076216, -0.00069444, -0.00069444};
        PpState state{PpOnSurface{a, true, Vector2{1.4142019007112767, 0.842617146393735}, Vector2{}}};
        get<PpOnSurface>(state).displacement = VtoD::find_on_segment_displacement(get<PpOnSurface>(state), displacement);
        auto test_handler = point_and_plane::EventHandler::make_test_handler();

        // what should displacement be after the transfer?
        state = (*pdriver)(state, *test_handler);
        get<PpOnSurface>(state).displacement = VtoD::find_on_segment_displacement(get<PpOnSurface>(state), displacement);
        auto displc = segment_displacement_to_v3(state);
        // now I need to reverse it...

        state = (*pdriver)(state, *test_handler);

        are_very_close(displc, Vector{displacement.x, 0, displacement.z});
        return test(get<PpOnSurface>(state).segment != a);
    });
    mark(suite).test([] {
        using VtoD = VelocitiesToDisplacement;
        // maybe I need to test this? -> find_on_segment_displacement
        // no this is fine...
        Vector displacement{-0.7, -0.1, -0.1};
        auto a = make_shared<Triangle>(Vector{19.5, 1., -.5}, Vector{19.5, 0, -1.5}, Vector{20.5, 0, -1.5});
        auto b = make_shared<Triangle>(Vector{19.5, 0, -1.5}, Vector{20.5, 0, -2.5}, Vector{20.5, 0, -1.5});
        PpOnSurface a_state{a, true, a->center_in_2d(), Vector2{}};
        PpOnSurface b_state{b, true, b->center_in_2d(), Vector2{}};
        a_state.displacement = VtoD::find_on_segment_displacement(a_state, displacement);
        b_state.displacement = VtoD::find_on_segment_displacement(b_state, displacement);
        auto a_displc = point_and_plane::segment_displacement_to_v3(PpState{a_state});
        auto b_displc = point_and_plane::segment_displacement_to_v3(PpState{b_state});
        return test(are_very_close(Vector{a_displc.x, 0, a_displc.z}, b_displc));
    });
#   undef mark
}
