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

#include "MapDirector.hpp"
#include "map-loader-task.hpp"
#include "RegionLoadRequest.hpp"

#include "../PlayerUpdateTask.hpp"
#include "../point-and-plane.hpp"
#include "../RenderModel.hpp"
#include "../Texture.hpp"

namespace {

using Continuation = BackgroundTask::Continuation;
using ContinuationStrategy = BackgroundTask::ContinuationStrategy;
using PpDriver = point_and_plane::Driver;

class MapDirectorTask final : public BackgroundTask {
public:
    MapDirectorTask
        (Entity player_physics,
         PpDriver &,
         UniquePtr<MapRegion> && root_region);

    Continuation & in_background
        (Callbacks &, ContinuationStrategy &) final;

private:
    EntityRef m_physics_physics_ref;
    MapDirector m_map_director;
};

// ----------------------------------------------------------------------------

class PlayerMapPreperationTask final : public BackgroundTask {
public:
    PlayerMapPreperationTask
        (SharedPtr<MapLoaderTask_> && map_loader,
         Entity && player_physics,
         PpDriver & ppdriver);

    Continuation & in_background
        (Callbacks & callbacks, ContinuationStrategy & strategy) final;

private:
    bool m_finished_loading_map = false;
    SharedPtr<MapLoaderTask_> m_map_loader;
    Entity m_player_physics;
    PpDriver & m_ppdriver;
};

} // end of <anonymous> namespace

/* static */ SharedPtr<BackgroundTask>
    MapDirector::begin_initial_map_loading
    (Entity player_physics,
     const char * initial_map,
     Platform & platform,
     PpDriver & ppdriver)
{
    return std::make_shared<PlayerMapPreperationTask>
        (MapLoaderTask_::make(initial_map, platform),
         std::move(player_physics),
         ppdriver);
}

void MapDirector::on_every_frame
    (TaskCallbacks & callbacks, const Entity & physic_ent)
{ check_for_other_map_segments(callbacks, physic_ent); }

/* private static */ Vector2I MapDirector::to_region_location
    (const Vector & location, const Size2I & segment_size)
{
    return Vector2I
        {int(std::floor( location.x / segment_size.width )),
         int(std::floor(-location.z / segment_size.height))};
}

/* private */ void MapDirector::check_for_other_map_segments
    (TaskCallbacks & callbacks, const Entity & physics_ent)
{
    m_region_tracker.process_load_requests
        (RegionLoadRequest::find(physics_ent), callbacks);
}

namespace {

MapDirectorTask::MapDirectorTask
    (Entity player_physics,
     PpDriver & ppdriver,
     UniquePtr<MapRegion> && root_region):
    m_physics_physics_ref(player_physics.as_reference()),
    m_map_director(ppdriver, std::move(root_region)) {}

Continuation & MapDirectorTask::in_background
    (Callbacks & taskcallbacks, ContinuationStrategy & strat)
{
    m_map_director.on_every_frame
        (taskcallbacks, Entity{m_physics_physics_ref});
    return strat.continue_();
}

// ----------------------------------------------------------------------------

PlayerMapPreperationTask::PlayerMapPreperationTask
    (SharedPtr<MapLoaderTask_> && map_loader,
     Entity && player_physics,
     PpDriver & ppdriver):
    m_map_loader(std::move(map_loader)),
    m_player_physics(std::move(player_physics)),
    m_ppdriver(ppdriver) {}

Continuation & PlayerMapPreperationTask::in_background
    (Callbacks & callbacks, ContinuationStrategy & strategy)
{
    if (!m_finished_loading_map) {
        m_finished_loading_map = true;
        return strategy.continue_().wait_on(m_map_loader);
    }

    // this seems like the proper place to start handling map objects

    auto player_update_task = make_shared<PlayerUpdateTask>
        (m_player_physics.as_reference());
    auto res = m_map_loader->retrieve();
    auto map_director_task = make_shared<MapDirectorTask>
        (m_player_physics, m_ppdriver, std::move(res.map_region));
    auto * player_object = res.map_objects.seek_by_name("player-spawn-point");
    auto & location = std::get<PpInAir>(m_player_physics.get<PpState>()).location;
    if (player_object) {
        auto x = player_object->get_numeric_attribute<Real>("x");
        auto y = player_object->get_numeric_attribute<Real>("y");
        if (x && y) {
            location = Vector{*x, 0, -*y};
        }
    }
    m_player_physics.
        add<
            Velocity, SharedPtr<EveryFrameTask>, SharedPtr<MapDirectorTask>
            >() = make_tuple
            (Velocity{}, player_update_task, map_director_task);

    callbacks.add(player_update_task);
    callbacks.add(map_director_task);
    return strategy.finish_task();
}

} // end of <anonymous> namespace
