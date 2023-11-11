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

RunableTasks::RunableTasks
    (std::vector<SharedPtr<EveryFrameTask>> && every_frame_tasks_,
     std::vector<SharedPtr<LoaderTask>> && loader_tasks_,
     std::vector<SharedPtr<BackgroundTask>> && background_tasks_):
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

    for (auto & task : m_background_tasks) {
        auto res = (*task)(callbacks_);
        if (auto delay_task = res.move_out_delay_task()) {
            delay_task->set_return_task(std::move(task));
            callbacks_.add(delay_task);
            task = nullptr;
        } else if (res == BackgroundTaskCompletion::k_finished) {
            task = nullptr;
        }
    }
    m_background_tasks.erase
        (std::remove(m_background_tasks.begin(), m_background_tasks.end(), nullptr),
         m_background_tasks.end());
    m_loader_tasks.clear();
}

RunableTasks RunableTasks::combine_tasks_with(RunableTasks && rhs) {
    insert_moved_shared_ptrs(m_every_frame_tasks, view_of(rhs.m_every_frame_tasks));
    insert_moved_shared_ptrs(m_background_tasks , view_of(rhs.m_background_tasks ));
    insert_moved_shared_ptrs(m_loader_tasks     , view_of(rhs.m_loader_tasks     ));

    return RunableTasks
        {std::move(m_every_frame_tasks),
         std::move(m_loader_tasks),
         std::move(m_background_tasks)};
}

template <typename T>
/* private static */ void RunableTasks::insert_moved_shared_ptrs
    (std::vector<SharedPtr<T>> & dest, TaskView<T> source)
{
    using std::make_move_iterator;

    dest.insert(dest.end(), make_move_iterator(source.begin()),
                make_move_iterator(source.end()));
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
