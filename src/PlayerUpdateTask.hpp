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

#pragma once

#include "Tasks.hpp"
#include "point-and-plane.hpp"

class PlayerTargetingSubTask final {
public:
    using Callbacks = EveryFrameTask::Callbacks;

    static Entity find_nearest_in
        (const PpState &, const std::vector<EntityRef> & entities);

    static Entity create_reticle(PlatformAssetsStrategy &);

    void on_every_frame(const Entity & player, Callbacks & callbacks);

private:
    std::vector<EntityRef> m_target_refs;
    Entity m_reticle;
};

/// all things the player needs to do everyframe
///
/// stuffing it in here, until there's a proper living place for this class
class PlayerUpdateTask final : public EveryFrameTask {
public:
    static constexpr const Real k_camera_rotation_speed = 0.25*2*k_pi;

    static void rotate_camera(Entity & e, Real et);

    static void drag_camera(Entity & e);

    static void set_facing_direction(Entity &);

    explicit PlayerUpdateTask
        (const EntityRef & physics_ent):
        m_physics_ent(physics_ent) {}

    void on_every_frame(Callbacks & callbacks, Real) final;

private:
    static void check_fall_below(Entity & ent);
    // | extremely important that the task is *not* owning
    // v the reason entity refs exists
    EntityRef m_physics_ent;
    PlayerTargetingSubTask m_targeting_subtask;
};
