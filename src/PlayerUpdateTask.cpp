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

#include "geometric-utilities.hpp"

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

void PlayerUpdateTask::on_every_frame(Callbacks &, Real seconds) {
    if (!m_physics_ent)
        { throw RuntimeError{"Player entity deleted before its update task"}; }
    Entity physics_ent{m_physics_ent};
    if (auto * pc = physics_ent.ptr<PlayerControl>()) {
        pc->frame_update();
    }
    check_fall_below(physics_ent);
    rotate_camera(physics_ent, seconds);
    drag_camera(physics_ent);
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
