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

void MapRegionTracker::frame_hit
    (const RegionLoadRequest & request, TaskCallbacks & callbacks)
{
    if (!m_root_region) return;
    // the two different containers are a data clump
    // let's see how it turns out, and extract a class from that...

    // need to include adder
    RegionLoadCollector collector{m_container};
    m_root_region->process_load_request(request, Vector2I{}, collector);

    // need to include remover
    auto decay_collector = collector.finish();
    m_container.decay_regions(decay_collector);
    callbacks.add(decay_collector.finish(m_edge_container, m_container));
}
