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

void PlayerUpdateTask::on_every_frame(Callbacks &, Real) {
    if (!m_physics_ent)
        { throw RuntimeError{"Player entity deleted before its update task"}; }
    Entity physics_ent{m_physics_ent};
    check_fall_below(physics_ent);
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
