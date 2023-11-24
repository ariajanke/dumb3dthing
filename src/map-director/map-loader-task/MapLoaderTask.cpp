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
using Continuation = BackgroundTask::Continuation;

#if 0
class WaitOnBackgroundTasksTask final : public BackgroundDelayTask {
public:
    explicit WaitOnBackgroundTasksTask(RunableBackgroundTasks && tasks):
        m_runable_tasks(std::move(tasks)) {}

    BackgroundTaskCompletion on_delay(Callbacks &) final;

private:
    RunableBackgroundTasks m_runable_tasks;
    BackgroundTaskTrap m_background_task_trap;
};
#endif
} // end of <anonymous> namespace

MapLoaderTask::MapLoaderTask(const char * map_filename, Platform & platform) {
    using namespace tiled_map_loading;

    m_content_loader.assign_platform(platform);
    m_map_loader = MapLoadStateMachine::make_with_starting_state
        (m_content_loader, map_filename);
}
#if 0
BackgroundTaskCompletion MapLoaderTask::on_delay(Callbacks & callbacks) {
    using TaskCompletion = BackgroundTaskCompletion;

    m_content_loader.assign_platform(callbacks.platform());
    auto res = m_map_loader.update_progress(m_content_loader);
    if (res.is_empty()) {
        return m_content_loader.delay_response();
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
#endif
Continuation & MapLoaderTask::in_background
    (Callbacks & callbacks, ContinuationStrategy & strat)
{
    m_content_loader.assign_platform(callbacks.platform());
    m_content_loader.assign_continuation_strategy(strat);
    auto res = m_map_loader.update_progress(m_content_loader);
#   if 0
    if (res.is_empty()) {
        return strat.continue_();
    } else {
#   endif
        (void)res.fold<int>(0).
            map([this] (MapLoadingSuccess && res) {
                m_loaded_region = std::move(res.loaded_region);
                return 0;
            }).
            map_left([] (MapLoadingError &&) {
                return 0;
            }).
            value();
#   if 0
        return strat.finish_task();
    }
#   endif
    return m_content_loader.continuation();
}

UniquePtr<MapRegion> MapLoaderTask::retrieve() {
    if (!m_loaded_region) {
        throw std::runtime_error{"No loaded region to retrieve"};
    }
    return std::move(m_loaded_region);
}

// ----------------------------------------------------------------------------

MapContentLoaderComplete::MapContentLoaderComplete(Platform & platform):
    m_platform(&platform) {}

FutureStringPtr MapContentLoaderComplete::promise_file_contents
    (const char * filename)
    { return m_platform->promise_file_contents(filename); }

SharedPtr<Texture> MapContentLoaderComplete::make_texture() const
    { return m_platform->make_texture(); }

void MapContentLoaderComplete::wait_on
    (const SharedPtr<BackgroundTask> & task)
    { m_background_task_trap.add(task); }
#if 0
BackgroundTaskCompletion MapContentLoaderComplete::delay_response() {
    if (m_background_task_trap.has_tasks()) {
        return BackgroundTaskCompletion
            {std::make_shared<WaitOnBackgroundTasksTask>
                (m_background_task_trap.move_out_tasks())};
    }
    return BackgroundTaskCompletion::k_in_progress;
}
#endif
namespace {
#if 0
BackgroundTaskCompletion WaitOnBackgroundTasksTask::on_delay
    (Callbacks & callbacks)
{
    m_background_task_trap.assign_callbacks(callbacks);
    m_runable_tasks.run_existing_tasks(m_background_task_trap);
    m_runable_tasks = m_runable_tasks.
        combine_tasks_with(m_background_task_trap.move_out_tasks());
    if (m_runable_tasks.is_empty())
        { return BackgroundTaskCompletion::k_finished; }
    return BackgroundTaskCompletion::k_in_progress;

}
#endif
} // end of <anonymous> namespace
