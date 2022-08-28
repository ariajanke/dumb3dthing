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
#include "Components.hpp"
#include "PointAndPlaneDriver.hpp"

// ------------------------------- <Messy Space> ------------------------------

static constexpr const Vector k_gravity = -k_up*10;

template <typename Vec>
std::enable_if_t<cul::k_is_vector_type<Vec>, Vec> normalize_if_nonzero(const Vec & r)
    { return are_very_close( cul::make_zero_vector<Vec>(), r ) ? r : normalize(r); }

// ----------------------------------------------------------------------------

class PlayerControlToVelocity final {
public:
    struct PlayerMotionProfile final {
        // Defaults...
        static constexpr const Real k_max_willed_speed = 5;
        static constexpr const Real k_max_acceleration = 10; // u/s^2
        static constexpr const Real k_min_acceleration = 2;
        static constexpr const Real k_unwilled_acceleration = 3;

        PlayerMotionProfile() {}

        PlayerMotionProfile(Real max_willed_speed_, Real max_acceleration_,
                            Real min_acceleration_, Real unwilled_acceleration_):
            max_willed_speed(max_willed_speed_), max_acceleration(max_acceleration_),
            min_acceleration(min_acceleration_), unwilled_acceleration(unwilled_acceleration_)
        {}

        Real max_willed_speed = k_max_willed_speed;
        Real max_acceleration = k_max_acceleration; // u/s^2
        Real min_acceleration = k_min_acceleration;
        Real unwilled_acceleration = k_unwilled_acceleration;

    };

    explicit PlayerControlToVelocity(Real seconds):
        m_seconds(seconds)
    {}

    void operator ()
        (PpState & state, Velocity & velocity, PlayerControl & control,
         Camera & camera) const;

    /** Computes a new velocity from an old velocity and a willed direction.
     *
     *  @param velocity current velocity of the player
     *  @param willed_dir the direction the player wants to move in, assumed to
     *                    have gravity subtracted, it must either be a normal
     *                    vector OR a zero vector
     *  @param seconds number of seconds that have passed this frame
     *  @return new velocity for the player
     */
    static Velocity find_new_velocity_from_willed
        (const PlayerMotionProfile & profile,
         const Velocity & velocity,
         const Vector & willed_dir, Real seconds);

private:
    Real m_seconds;
};

class VelocitiesToDisplacement final {
public:
    explicit VelocitiesToDisplacement(Real seconds):
        m_seconds(seconds) {}

    void operator ()
        (PpState & state, Velocity & velocity, Opt<JumpVelocity> jumpvel) const;

    /** Converts a displacement in 3D to 2D on a triangle segment.
     *
     *  Naturally, this cancels out dislacement orthogonal to the plane of the
     *  segment.
     *
     *  @param on_segment segment to displace accross
     *  @param dis_in_3d original displacement in 3D
     *  @return displacement vector appropriate for the segment
     */
    static Vector2 find_on_segment_displacement
        (const PpOnSegment & on_segment, const Vector & dis_in_3d);

private:
    Real m_seconds;
};

class AccelerateVelocities final {
public:
    explicit AccelerateVelocities(Real seconds):
        m_seconds(seconds) {}

    void operator () (PpState &, Velocity &, Opt<JumpVelocity>) const;

private:
    Real m_seconds;
};

// accel to velocity
// velocities to displacement

class CheckJump final {
public:
    void operator () (PpState &, PlayerControl &, JumpVelocity &, Opt<Velocity>) const;
};

class UpdatePpState final {
public:
    explicit UpdatePpState(point_and_plane::Driver & driver):
        m_driver(driver) {}

    void operator () (PpState & state, Opt<Velocity> vel) const {
#       if 0
        // more likely to flip flop
        (void)vel;
        static auto evnhandler = point_and_plane::EventHandler::make_test_handler();
        state = m_driver(state, *evnhandler);
#       else
        state = m_driver(state, EventHandler{vel, Opt<JumpVelocity>{}});
#       endif
    }

    class EventHandler final : public point_and_plane::EventHandler {
    public:
        EventHandler() {}

        EventHandler(Opt<Velocity> vel, Opt<JumpVelocity> jumpvel):
            m_vel(vel), m_jumpvel(jumpvel) {}

        Variant<Vector2, Vector> displacement_after_triangle_hit
            (const TriangleSegment & triangle, const Vector & /*location*/,
             const Vector & new_, const Vector & intersection) const final;

        Variant<SegmentTransfer, Vector> pass_triangle_side
            (const TriangleSegment &, const TriangleSegment * to,
             const Vector &, const Vector &) const final;

        bool cling_to_edge(const TriangleSegment & triangle, TriangleSide side) const final;

    private:
        Opt<Velocity> m_vel;
        Opt<JumpVelocity> m_jumpvel;
    };

private:
    point_and_plane::Driver & m_driver;
};
