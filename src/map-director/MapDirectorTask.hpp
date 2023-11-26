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

#include "MapDirector.hpp"

#include "../Tasks.hpp"

class MapDirectorTask final : public BackgroundTask {
public:
    using PpDriver = point_and_plane::Driver;

    MapDirectorTask
        (Entity player_physics,
         PpDriver & ppdriver,
         UniquePtr<MapRegion> && root_region):
        m_physics_physics_ref(player_physics.as_reference()),
        m_map_director(ppdriver, std::move(root_region)) {}

    Continuation & in_background
        (Callbacks & taskcallbacks, ContinuationStrategy & strat) final
    {
        m_map_director.on_every_frame
            (taskcallbacks, Entity{m_physics_physics_ref});
        return strat.continue_();
    }

private:
    EntityRef m_physics_physics_ref;
    MapDirector m_map_director;
};
