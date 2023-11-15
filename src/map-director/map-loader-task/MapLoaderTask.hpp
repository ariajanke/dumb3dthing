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
    void assign_callbacks(TaskCallbacks & callbacks_)
        { m_callbacks = &callbacks_; }

    void add(const SharedPtr<EveryFrameTask> & task) final
        { m_callbacks->add(task); }

    void add(const SharedPtr<LoaderTask> & task) final
        { m_callbacks->add(task); }

    void add(const SharedPtr<BackgroundTask> & task) final
        { m_background_tasks.push_back(task); }

    void add(const Entity & entity) final
        { m_callbacks->add(entity); }

    Platform & platform() final
        { return m_callbacks->platform(); }

    bool has_tasks() const
        { return !m_background_tasks.empty(); }

    RunableBackgroundTasks move_out_tasks()
        { return RunableBackgroundTasks{std::move(m_background_tasks)}; }

private:
    TaskCallbacks * m_callbacks = nullptr;
    std::vector<SharedPtr<BackgroundTask>> m_background_tasks;
};

// ----------------------------------------------------------------------------

class MapContentLoaderComplete final : public MapContentLoader {
public:
    MapContentLoaderComplete() {}

    explicit MapContentLoaderComplete(Platform &);

    bool delay_required() const final
        { return m_background_task_trap.has_tasks(); }

    FutureStringPtr promise_file_contents(const char * filename) final;

    void add_warning(MapLoadingWarningEnum) final {}

    SharedPtr<Texture> make_texture() const final;

    void wait_on(const SharedPtr<BackgroundTask> &) final;

    BackgroundTaskCompletion delay_response();

    void assign_platform(Platform & platform)
        { m_platform = &platform; }

private:
    Platform * m_platform = nullptr;
    BackgroundTaskTrap m_background_task_trap;
};

// ----------------------------------------------------------------------------

class MapLoaderTask final : public MapLoaderTask_ {
public:
    MapLoaderTask(const char * map_filename, Platform & platform);

    BackgroundTaskCompletion on_delay(Callbacks &) final;

    UniquePtr<MapRegion> retrieve() final;

private:
    UniquePtr<MapRegion> m_loaded_region;
    tiled_map_loading::MapLoadStateMachine m_map_loader;
    MapContentLoaderComplete m_content_loader;
};
