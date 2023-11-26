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

MapRegionTracker::MapRegionTracker():
    m_load_collector(m_container) {}

MapRegionTracker::MapRegionTracker
    (UniquePtr<MapRegion> && root_region):
    m_load_collector(m_container),
    m_root_region(std::move(root_region)) {}

void MapRegionTracker::process_load_requests
    (const RegionLoadRequest & request, TaskCallbacks & callbacks)
{
    m_root_region->
        process_load_request(request, RegionPositionFraming{}, m_load_collector);
    auto decay_collector = m_load_collector.finish();
    m_container.decay_regions(decay_collector);
    m_load_collector = decay_collector.run_changes
        (callbacks, m_edge_container, m_container);
}
