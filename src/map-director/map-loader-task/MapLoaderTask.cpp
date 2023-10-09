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

MapLoaderTask::MapLoaderTask
    (tiled_map_loading::MapLoadStateMachine && map_loader,
     const SharedPtr<MapRegionTracker> & target_region_instance):
     m_region_tracker(verify_region_tracker_presence
         ("MapLoaderTask", target_region_instance)),
     m_map_loader(std::move(map_loader)) {}

BackgroundCompletion MapLoaderTask::operator () (Callbacks &) {
    return m_map_loader.
        update_progress().
        fold<BackgroundCompletion>(BackgroundCompletion::in_progress).
        map([this] (MapLoadingSuccess && res) {
            // want to move this line out
#           if 0
            *m_region_tracker = MapRegionTracker{std::move(res.loaded_region)};
            return BackgroundCompletion::finished;
#           endif

            SharedPtr<MapRegion> parent;
            parent.reset(res.loaded_region.release());
            auto composite_region = make_unique<CompositeMapRegion>
                (Grid<MapSubRegion>{
                    {
                        MapSubRegion{RectangleI{20, 20, 20, 20}, parent},
                        MapSubRegion{RectangleI{ 0, 20, 20, 20}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20,  0, 20, 20}, parent},
                        MapSubRegion{RectangleI{ 0,  0, 20, 20}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 20, 20, 20}, parent},
                        MapSubRegion{RectangleI{ 0, 20, 20, 20}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20,  0, 20, 20}, parent},
                        MapSubRegion{RectangleI{ 0,  0, 20, 20}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 20, 20, 20}, parent},
                        MapSubRegion{RectangleI{ 0, 20, 20, 20}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                    {
                        MapSubRegion{RectangleI{20, 21, 20, 1}, parent},
                        MapSubRegion{RectangleI{ 0,  1, 20, 1}, parent}
                    },
                },
                ScaleComputation{20, 1, 20});
            *m_region_tracker = MapRegionTracker{std::move(composite_region)};
            return BackgroundCompletion::finished;
        }).
        map_left([] (MapLoadingError &&) {
            return BackgroundCompletion::finished;
        }).
        value();
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
