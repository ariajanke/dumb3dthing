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

#include "../MapRegionTracker.hpp"

#include "../../Components.hpp"

class MapLoaderTask final : public BackgroundTask {
public:
#   if 0
    MapLoaderTask
        (tiled_map_loading::MapLoadStateMachine && map_loader,
         const SharedPtr<MapRegionTracker> & target_region_instance);
#   endif
    MapLoaderTask
        (const char * map_filename,
         const SharedPtr<MapRegionTracker> & target_region_instance,
         Platform & platform);

    BackgroundTaskCompletion operator () (Callbacks &) final;

private:
    static const SharedPtr<MapRegionTracker> &
        verify_region_tracker_presence
        (const char * caller, const SharedPtr<MapRegionTracker> &);

    SharedPtr<MapRegionTracker> m_region_tracker;
    tiled_map_loading::MapLoadStateMachine m_map_loader;
};

class MapContentLoaderComplete final : public MapContentLoader {
public:
    explicit MapContentLoaderComplete(TaskCallbacks & callbacks):
        m_platform(callbacks.platform()) {}

    explicit MapContentLoaderComplete(Platform & platform):
        m_platform(platform) {}

    bool delay_required() const final { return !m_waited_on_tasks.empty(); }

    FutureStringPtr promise_file_contents(const char * filename) final
        { return m_platform.promise_file_contents(filename); }

    void add_warning(MapLoadingWarningEnum) final {}

    SharedPtr<Texture> make_texture() const final
        { return m_platform.make_texture(); }

    void wait_on(const SharedPtr<BackgroundTask> & task) final {
        m_waited_on_tasks.push_back(task);
    }

    BackgroundTaskCompletion delay_response() {
        auto tasks = std::move(m_waited_on_tasks);
        m_waited_on_tasks.clear();

        class Impl final : public BackgroundDelayTask {
        public:
            explicit Impl
                (std::vector<SharedPtr<BackgroundTask>> && waited_on_tasks):
                m_waited_on_tasks(std::move(waited_on_tasks)) {}

            BackgroundTaskCompletion on_delay(Callbacks & callbacks) {
                for (auto & task : m_waited_on_tasks) {
                    auto res = (*task)(callbacks);
                    if (res == BackgroundTaskCompletion::k_finished) {
                        task = SharedPtr<BackgroundTask>{};
                    }
                }
                auto rem_end = std::remove_if
                    (m_waited_on_tasks.begin(),
                     m_waited_on_tasks.end(),
                     [] (const SharedPtr<BackgroundTask> & ptr)
                     { return static_cast<bool>(ptr); });
                m_waited_on_tasks.erase(rem_end, m_waited_on_tasks.end());
                if (m_waited_on_tasks.empty())
                    return BackgroundTaskCompletion::k_finished;
                return BackgroundTaskCompletion::k_in_progress;
            }

        private:
            std::vector<SharedPtr<BackgroundTask>> m_waited_on_tasks;
        };
        if (tasks.empty()) return BackgroundTaskCompletion::k_in_progress;
        return BackgroundTaskCompletion{ std::make_shared<Impl>(std::move(tasks)) };
    }

private:
    Platform & m_platform;
    std::vector<SharedPtr<BackgroundTask>> m_waited_on_tasks;
};
