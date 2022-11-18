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

namespace {

using TransferOnSegment = point_and_plane::EventHandler::TransferOnSegment;

} // end of <anonymous> namespace

void PlayerControlToVelocity::operator ()
    (PpState & state, Velocity & velocity, PlayerControl & control,
     Camera & camera) const
{
    Vector player_loc = point_and_plane::location_of(state);
    Vector diff       = project_onto_plane(player_loc - camera.position, k_up);
    if (are_very_close(diff, Vector{})) return;
    Vector forward    = normalize(diff);
    Vector left       = normalize(cross(k_up, forward));
    // +y is forward, +x is right
    auto willed_dir = normalize_if_nonzero(control.heading().y*forward - control.heading().x*left);
    // more likely to flip-flop
    velocity = find_new_velocity_from_willed(
        PlayerMotionProfile{}, velocity, willed_dir, m_seconds);
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
        if (jumpvel) { *jumpvel = Vector{}; }
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

Variant<Vector2, Vector>
    UpdatePpState::EventHandler::on_triangle_hit
    (const Triangle & triangle, const Vector &, const Vector2 & inside,
     const Vector & next) const
{
    // for starters:
    // always attach, entirely consume displacement
    // triangle, candidate_intx.limit, candidate_intx.intersection, new_loc
    // v I hate this, this is an unexpected side effect!
    if (m_vel) {
        *Opt<Velocity>{m_vel} = project_onto_plane(m_vel->value, triangle.normal());
    }
    if (m_jumpvel) {
        *Opt<JumpVelocity>{m_jumpvel} = project_onto_plane(m_jumpvel->value, triangle.normal());
    }
    return triangle.closest_point(next) - inside;
}

Variant<Vector, Vector2>
    UpdatePpState::EventHandler::on_transfer_absent_link
    (const Triangle & triangle, const SideCrossing & crossing,
     const Vector2 & projected_new_location) const
{
    if (!m_vel) return Vector2{};
    auto [sa, sb] = triangle.side_points(crossing.side); {}
    *Opt<Velocity>{m_vel} = project_onto(m_vel->value, sa - sb);

    // clip remaining displacement
    // but needs to not end up calling this again for the same triangle
    // (within the framce)
    auto [t, u] = triangle.side_points_in_2d(crossing.side);
    return project_onto(projected_new_location - crossing.outside, t - u);
}

Variant<Vector, TransferOnSegment>
    UpdatePpState::EventHandler::on_transfer
    (const Triangle & original, const SideCrossing & crossing,
     const Triangle & next, const Vector & new_loc) const
{
    auto outside = original.point_at(crossing.outside);
    auto rv = next.closest_point(new_loc) - next.closest_point(outside);

    return make_tuple(rv*0.9, true);
}
