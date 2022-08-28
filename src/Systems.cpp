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
    Vector forward    = normalize(project_onto_plane(player_loc - camera.position, k_up));
    Vector left       = normalize(cross(k_up, forward));
    // +y is forward, +x is right
    auto willed_dir = normalize_if_nonzero(control.heading().y*forward - control.heading().x*left);
#   if 0
    velocity = are_very_close(willed_dir, Vector{}) ? Velocity{} : Velocity{willed_dir*2.5};
#   else
    // more likely to flip-flop
    velocity = find_new_velocity_from_willed(
        PlayerMotionProfile{}, velocity, willed_dir, m_seconds);
#   endif
}

/* static */ Velocity PlayerControlToVelocity::find_new_velocity_from_willed
    (const PlayerMotionProfile & pf, const Velocity & velocity,
     const Vector & willed_dir, Real seconds)
{
    assert(   are_very_close(magnitude(willed_dir), 1)
           || are_very_close(magnitude(willed_dir), 0));

    const auto & old_vel = velocity.value;
    // no willed direction? do deceleration
    if (are_very_close(magnitude(willed_dir), 0)) {
        auto new_speed = magnitude(old_vel)
                - pf.unwilled_acceleration*seconds;
        if (new_speed <= 0) return Velocity{};
        return Velocity{new_speed*normalize(old_vel)};
    }

    Real dir_boost = 0;
    if (!are_very_close(old_vel, Vector{})){
        auto angl = angle_between(old_vel, willed_dir);
        dir_boost = angl / k_pi;
        assert(are_very_close(angl, k_pi*0.5) ? are_very_close(angl / k_pi, 0.5) : true);
    }

    auto low_speed_boost = std::max(
        1 - magnitude(old_vel) / pf.max_willed_speed,
        Real(0));
    auto t = (dir_boost + low_speed_boost) / 2;
    auto acc = (1 - t)*pf.min_acceleration + t*pf.max_acceleration;

    auto new_vel = old_vel + willed_dir*seconds*acc;
    if (   magnitude(old_vel) > pf.max_willed_speed
        && magnitude(new_vel) >= magnitude(old_vel))
    { return velocity; }

    if (magnitude(new_vel) < magnitude(old_vel))
        return Velocity{new_vel};

    if (magnitude(new_vel) > pf.max_willed_speed)
        { return Velocity{normalize(new_vel)*(pf.max_willed_speed*0.9995)}; }

    return Velocity{new_vel};
}

// ----------------------------------------------------------------------------

void VelocitiesToDisplacement::operator ()
    (PpState & state, Velocity & velocity, Opt<JumpVelocity> jumpvel) const
{
    auto displacement =   velocity*m_seconds
                        + (jumpvel ? *jumpvel : JumpVelocity{})*m_seconds
                        ;//+ (1. / 2)*k_gravity*m_seconds*m_seconds;
    if (auto * in_air = get_if<PpInAir>(&state)) {
        in_air->displacement = displacement;
    } else if (auto * on_segment = get_if<PpOnSegment>(&state)) {
        on_segment->displacement =
            find_on_segment_displacement(*on_segment, displacement);
    }
}

/* static */ Vector2 VelocitiesToDisplacement::find_on_segment_displacement
    (const PpOnSegment & on_segment, const Vector & dis_in_3d)
{
    auto & triangle   = *on_segment.segment;
    auto displacement = project_onto_plane(dis_in_3d, triangle.normal());
    auto pt_v3        = triangle.point_at(on_segment.location);
    auto new_pos      = triangle.closest_point(pt_v3 + displacement);
    return new_pos - on_segment.location;
}

// ----------------------------------------------------------------------------

void AccelerateVelocities::operator ()
    (PpState & ppstate, Velocity & velocity, Opt<JumpVelocity> jumpvel) const
{
    auto new_vel = [this](Vector r)
        { return r + k_gravity*m_seconds; };
    if (get_if<PpInAir>(&ppstate)) {
        velocity = new_vel(velocity.value);
        if (jumpvel) *jumpvel = new_vel(jumpvel->value);
    } else if (auto * on_seg = get_if<PpOnSegment>(&ppstate)) {
        auto new_vel_on_seg = [new_vel, on_seg] (Vector r) {
            auto seg_norm = on_seg->segment->normal();
            return project_onto_plane(new_vel(r), seg_norm);
        };
        velocity = new_vel_on_seg(velocity.value);
        if (jumpvel) { *jumpvel = new_vel_on_seg(jumpvel->value); }
    }
}

// ----------------------------------------------------------------------------

void CheckJump::operator ()
    (PpState & state, PlayerControl & control, JumpVelocity & vel,
     Opt<Velocity>) const
{
    constexpr auto k_jump_vel = k_up*10;

    // begins a jump
    auto * on_segment = get_if<PpOnSegment>(&state);
    if (on_segment && control.is_starting_jump()) {
        auto & triangle = *on_segment->segment;
        auto dir = (on_segment->invert_normal ? -1 : 1)*triangle.normal()*0.1;
        vel = k_jump_vel;
#       if 0
        if (regvel) *regvel = regvel->value - project_onto(regvel->value, k_up);
#       endif
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
        *Opt<Velocity>{m_vel} = project_onto_plane(m_vel->value, triangle.normal());
    }
    if (m_jumpvel) {
        *Opt<JumpVelocity>{m_jumpvel} = project_onto_plane(m_jumpvel->value, triangle.normal());
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
    *Opt<Velocity>{m_vel} = project_onto(m_vel->value, sa - sb);
    return true;
}
