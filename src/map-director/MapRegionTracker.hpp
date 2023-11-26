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

#include "RegionLoadRequest.hpp"
#include "RegionEdgeConnectionsContainer.hpp"
#include "MapRegionChangesTask.hpp"

class TaskCallbacks;
class RegionLoadRequest;
class RegionLoadCollector;
class RegionDecayCollector;

/// keeps track of already loaded map regions
///
/// regions are treated as one flat collection by this class through a root
/// region
class MapRegionTracker final {
public:
    using TaskContinuation = BackgroundTask::Continuation;

    MapRegionTracker();

    explicit MapRegionTracker(UniquePtr<MapRegion> && root_region);

    void process_load_requests(const RegionLoadRequest &, TaskCallbacks &);

    bool has_root_region() const noexcept
        { return !!m_root_region; }

private:
    RegionLoadCollector m_load_collector;
    RegionEdgeConnectionsContainer m_edge_container;
    MapRegionContainer m_container;
    UniquePtr<MapRegion> m_root_region;
};
