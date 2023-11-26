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

#include "MapRegionTracker.hpp"
#include "TileFactory.hpp"
#include "MapRegionChangesTask.hpp"

namespace {

using TaskContinuation = MapRegionTracker::TaskContinuation;

} // end of <anonymous> namespace
#if 0
void MapRegionTracker::process_load_requests
    (const RegionLoadRequest & request, TaskCallbacks & callbacks)
{
    // can this *NOT* be a task?
    callbacks.add
        (process_decays_into_task
            (process_into_decay_collector
                (request, RegionLoadCollector{m_container})));
}

TaskContinuation & MapRegionTracker::process_load_requests
    (const RegionLoadRequest &, TaskContinuation &)
    {}
#endif

void MapRegionTracker::process_load_requests
    (const RegionLoadRequest & request, TaskCallbacks & callbacks)
{
    auto decay_collector = process_into_decay_collector
        (request, RegionLoadCollector{m_container});
    m_container.decay_regions(decay_collector);
    decay_collector.run_changes
        (callbacks, m_edge_container, m_container);
}

/* private */ RegionDecayCollector
    MapRegionTracker::process_into_decay_collector
    (const RegionLoadRequest & request, RegionLoadCollector && collector)
{
    m_root_region->process_load_request
        (request, RegionPositionFraming{}, collector);
    return collector.finish();
}
#if 0
/* private */ SharedPtr<EveryFrameTask> MapRegionTracker::process_decays_into_task
    (RegionDecayCollector && decay_collector)
{
    m_container.decay_regions(decay_collector);
    return decay_collector.finish_into_task_with(m_edge_container, m_container);
}
#endif
