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

#include "MapLoaderTask.hpp"
#include "../../Definitions.hpp"

namespace {

using namespace cul::exceptions_abbr;
using MapLoadResult = tiled_map_loading::BaseState::MapLoadResult;

} // end of <anonymous> namespace
#if 0
MapLoaderTask::MapLoaderTask
    (tiled_map_loading::MapLoadStateMachine && map_loader,
     const SharedPtr<MapRegionTracker> & target_region_instance):
     m_region_tracker(verify_region_tracker_presence
         ("MapLoaderTask", target_region_instance)),
     m_map_loader(std::move(map_loader)) {}
#endif

MapLoaderTask::MapLoaderTask
    (const char * map_filename,
     const SharedPtr<MapRegionTracker> & target_region_instance,
     Platform & platform):
    m_region_tracker(verify_region_tracker_presence
        ("MapLoaderTask", target_region_instance))
{
    using namespace tiled_map_loading;
    MapContentLoaderComplete content_loader{platform};
    m_map_loader = MapLoadStateMachine::make_with_starting_state
        (content_loader, map_filename);
}

BackgroundTaskCompletion MapLoaderTask::operator () (Callbacks & callbacks) {
    using TaskCompletion = BackgroundTaskCompletion;
#   if 0
    TaskCompletion::delay_until([] (Callbacks &) {
        ;
    });
#   endif
    MapContentLoaderComplete content_loader{callbacks};
    auto res = m_map_loader.update_progress(content_loader);
    if (res.is_empty()) {
        return content_loader.delay_response();
    } else {
        (void)res.require().fold<int>().
            map([this] (MapLoadingSuccess && res) {
                *m_region_tracker = MapRegionTracker{std::move(res.loaded_region)};
                return 0;
            }).
            map_left([] (MapLoadingError &&) {
                return 0;
            }).
            value();
        return TaskCompletion::k_finished;
    }
#   if 0
    return m_map_loader.
        update_progress(content_loader).
        map([this] (MapLoadingSuccess && res) {
            return std::move(res);
        }).
        fold<TaskCompletion>(TaskCompletion{TaskCompletion::k_in_progress}).
        map([this] (MapLoadingSuccess && res) {
            *m_region_tracker = MapRegionTracker{std::move(res.loaded_region)};
            return TaskCompletion::k_finished;
        }).
        map_left([] (MapLoadingError &&) {
            return TaskCompletion::k_finished;
        }).
        value();
#   endif
}

/* private static */ const SharedPtr<MapRegionTracker> &
    MapLoaderTask::verify_region_tracker_presence
    (const char * caller, const SharedPtr<MapRegionTracker> & ptr)
{
    if (ptr) return ptr;
    throw InvArg{"MapLoaderTask::" + std::string{caller} + ": given map "
                 "region pointer must point to an existing instance, even "
                 "(and favorably) one that has no root region"             };
}
