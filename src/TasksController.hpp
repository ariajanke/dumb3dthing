/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "Components.hpp"

namespace point_and_plane {

class Driver;

} // end of point_and_plane namespace

class TasksController;

class TasksReceiver : virtual public LoaderTask::Callbacks {
public:
    using LoaderTask::Callbacks::add;
    template <typename T>
    using TaskIterator = typename std::vector<SharedPtr<T>>::iterator;
    template <typename T>
    using TaskView = View<TaskIterator<T>>;

    void add(const SharedPtr<EveryFrameTask> & ptr) final;

    void add(const SharedPtr<LoaderTask> & ptr) final;

    void add(const SharedPtr<BackgroundTask> & ptr) final;

    void clear_all();

    bool has_any_tasks() const;

    TaskView<EveryFrameTask> every_frame_tasks();

    TaskView<LoaderTask> loader_tasks();

    TaskView<BackgroundTask> background_tasks();

private:
    template <typename T>
    static void add_to
        (std::vector<SharedPtr<T>> & vec, const SharedPtr<T> & ptr);

    std::vector<SharedPtr<EveryFrameTask>> m_every_frame_tasks;
    std::vector<SharedPtr<LoaderTask>> m_loader_tasks;
    std::vector<SharedPtr<BackgroundTask>> m_background_tasks;
};

// ----------------------------------------------------------------------------

class TriangleLinksReceiver : public virtual LoaderTask::Callbacks {
public:
    using LoaderTask::Callbacks::add;

    void add(const SharedPtr<TriangleLink> & ptr) final;

    void assign_point_and_plane_driver(point_and_plane::Driver &);

    void remove(const SharedPtr<const TriangleLink> &) final;

private:
    void verify_driver_set(const char *) const;

    point_and_plane::Driver * m_ppdriver = nullptr;
};

// ----------------------------------------------------------------------------

class EntitiesReceiver : public virtual LoaderTask::Callbacks {
public:
    using LoaderTask::Callbacks::add;

    void add(const Entity & ent) final;

    void add_entities_to(Scene & scene);

private:
    std::vector<Entity> m_entities;
};

// ----------------------------------------------------------------------------

class MultiReceiver final :
    public EntitiesReceiver,
    public TriangleLinksReceiver,
    public TasksReceiver
{
public:
    using LoaderTask::Callbacks::add;

    MultiReceiver() {}

    explicit MultiReceiver(Platform & platform_):
        m_platform(&platform_) {}

    Platform & platform() final;

    void assign_platform(Platform & platform_)
        { m_platform = &platform_; }

private:
    Platform * m_platform;
};

// ----------------------------------------------------------------------------

// should newly created tasks be run?
class TasksControllerPart final {
public:
    void run_existing_tasks(LoaderTask::Callbacks &, Real elapsed_time);

    void replace_tasks_with(TasksReceiver &);

    void take_tasks_from(TasksControllerPart &);

private:
    template <typename T>
    using TaskIterator = TasksReceiver::TaskIterator<T>;
    template <typename T>
    using TaskView = TasksReceiver::TaskView<T>;

    template <typename T>
    static void insert_moved_shared_ptrs
        (std::vector<SharedPtr<T>> & dest, TaskView<T> source);

    template <typename T>
    static TaskView<T> view_of(std::vector<SharedPtr<T>> & vec)
        { return View{vec.begin(), vec.end()}; }

    std::vector<SharedPtr<EveryFrameTask>> m_every_frame_tasks;
    std::vector<SharedPtr<LoaderTask>> m_loader_tasks;
    std::vector<SharedPtr<BackgroundTask>> m_background_tasks;
};

// ----------------------------------------------------------------------------

class TasksController final : public LoaderTask::Callbacks {
public:
    void add_entities_to(Scene & scene);

    void add(const SharedPtr<EveryFrameTask> & ptr) final;

    void add(const SharedPtr<LoaderTask> & ptr) final;

    void add(const SharedPtr<BackgroundTask> & ptr) final;

    void add(const Entity & ent) final;

    void add(const SharedPtr<TriangleLink> & tri) final;

    void assign_platform(Platform & platform_);

    void assign_point_and_plane_driver(point_and_plane::Driver &);

    Platform & platform();

    void remove(const SharedPtr<const TriangleLink> &) final;

    void run_tasks(Real elapsed_time);

private:
    MultiReceiver m_multireceiver;
    TasksControllerPart m_old_part;
    TasksControllerPart m_new_part;
};
