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

#include "TasksController.hpp"
#include "point-and-plane.hpp"

namespace {

template <typename T>
using TaskView = TasksReceiver::TaskView<T>;

using Continuation = BackgroundTask::Continuation;

template <typename T>
bool is_sole_owner(const SharedPtr<T> & ptr)
    { return ptr.use_count() == 1; }

} // end of <anonymous> namespace

void TasksReceiver::add(const SharedPtr<EveryFrameTask> & ptr)
    { add_to(m_every_frame_tasks, ptr); }
#if 0
void TasksReceiver::add(const SharedPtr<LoaderTask> & ptr)
    { add_to(m_loader_tasks, ptr); }
#endif
void TasksReceiver::add(const SharedPtr<BackgroundTask> & ptr)
    { add_to(m_background_tasks, ptr); }

void TasksReceiver::clear_all() {
    m_every_frame_tasks.clear();
#   if 0
    m_loader_tasks.clear();
#   endif
    m_background_tasks.clear();
}

bool TasksReceiver::has_any_tasks() const {
    return    !m_every_frame_tasks.empty()
#           if 0
           || !m_loader_tasks.empty()
#           endif
           || !m_background_tasks.empty();
}

TaskView<EveryFrameTask> TasksReceiver::every_frame_tasks()
    { return View{m_every_frame_tasks.begin(), m_every_frame_tasks.end()}; }
#if 0
TaskView<LoaderTask> TasksReceiver::loader_tasks()
    { return View{m_loader_tasks.begin(), m_loader_tasks.end()}; }
#endif
TaskView<BackgroundTask> TasksReceiver::background_tasks()
    { return View{m_background_tasks.begin(), m_background_tasks.end()}; }


template <typename T>
/* private static */ void TasksReceiver::add_to
    (std::vector<SharedPtr<T>> & vec, const SharedPtr<T> & ptr)
{
    if (ptr)
        vec.push_back(ptr);
}

// ----------------------------------------------------------------------------

void TriangleLinksReceiver::add(const SharedPtr<TriangleLink> & ptr) {
    assert(ptr);
    verify_driver_set("add");
    m_ppdriver->add_triangle(ptr);
}

void TriangleLinksReceiver::remove(const SharedPtr<const TriangleLink> & ptr) {
    assert(ptr);
    verify_driver_set("remove");
    m_ppdriver->remove_triangle(ptr);
}

void TriangleLinksReceiver::assign_point_and_plane_driver
    (point_and_plane::Driver & driver)
    { m_ppdriver = &driver; }

/* private */ void TriangleLinksReceiver::verify_driver_set
    (const char * caller) const
{
    if (m_ppdriver) return;
    throw RuntimeError{"TriangleLinksReceiver::" + std::string{caller}
                       + ": point and plane driver needs to be set"};
}

// ----------------------------------------------------------------------------

void EntitiesReceiver::add(const Entity & ent) {
    assert(ent);
    m_entities.push_back(ent);
    if (auto * everyframe = ent.ptr<SharedPtr<EveryFrameTask>>()) {
        add(*everyframe);
    }
    if (auto * background = Entity{ent}.ptr<SharedPtr<BackgroundTask>>()) {
        add(*background);
        *background = nullptr;
    }
}

void EntitiesReceiver::add_entities_to(Scene & scene) {
    if (m_entities.empty()) return;
    // need to be in source to print out
    scene.add_entities(m_entities);
    scene.update_entities();
    m_entities.clear();
}

// ----------------------------------------------------------------------------

Platform & MultiReceiver::platform() {
    if (!m_platform) {
        throw RuntimeError{"MultiReceiver::platform: no platform was assigned"};
    }
    return *m_platform;
}

RunableTasks MultiReceiver::retrieve_runable_tasks(RunableTasks && runable_tasks) {
    return std::move(runable_tasks).
        combine_with(move_out_every_frame_tasks(),
#                   if 0
                     move_out_loader_tasks(),
#                   endif
                     move_out_background_tasks());
}

// ----------------------------------------------------------------------------

TaskContinuationComplete::TaskContinuationComplete
    (BackgroundTaskCollection && reused_collection):
    m_waited_on_tasks(std::move(reused_collection))
    { m_waited_on_tasks.clear(); }

Continuation & TaskContinuationComplete::wait_on
    (const SharedPtr<BackgroundTask> & task)
{
    assert(task);
    m_waited_on_tasks.emplace_back(task);
    return *this;
}

void TaskContinuationComplete::add_waited_on_tasks_to
    (const SharedPtr<BackgroundTask> & current_task,
     const SharedPtr<BackgroundTask> & tasks_return_task,
     ElementCollector<NewTaskEntry> new_tasks_collector,
     ReturnToTasksCollection & tracked_return_tasks)
{
    if (m_waited_on_tasks.empty())
        { return; }
    NewTaskEntry entry;
    entry.return_to_task = current_task;
    tracked_return_tasks.track_return_task
        (current_task, tasks_return_task, m_waited_on_tasks.size());
    for (auto & task : m_waited_on_tasks) {
        entry.task = task;
        new_tasks_collector.push_back(entry);
    }
    m_waited_on_tasks.clear();
}

// ----------------------------------------------------------------------------

void ReturnToTasksCollection::add_return_task_to
    (ElementCollector<NewTaskEntry> new_tasks,
     const SharedPtr<BackgroundTask> & return_task)
{
    if (!return_task)
        { return; }
    auto itr = m_tracked_tasks.find(return_task);
    if (itr == m_tracked_tasks.end()) {
        throw InvalidArgument
            {"Given return to task is not tracked by this collection"};
    }
#   ifdef MACRO_DEBUG
    assert(itr->second.counter > 0);
#   endif
    --itr->second.counter;
    if (itr->second.counter == 0) {
        auto ex = m_tracked_tasks.extract(itr);
        NewTaskEntry entry;
        entry.task = std::move(ex.key);
        entry.return_to_task = std::move(ex.element.return_to_task);
        new_tasks.emplace_back(std::move(entry));
    }
}

void ReturnToTasksCollection::track_return_task
    (const SharedPtr<BackgroundTask> & task_to_return_to,
     const SharedPtr<BackgroundTask> & its_return_to_task,
     int number_of_tasks_to_wait_on)
{
    if (!task_to_return_to) {
        throw InvalidArgument{"Cannot track nullptr"};
    } else if (number_of_tasks_to_wait_on <= 0) {
        throw InvalidArgument
            {"Must wait on at least one task (like the one being passed)"};
    }
    Entry entry;
    entry.counter = number_of_tasks_to_wait_on;
    entry.return_to_task = its_return_to_task;
    m_tracked_tasks.emplace(task_to_return_to, std::move(entry));
}

// ----------------------------------------------------------------------------

RunableBackgroundTasks::RunableBackgroundTasks
    (BackgroundTaskMap && runable_tasks,
     std::vector<NewTaskEntry> && new_tasks,
     TaskContinuationComplete && task_continuation,
     ReturnToTasksCollection && returning_collection):
    m_running_tasks(std::move(runable_tasks)),
    m_new_tasks(std::move(new_tasks)),
    m_task_continuation(std::move(task_continuation)),
    m_return_task_collection(std::move(returning_collection)) {}

void RunableBackgroundTasks::run_existing_tasks(TaskCallbacks & callbacks) {
    TaskStrategy strategy{m_task_continuation};
    for (auto itr = m_running_tasks.begin();
         itr != m_running_tasks.end();)
    {
        auto & task = *(itr->first);
        auto & continuation = task.in_background(callbacks, strategy);
        if (&continuation == &Continuation::task_completion()) {
            m_return_task_collection.add_return_task_to
                (ElementCollector{m_new_tasks}, itr->second.return_task);
        } else if (&continuation == &m_task_continuation) {
            if (!m_task_continuation.has_waited_on_tasks()) {
                ++itr;
                continue;
            }
            m_task_continuation.add_waited_on_tasks_to
                (itr->first,
                 itr->second.return_task,
                 ElementCollector{m_new_tasks},
                 m_return_task_collection);
        } else {
            throw RuntimeError{"Task returned continuation not from strategy"};
        }
        itr = m_running_tasks.erase(itr);
    }
#   if 0
    bool has_new_tasks = !m_new_tasks.empty();
#   endif
    for (auto & entry : m_new_tasks)
        { add_new_task_to(std::move(entry), m_running_tasks); }
    m_new_tasks.clear();
#   if 0
    if (has_new_tasks)
        { return run_existing_tasks(callbacks); }
#   endif
}

RunableBackgroundTasks RunableBackgroundTasks::combine_with
    (std::vector<SharedPtr<BackgroundTask>> && background_tasks) &&
{
    m_running_tasks.reserve(m_running_tasks.size() + background_tasks.size());
    for (auto & task : background_tasks) {
        m_running_tasks.emplace(std::move(task), nullptr);
        task = nullptr;
    }
    return RunableBackgroundTasks
        {std::move(m_running_tasks),
         std::move(m_new_tasks),
         std::move(m_task_continuation),
         std::move(m_return_task_collection)};
}

// ----------------------------------------------------------------------------

RunableTasks::RunableTasks
    (std::vector<SharedPtr<EveryFrameTask>> && every_frame_tasks_,
#   if 0
     std::vector<SharedPtr<LoaderTask>> && loader_tasks_,
#   endif
     RunableBackgroundTasks && background_tasks_):
    m_every_frame_tasks(std::move(every_frame_tasks_)),
#   if 0
    m_loader_tasks(std::move(loader_tasks_)),
#   endif
    m_background_tasks(std::move(background_tasks_)) {}

void RunableTasks::run_existing_tasks
    (TaskCallbacks & callbacks_, Real seconds)
{
    {
    auto enditr = m_every_frame_tasks.begin();
    m_every_frame_tasks.erase
        (std::remove_if
            (m_every_frame_tasks.begin(), enditr,
             is_sole_owner<EveryFrameTask>),
         enditr);
    }

    for (auto & task : m_every_frame_tasks) {
        task->on_every_frame(callbacks_, seconds);
    }
#   if 0
    for (auto & task : m_loader_tasks) {
        (*task)(callbacks_);
    }
#   endif
    m_background_tasks.run_existing_tasks(callbacks_);
#   if 0
    m_loader_tasks.clear();
#   endif
}

RunableTasks RunableTasks::combine_with
    (std::vector<SharedPtr<EveryFrameTask>> && every_frame_tasks,
#   if 0
     std::vector<SharedPtr<LoaderTask>> && loader_tasks,
#   endif
     std::vector<SharedPtr<BackgroundTask>> && background_tasks) &&
{
    insert_moved_shared_ptrs(m_every_frame_tasks, view_of(every_frame_tasks));
#   if 0
    insert_moved_shared_ptrs(m_loader_tasks     , view_of(loader_tasks     ));
#   endif
    return RunableTasks
        {std::move(m_every_frame_tasks),
#       if 0
         std::move(m_loader_tasks),
#       endif
         std::move(m_background_tasks).combine_with(std::move(background_tasks))};
}

// ----------------------------------------------------------------------------

void TasksController::run_tasks(Real elapsed_seconds) {
    m_runable_tasks.run_existing_tasks(m_multireceiver, elapsed_seconds);
    m_runable_tasks = m_multireceiver.
        retrieve_runable_tasks(std::move(m_runable_tasks));
}

void TasksController::add_entities_to(Scene & scene)
    { m_multireceiver.add_entities_to(scene); }

void TasksController::add(const SharedPtr<EveryFrameTask> & ptr)
    { m_multireceiver.add(ptr); }
#if 0
void TasksController::add(const SharedPtr<LoaderTask> & ptr)
    { m_multireceiver.add(ptr); }
#endif
void TasksController::add(const SharedPtr<BackgroundTask> & ptr)
    { m_multireceiver.add(ptr); }

void TasksController::add(const Entity & ent)
    { m_multireceiver.add(ent); }

void TasksController::add(const SharedPtr<TriangleLink> & tri)
    { m_multireceiver.add(tri); }

void TasksController::assign_platform(Platform & platform_)
    { m_multireceiver.assign_platform(platform_); }

void TasksController::assign_point_and_plane_driver
    (point_and_plane::Driver & ppdriver)
    { m_multireceiver.assign_point_and_plane_driver(ppdriver); }

Platform & TasksController::platform()
    { return m_multireceiver.platform(); }

void TasksController::remove(const SharedPtr<const TriangleLink> & ptr)
    { m_multireceiver.remove(ptr); }
