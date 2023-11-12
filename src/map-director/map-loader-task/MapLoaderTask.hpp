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

#include "TiledMapLoader.hpp"

#include "../map-loader-task.hpp"
#include "../MapRegionTracker.hpp"

#include "../../Components.hpp"

class MapLoaderTask final : public MapLoaderTask_ {
public:
    MapLoaderTask(const char * map_filename, Platform & platform);

    BackgroundTaskCompletion on_delay(Callbacks &) final;

    UniquePtr<MapRegion> retrieve() final;

private:
    UniquePtr<MapRegion> m_loaded_region;
    tiled_map_loading::MapLoadStateMachine m_map_loader;
};

// ----------------------------------------------------------------------------

class MapContentLoaderComplete final : public MapContentLoader {
public:
    explicit MapContentLoaderComplete(TaskCallbacks &);

    explicit MapContentLoaderComplete(Platform &);

    bool delay_required() const final { return !m_waited_on_tasks.empty(); }

    FutureStringPtr promise_file_contents(const char * filename) final;

    void add_warning(MapLoadingWarningEnum) final {}

    SharedPtr<Texture> make_texture() const final;

    void wait_on(const SharedPtr<BackgroundTask> &) final;

    BackgroundTaskCompletion delay_response();

private:
    Platform & m_platform;
    std::vector<SharedPtr<BackgroundTask>> m_waited_on_tasks;
};
