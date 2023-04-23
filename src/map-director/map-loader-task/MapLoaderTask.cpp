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
#include "../../Defs.hpp"

namespace {

using namespace cul::exceptions_abbr;
using MapLoadResult = MapLoadingContext::MapLoadResult;

} // end of <anonymous> namespace

MapLoaderTask::MapLoaderTask
    (TiledMapLoader && map_loader,
     const SharedPtr<MapRegionTracker> & target_region_instance,
     const Entity & player_physics, const Size2I & region_size_in_tiles):
     m_region_tracker(verify_region_tracker_presence
         ("MapLoaderTask", target_region_instance)),
     m_map_loader(std::move(map_loader)),
     m_player_physics(player_physics),
     m_region_size_in_tiles(region_size_in_tiles) {}

BackgroundCompletion MapLoaderTask::operator () (Callbacks &) {
    return for_value_or<MapLoadResult>
        (m_map_loader.update_progress(),
         BackgroundCompletion::in_progress,
         [this](MapLoadResult && res)
    {
        std::move(res).map([this](MapLoadingSuccess && res) {
            Entity{m_player_physics}.ensure<Velocity>();
            *m_region_tracker = MapRegionTracker
                {make_unique<TiledMapRegion>
                    (std::move(res.producables_view_grid), m_region_size_in_tiles),
                 m_region_size_in_tiles};
            return res;
        }).map_error([this] (MapLoadingError && error) {
            ;
        });
        return BackgroundCompletion::finished;
    });
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
