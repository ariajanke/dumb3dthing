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
#include "../../TasksController.hpp"

namespace {

using namespace cul::exceptions_abbr;
using MapLoadResult = tiled_map_loading::BaseState::MapLoadResult;

class WaitOnTilesetsTask final : public BackgroundDelayTask {
public:
    explicit WaitOnTilesetsTask
        (std::vector<SharedPtr<BackgroundTask>> && waited_on_tasks):
        m_waited_on_tasks(std::move(waited_on_tasks)) {}

    BackgroundTaskCompletion on_delay(Callbacks &);

private:
    RunableBackgroundTasks m_waited_on_tasks;
};

} // end of <anonymous> namespace

MapLoaderTask::MapLoaderTask(const char * map_filename, Platform & platform) {
    using namespace tiled_map_loading;
    MapContentLoaderComplete content_loader{platform};
    m_map_loader = MapLoadStateMachine::make_with_starting_state
        (content_loader, map_filename);
}

BackgroundTaskCompletion MapLoaderTask::on_delay(Callbacks & callbacks) {
    using TaskCompletion = BackgroundTaskCompletion;

    MapContentLoaderComplete content_loader{callbacks};
    auto res = m_map_loader.update_progress(content_loader);
    if (res.is_empty()) {
        return content_loader.delay_response();
    } else {
        (void)res.require().fold<int>().
            map([this] (MapLoadingSuccess && res) {
                m_loaded_region = std::move(res.loaded_region);
                return 0;
            }).
            map_left([] (MapLoadingError &&) {
                return 0;
            }).
            value();
        return TaskCompletion::k_finished;
    }
}

UniquePtr<MapRegion> MapLoaderTask::retrieve() {
    if (!m_loaded_region) {
        throw std::runtime_error{"No loaded region to retrieve"};
    }
    return std::move(m_loaded_region);
}

// ----------------------------------------------------------------------------

MapContentLoaderComplete::MapContentLoaderComplete(TaskCallbacks & callbacks):
    m_platform(callbacks.platform()) {}

MapContentLoaderComplete::MapContentLoaderComplete(Platform & platform):
    m_platform(platform) {}

FutureStringPtr MapContentLoaderComplete::promise_file_contents
    (const char * filename)
    { return m_platform.promise_file_contents(filename); }

SharedPtr<Texture> MapContentLoaderComplete::make_texture() const
    { return m_platform.make_texture(); }

void MapContentLoaderComplete::wait_on
    (const SharedPtr<BackgroundTask> & task)
    { m_waited_on_tasks.push_back(task); }

BackgroundTaskCompletion MapContentLoaderComplete::delay_response() {
    auto tasks = std::move(m_waited_on_tasks);
    m_waited_on_tasks.clear();

    if (tasks.empty()) return BackgroundTaskCompletion::k_in_progress;
    return BackgroundTaskCompletion
        {std::make_shared<WaitOnTilesetsTask>(std::move(tasks))};
}

namespace {

BackgroundTaskCompletion WaitOnTilesetsTask::on_delay(Callbacks & callbacks) {
    m_waited_on_tasks.run_existing_tasks(callbacks);
    if (m_waited_on_tasks.is_empty())
        { return BackgroundTaskCompletion::k_finished; }
    return BackgroundTaskCompletion::k_in_progress;
}

} // end of <anonymous> namespace
