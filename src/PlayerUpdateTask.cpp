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

#include "PlayerUpdateTask.hpp"
#include "point-and-plane.hpp"
#include "Components.hpp"
#include "targeting-state.hpp"
#include "RenderModel.hpp"
#include "Texture.hpp"

#include "geometric-utilities.hpp"

/* static */ Entity PlayerTargetingSubTask::find_nearest_in
    (const PpState & pp_state, const std::vector<EntityRef> & entities)
{
    if (entities.empty()) {
        return Entity{};
    }
    auto player_location = point_and_plane::location_of(pp_state);
    Entity selection;
    Real nearest_distance = k_inf;
    for (auto & ent_ref : entities) {
        Entity candidate{ent_ref};
        auto ent_location = point_and_plane::location_of(candidate.get<PpState>());
        auto distance = magnitude(player_location - ent_location);
        if (distance < nearest_distance) {
            selection = candidate;
        }
    }
    return selection;
}

/* static */ Entity PlayerTargetingSubTask::create_reticle(Platform & platform) {
    auto ent = Entity::make_sceneless_entity();
    TupleBuilder{}.
        add(RenderModel::make_cone(platform)).
        add(Texture::make_ground(platform)).
        add(ModelTranslation{}).
        add(ModelVisibility{}).
        add(XRotation{k_pi}).
        add_to_entity(ent);
    return ent;
}

void PlayerTargetingSubTask::on_every_frame
    (const Entity & player, Callbacks & callbacks)
{
    const auto & seeker = player.get<TargetSeeker>();
    const auto & retrieval = *player.get<SharedPtr<const TargetsRetrieval>>();
    const auto & pp_state = player.get<PpState>();

    m_target_refs = seeker.
        find_targetables(retrieval, pp_state, std::move(m_target_refs));
    if (!m_reticle) {
        callbacks.add( m_reticle = create_reticle(callbacks.platform()) );
    }
    auto nearest = find_nearest_in(pp_state, m_target_refs);
    m_reticle.get<ModelVisibility>().value = !nearest.is_null();
    if (nearest) {
        m_reticle.get<ModelTranslation>() =
            point_and_plane::location_of(nearest.get<PpState>()) +
            k_up*2;
    }
}

// ----------------------------------------------------------------------------

/* static */ void PlayerUpdateTask::rotate_camera(Entity & e, Real seconds) {
    const auto * pcontrol = e.ptr<PlayerControl>();
    auto * camera = e.ptr<Camera>();
    auto * drag_camera = e.ptr<DragCamera>();
    if (!camera || !pcontrol || !drag_camera)
        { return; }
    auto dir = pcontrol->camera_rotation_direction();
    if (dir == 0)
        { return; }
    auto diff = camera->position - camera->target;
    auto rotated = VectorRotater{camera->up}
        (diff, k_camera_rotation_speed*seconds*dir);
    drag_camera->position = rotated + camera->target;
}

/* static */ void PlayerUpdateTask::drag_camera(Entity & player) {
    auto * ppstate = player.ptr<PpState>();
    if (!ppstate)
        { return; }

    auto pos = location_of(player.get<PpState>()) + Vector{0, 3, 0};
    auto & cam = player.get<DragCamera>();
    if (magnitude(cam.position - pos) > cam.max_distance) {
        cam.position +=
            normalize(pos - cam.position)*
            (magnitude(cam.position - pos) - cam.max_distance);
        assert(are_very_close( magnitude( cam.position - pos ), cam.max_distance ));
    }

    player.get<Camera>().target = location_of(player.get<PpState>());
    player.get<Camera>().position = cam.position;
}

/* static */ void PlayerUpdateTask::set_facing_direction(Entity & player) {
    auto & seeker = player.get<TargetSeeker>();
    auto pos = location_of(player.get<PpState>());
    auto & cam = player.get<DragCamera>();
    seeker.set_facing_direction(normalize( project_onto_plane( pos - cam.position, k_up ) ));
}

void PlayerUpdateTask::on_every_frame(Callbacks & callbacks, Real seconds) {
    if (!m_physics_ent)
        { throw RuntimeError{"Player entity deleted before its update task"}; }
    Entity physics_ent{m_physics_ent};
    if (auto * pc = physics_ent.ptr<PlayerControl>()) {
        pc->frame_update();
    }
    check_fall_below(physics_ent);
    rotate_camera(physics_ent, seconds);
    drag_camera(physics_ent);
    set_facing_direction(physics_ent);
    m_targeting_subtask.on_every_frame(physics_ent, callbacks);
}

/* private static */ void PlayerUpdateTask::check_fall_below(Entity & ent) {
    auto * ppair = get_if<PpInAir>(&ent.get<PpState>());
    if (!ppair) return;
    const auto & recovery_point = ent.get<PlayerRecovery>();
    auto & loc = ppair->location;
    if (loc.y < -10) {
        loc = recovery_point.value;
        ent.get<Velocity>() = Velocity{};
    }
}
