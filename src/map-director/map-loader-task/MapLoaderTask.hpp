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
#include "../../TasksController.hpp"

class BackgroundTaskTrap final : public TaskCallbacks {
public:
    using TaskContinuation = BackgroundTask::Continuation;
    using ContinuationStrategy = BackgroundTask::ContinuationStrategy;

    void assign_callbacks(TaskCallbacks & callbacks_)
        { m_callbacks = &callbacks_; }

    void assign_continuation_strategy(ContinuationStrategy & strategy) {
        m_strategy = &strategy;
        m_continuation = &strategy.finish_task();
    }

    void add(const SharedPtr<EveryFrameTask> & task) final
        { m_callbacks->add(task); }

    void add(const SharedPtr<LoaderTask> & task) final
        { m_callbacks->add(task); }

    void add(const SharedPtr<BackgroundTask> & task) final {
        m_continuation = &m_strategy->continue_();
        m_continuation->wait_on(task);
    }

    void add(const Entity & entity) final
        { m_callbacks->add(entity); }

    Platform & platform() final
        { return m_callbacks->platform(); }
#   if 0
    bool has_tasks() const
        { return !m_background_tasks.empty(); }

    RunableBackgroundTasks move_out_tasks()
        { return RunableBackgroundTasks{std::move(m_background_tasks)}; }
#   endif
    TaskContinuation & continuation() const {
        if (m_continuation) return *m_continuation;
        throw RuntimeError{"Strategy was not set"};
    }

    bool delay_required() const {
        return &continuation() == &m_strategy->continue_();
    }

private:
    ContinuationStrategy * m_strategy = nullptr;
    TaskContinuation * m_continuation = nullptr;
    TaskCallbacks * m_callbacks = nullptr;
#   if 0
    std::vector<SharedPtr<BackgroundTask>> m_background_tasks;
#   endif
};

// ----------------------------------------------------------------------------

class MapContentLoaderComplete final : public MapContentLoader {
public:
    using TaskContinuation = BackgroundTask::Continuation;
    using ContinuationStrategy = BackgroundTask::ContinuationStrategy;

    MapContentLoaderComplete() {}

    explicit MapContentLoaderComplete(Platform &);

    bool delay_required() const final
        { return m_background_task_trap.delay_required(); }

    FutureStringPtr promise_file_contents(const char * filename) final;

    void add_warning(MapLoadingWarningEnum) final {}

    SharedPtr<Texture> make_texture() const final;

    void wait_on(const SharedPtr<BackgroundTask> &) final;
#   if 0
    BackgroundTaskCompletion delay_response();
#   endif
    void assign_platform(Platform & platform)
        { m_platform = &platform; }

    void assign_continuation_strategy(ContinuationStrategy & strategy)
        { m_background_task_trap.assign_continuation_strategy(strategy); }

    TaskContinuation & continuation() const
        { return m_background_task_trap.continuation(); }

private:
    Platform * m_platform = nullptr;
    BackgroundTaskTrap m_background_task_trap;
};

// ----------------------------------------------------------------------------

class MapLoaderTask final : public MapLoaderTask_ {
public:
    MapLoaderTask(const char * map_filename, Platform & platform);
#   if 0
    BackgroundTaskCompletion on_delay(Callbacks &) final;
#   endif
    Continuation & in_background
        (Callbacks &, ContinuationStrategy &) final;

    UniquePtr<MapRegion> retrieve() final;

private:
    UniquePtr<MapRegion> m_loaded_region;
    tiled_map_loading::MapLoadStateMachine m_map_loader;
    MapContentLoaderComplete m_content_loader;
};
