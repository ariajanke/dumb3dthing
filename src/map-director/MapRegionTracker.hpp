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

#include "MapRegion.hpp"
#include "RegionLoadRequest.hpp"
#include "RegionEdgeConnectionsContainer.hpp"

class TaskCallbacks;
class RegionLoadRequest;

/// keeps track of already loaded map regions
///
/// regions are treated as one flat collection by this class through a root
/// region
class MapRegionTracker final {
public:
    MapRegionTracker() {}

    explicit MapRegionTracker
        (UniquePtr<MapRegion> && root_region):
        m_root_region(std::move(root_region)) {}

    // should rename
    void frame_hit(const RegionLoadRequest &, TaskCallbacks &);

    bool has_root_region() const noexcept
        { return !!m_root_region; }

private:
    RegionEdgeConnectionsContainer m_edge_container;
    MapRegionContainer m_container;
    UniquePtr<MapRegion> m_root_region;
};
