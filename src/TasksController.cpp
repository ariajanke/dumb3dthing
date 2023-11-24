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
#if 0
using BackgroundTaskEntry = RunableBackgroundTasks::BackgroundTaskEntry;
#endif
template <typename T>
using TaskView = TasksReceiver::TaskView<T>;

using namespace cul::exceptions_abbr;

template <typename T>
bool is_sole_owner(const SharedPtr<T> & ptr)
    { return ptr.use_count() == 1; }

} // end of <anonymous> namespace

void TasksReceiver::add(const SharedPtr<EveryFrameTask> & ptr)
    { add_to(m_every_frame_tasks, ptr); }

void TasksReceiver::add(const SharedPtr<LoaderTask> & ptr)
    { add_to(m_loader_tasks, ptr); }

void TasksReceiver::add(const SharedPtr<BackgroundTask> & ptr)
    { add_to(m_background_tasks, ptr); }

void TasksReceiver::clear_all() {
    m_every_frame_tasks.clear();
    m_loader_tasks.clear();
    m_background_tasks.clear();
}

bool TasksReceiver::has_any_tasks() const {
    return    !m_every_frame_tasks.empty() || !m_loader_tasks.empty()
           || !m_background_tasks.empty();
}

TaskView<EveryFrameTask> TasksReceiver::every_frame_tasks()
    { return View{m_every_frame_tasks.begin(), m_every_frame_tasks.end()}; }

TaskView<LoaderTask> TasksReceiver::loader_tasks()
    { return View{m_loader_tasks.begin(), m_loader_tasks.end()}; }

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
    throw RtError{  "TriangleLinksReceiver::" + std::string{caller}
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
        throw RtError{"MultiReceiver::platform: no platform was assigned"};
    }
    return *m_platform;
}

RunableTasks MultiReceiver::retrieve_runable_tasks() {
    return RunableTasks
        {move_out_every_frame_tasks(),
         move_out_loader_tasks(),
         move_out_background_tasks()};
}

// ----------------------------------------------------------------------------
#if 0
/* static */ BackgroundTaskEntry
    RunableBackgroundTasks::run_task
    (BackgroundTaskEntry && task_entry, TaskCallbacks & callbacks_)
{
    auto res = (*task_entry.task)(callbacks_);
    if (auto delay_task = res.move_out_delay_task()) {
#       if 0
        delay_task->set_return_task(std::move(task));
#       endif
        task_entry.task_to_return_to = std::move(task_entry.task);
        callbacks_.add(delay_task);
        return BackgroundTaskEntry{};
    } else if (res == BackgroundTaskCompletion::k_finished) {
        return BackgroundTaskEntry{};
    }
    return std::move(task_entry);
}

RunableBackgroundTasks::RunableBackgroundTasks
    (std::vector<SharedPtr<BackgroundTask>> && background_tasks_):
    m_background_tasks(std::move(background_tasks_)) {}

void RunableBackgroundTasks::run_existing_tasks
    (TaskCallbacks & callbacks_)
{
    for (auto & task : m_background_tasks)
        { task = run_task(std::move(task), callbacks_); }

    m_background_tasks.erase
        (std::remove(m_background_tasks.begin(), m_background_tasks.end(), nullptr),
         m_background_tasks.end());
}

RunableBackgroundTasks RunableBackgroundTasks::combine_tasks_with
    (RunableBackgroundTasks && tasks_)
{
    insert_moved_shared_ptrs
        (m_background_tasks, view_of(tasks_.m_background_tasks));
    return RunableBackgroundTasks{std::move(m_background_tasks)};
}
#endif
// ----------------------------------------------------------------------------

/* static */ const RunableBackgroundTasks::BackgroundTaskMap
    RunableBackgroundTasks::k_default_background_task_map =
    RunableBackgroundTasks::BackgroundTaskMap{nullptr};

/* static */ RunableBackgroundTasks::Iterator
    RunableBackgroundTasks::process_task
        (Iterator && itr, NewTaskEntryCollector collector)
{

}

RunableBackgroundTasks::RunableBackgroundTasks
    (std::vector<SharedPtr<BackgroundTask>> && background_tasks_)
{
    m_running_tasks.reserve(background_tasks_.size());
    for (auto & task : background_tasks_) {
        m_running_tasks.emplace(std::move(task), nullptr);
    }
}

RunableBackgroundTasks::RunableBackgroundTasks
    (BackgroundTaskMap && task_map, std::vector<NewTaskEntry> && new_tasks):
    m_running_tasks(std::move(task_map)),
    m_new_tasks(std::move(new_tasks)) {}

void RunableBackgroundTasks::run_existing_tasks(TaskCallbacks & callbacks) {

    TaskStrategy strategy{m_task_continuation};

    for (auto itr = m_running_tasks.begin();
         itr != m_running_tasks.end();)
    {
        auto & task = *(itr->first);
        auto & continuation = task.in_background(callbacks, strategy);
        if (&continuation == &Continuation::task_completion()) {
            const auto & completed_task = itr->first;
            auto task_and_its_return_to_task = m_return_task_collection.
                removed_return_to_task_for(completed_task);
            if (task_and_its_return_to_task.first) {
                NewTaskEntry entry;
                entry.task = std::move(task_and_its_return_to_task.first);
                entry.return_to_task = std::move(task_and_its_return_to_task.second);
                m_new_tasks.emplace_back(std::move(entry));
            }
            itr = m_running_tasks.erase(itr);
        } else if (&continuation == &m_task_continuation) {
            if (m_task_continuation.has_waited_on_tasks()) {
                // yeah, need to refactor these comments away... :/
                // add new tasks to wait on
                const auto & task_to_return_to = itr->first;
                auto tasks_waited_on = m_task_continuation.number_of_tasks_to_wait_on();
                m_new_tasks = m_task_continuation.
                    add_new_entries_to(task_to_return_to, std::move(m_new_tasks));
                // remove current task from running tasks
                auto ex = m_running_tasks.extract(itr);
                // track it as a return to task
                m_return_task_collection.take_from_running_tasks
                    (ex.key, ex.element, tasks_waited_on);
                itr = ex.next;
            } else {
                ++itr;
            }
        } else {
            throw RuntimeError{"Task returned continuation not from strategy"};
        }
    }
    for (auto & entry : m_new_tasks) {
        auto task = std::move(entry.task);
        m_running_tasks.emplace
            (std::move(task), std::move(entry.return_to_task));
    }
    m_new_tasks.clear();
}

RunableBackgroundTasks RunableBackgroundTasks::combine_tasks_with
    (RunableBackgroundTasks && rhs)
{
    for (auto itr = rhs.m_running_tasks.begin();
         !rhs.m_running_tasks.is_empty();)
    {
        auto ex = rhs.m_running_tasks.extract(itr);
        (void)m_running_tasks.
            emplace(std::move(ex.key), std::move(ex.element));
        itr = ex.next;
    }

    using std::move_iterator;
    m_new_tasks.insert
        (m_new_tasks.end(),
         move_iterator{rhs.m_new_tasks.begin()},
         move_iterator{rhs.m_new_tasks.end  ()});

    return RunableBackgroundTasks
        {std::move(m_running_tasks), std::move(m_new_tasks)};
}
#if 0
bool RunableBackgroundTasks_New::is_empty() const
    { return m_background_task_map.is_empty(); }
#endif
// ----------------------------------------------------------------------------
#if 1
RunableTasks::RunableTasks
    (std::vector<SharedPtr<EveryFrameTask>> && every_frame_tasks_,
     std::vector<SharedPtr<LoaderTask>> && loader_tasks_,
     std::vector<SharedPtr<BackgroundTask>> && background_tasks_):
    RunableTasks
        (std::move(every_frame_tasks_),
         std::move(loader_tasks_     ),
         RunableBackgroundTasks{std::move(background_tasks_)}) {}
#endif
RunableTasks::RunableTasks
    (std::vector<SharedPtr<EveryFrameTask>> && every_frame_tasks_,
     std::vector<SharedPtr<LoaderTask>> && loader_tasks_,
     RunableBackgroundTasks && background_tasks_):
    m_every_frame_tasks(std::move(every_frame_tasks_)),
    m_loader_tasks(std::move(loader_tasks_)),
    m_background_tasks(std::move(background_tasks_)) {}

void RunableTasks::run_existing_tasks
    (LoaderTask::Callbacks & callbacks_, Real seconds)
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

    for (auto & task : m_loader_tasks) {
        (*task)(callbacks_);
    }

    m_background_tasks.run_existing_tasks(callbacks_);
    m_loader_tasks.clear();
}

RunableTasks RunableTasks::combine_tasks_with(RunableTasks && rhs) {
    insert_moved_shared_ptrs(m_every_frame_tasks, view_of(rhs.m_every_frame_tasks));
    insert_moved_shared_ptrs(m_loader_tasks     , view_of(rhs.m_loader_tasks     ));

    return RunableTasks
        {std::move(m_every_frame_tasks),
         std::move(m_loader_tasks),
         m_background_tasks.combine_tasks_with(std::move(rhs.m_background_tasks))};
}

// ----------------------------------------------------------------------------

void TasksController::run_tasks(Real elapsed_seconds) {
    m_runable_tasks.run_existing_tasks(m_multireceiver, elapsed_seconds);
    m_runable_tasks = m_multireceiver.
        retrieve_runable_tasks().
        combine_tasks_with(std::move(m_runable_tasks));
}

void TasksController::add_entities_to(Scene & scene)
    { m_multireceiver.add_entities_to(scene); }

void TasksController::add(const SharedPtr<EveryFrameTask> & ptr)
    { m_multireceiver.add(ptr); }

void TasksController::add(const SharedPtr<LoaderTask> & ptr)
    { m_multireceiver.add(ptr); }

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
