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

/// all things the player needs to do everyframe
///
/// stuffing it in here, until there's a proper living place for this class
class PlayerUpdateTask final : public EveryFrameTask {
public:
    explicit PlayerUpdateTask
        (const EntityRef & physics_ent):
        m_physics_ent(physics_ent) {}

    void on_every_frame(Callbacks & callbacks, Real) final;

private:
    static void check_fall_below(Entity & ent);
    // | extremely important that the task is *not* owning
    // v the reason entity refs exists
    EntityRef m_physics_ent;
};
