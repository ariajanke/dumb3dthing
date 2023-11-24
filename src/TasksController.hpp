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

#include "Tasks.hpp"

#include <ariajanke/cul/HashMap.hpp>

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

protected:
    std::vector<SharedPtr<EveryFrameTask>> move_out_every_frame_tasks()
        { return std::move(m_every_frame_tasks); }

    std::vector<SharedPtr<LoaderTask>> move_out_loader_tasks()
        { return std::move(m_loader_tasks); }

    std::vector<SharedPtr<BackgroundTask>> move_out_background_tasks()
        { return std::move(m_background_tasks); }

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

class RunableTasks;

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

    RunableTasks retrieve_runable_tasks(RunableTasks &&);

private:
    Platform * m_platform;
};

// ----------------------------------------------------------------------------

class RunableTasksBase {
protected:
    RunableTasksBase() {}

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
};

// ----------------------------------------------------------------------------

class RunableBackgroundTasks final {
public:
    struct Entry final {
        // unique per what?
        int return_to_counter = 0;
        SharedPtr<BackgroundTask> return_to_task;
    };
    struct NewTaskEntry final {
        //int return_to_counter = 0;
        SharedPtr<BackgroundTask> return_to_task;
        SharedPtr<BackgroundTask> task;
    };
    template <typename T>
    class ElementCollector final {
    public:
        /* implicit */ ElementCollector(std::vector<T> & collection):
            m_collection(collection) {}

        void push_back(const T & obj) { m_collection.push_back(obj); }

    private:
        std::vector<T> & m_collection;
    };
    using Continuation = BackgroundTask::Continuation;
    class ReturnToTasksCollection;
    class TaskContinuation final : public Continuation {
    public:
        using BackgroundTaskCollection = std::vector<SharedPtr<BackgroundTask>>;

        TaskContinuation() {}

        explicit TaskContinuation(BackgroundTaskCollection && reused_collection):
            m_waited_on_tasks(std::move(reused_collection))
            { m_waited_on_tasks.clear(); }

        Continuation & wait_on(const SharedPtr<BackgroundTask> & task) final {
            assert(task);
            m_waited_on_tasks.emplace_back(task);
            return *this;
        }
#       if 0
        BackgroundTaskCollection move_out_tasks() {
            m_waited_on_tasks.clear();
            return std::move(m_waited_on_tasks);
        }

        TaskContinuation combine_with(TaskContinuation && rhs) {

            m_waited_on_tasks;
        }
#       endif
#       if 0
        std::vector<NewTaskEntry> add_new_entries_to
            (const SharedPtr<BackgroundTask> & return_task,
             std::vector<NewTaskEntry> && new_tasks)
        {
            if (m_waited_on_tasks.empty()) {
                throw RuntimeError
                    {"Cannot add new entries, as there are none (accidently "
                     "called wrong method)"};
            }
            NewTaskEntry entry;
#           if 0
            entry.return_to_counter = int(m_waited_on_tasks.size());
#           endif
            entry.return_to_task = return_task;
            for (auto & task : m_waited_on_tasks) {
                entry.task = std::move(task);
                new_tasks.push_back(entry);
            }
            m_waited_on_tasks.clear();
            return std::move(new_tasks);
        }
#       endif
        void add_new_entries_to
            (const SharedPtr<BackgroundTask> & current_task,
             const SharedPtr<BackgroundTask> & tasks_return_task,
             ElementCollector<NewTaskEntry>,
             ReturnToTasksCollection &);

        bool has_waited_on_tasks() const noexcept
            { return !m_waited_on_tasks.empty(); }

        std::size_t number_of_tasks_to_wait_on() const noexcept
            { return m_waited_on_tasks.size(); }

    private:
        std::vector<SharedPtr<BackgroundTask>> m_waited_on_tasks;
    };
    class TaskStrategy final : public BackgroundTask::ContinuationStrategy {
    public:
        explicit TaskStrategy(TaskContinuation & continuation):
            m_continuation(continuation) {}

        Continuation & continue_() final { return m_continuation; }

    private:
        TaskContinuation & m_continuation;
    };
    using NewTaskEntryCollector = ElementCollector<NewTaskEntry>;
    // task -> its return task
    using BackgroundTaskMap =
        cul::HashMap<SharedPtr<BackgroundTask>, SharedPtr<BackgroundTask>>;
    using Iterator = BackgroundTaskMap::Iterator;
    class ReturnToTasksCollection final {
    public:
        struct Entry final {
            int counter = 0;
            SharedPtr<BackgroundTask> return_to_task;
        };
        using TaskExtraction = BackgroundTaskMap::Extraction;
        using Iterator = BackgroundTaskMap::Iterator;

        ReturnToTasksCollection(): m_tracked_tasks(nullptr) {}

        // replace with proper struct/object
        // task -> its return task
        std::pair<SharedPtr<BackgroundTask>, SharedPtr<BackgroundTask>>
            removed_return_to_task_for
            (const SharedPtr<BackgroundTask> & return_task)
        {
            if (!return_task)
                { return std::make_pair(nullptr, nullptr); }
            auto itr = m_tracked_tasks.find(return_task);
            if (itr == m_tracked_tasks.end()) {
                throw InvalidArgument
                    {"Given return to task is not tracked by this collection"};
            }
            --itr->second.counter;
            if (itr->second.counter == 0) {
                auto ex = m_tracked_tasks.extract(itr);
                return std::make_pair
                    (std::move(ex.key), std::move(ex.element.return_to_task));
            }
            return std::make_pair(nullptr, nullptr);
        }
#       if 0
        NewTaskEntry return_task_as_new_task(const SharedPtr<BackgroundTask> & return_task);
         {
            auto itr = m_backgroud_tasks.find(return_task);
            if (itr == m_backgroud_tasks.end()) {
                throw InvalidArgument{"Given return to task is not tracked by this collection"};
            }
            auto ex = m_backgroud_tasks.extract(itr);
            NewTaskEntry entry;
            entry.return_to_counter = ex.element.return_to_counter;
            entry.return_to_task = std::move(ex.element.return_to_task);
            entry.task = std::move(ex.key);
            return entry;
        }

        Iterator take_task_from(Extraction && ex) {
            m_backgroud_tasks.emplace(std::move(ex.key), std::move(ex.element));
            return ex.next;
        }
#       endif
        void track_return_task
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
    private:
        cul::HashMap<SharedPtr<BackgroundTask>, Entry> m_tracked_tasks;
    };

    static Iterator process_task(Iterator &&, NewTaskEntryCollector);
#   if 0
    static Entry to_entry(NewTaskEntry && entry) {
        Entry reg_entry;
        reg_entry.return_to_task = std::move(entry.return_to_task);
        reg_entry.return_to_counter = std::move(entry.return_to_counter);
        return reg_entry;
    }
#   endif
    RunableBackgroundTasks() {}

    explicit RunableBackgroundTasks
        (std::vector<SharedPtr<BackgroundTask>> && background_tasks_);

    RunableBackgroundTasks
        (BackgroundTaskMap &&, std::vector<NewTaskEntry> &&);

    RunableBackgroundTasks
        (BackgroundTaskMap &&,
         std::vector<NewTaskEntry> &&,
         TaskContinuation &&,
         ReturnToTasksCollection &&);

    void run_existing_tasks(TaskCallbacks & callbacks);

    RunableBackgroundTasks combine_with
        (std::vector<SharedPtr<BackgroundTask>> &&) &&;
#   if 0
    bool is_empty() const;
#   endif
private:
    static const BackgroundTaskMap k_default_background_task_map;

    BackgroundTaskMap m_running_tasks = k_default_background_task_map;
    std::vector<NewTaskEntry> m_new_tasks;
    TaskContinuation m_task_continuation;
    ReturnToTasksCollection m_return_task_collection;
};
#if 0
class RunableBackgroundTasks final : public RunableTasksBase {
public:
    struct BackgroundTaskEntry final {
        BackgroundTaskEntry() {}

        BackgroundTaskEntry
            (SharedPtr<BackgroundTask> && task_,
             SharedPtr<BackgroundTask> && task_to_return_to_):
            task(std::move(task_)),
            task_to_return_to(std::move(task_to_return_to_)) {}

        SharedPtr<BackgroundTask> task;
        SharedPtr<BackgroundTask> task_to_return_to;
    };

    static BackgroundTaskEntry run_task
        (BackgroundTaskEntry &&, TaskCallbacks &);

    RunableBackgroundTasks() {}

    explicit RunableBackgroundTasks
        (std::vector<SharedPtr<BackgroundTask>> && background_tasks_);

    void run_existing_tasks(TaskCallbacks &);

    RunableBackgroundTasks combine_tasks_with(RunableBackgroundTasks &&);

    bool is_empty() const { return m_background_tasks.empty(); }

private:
    std::vector<BackgroundTaskEntry> m_background_tasks;
};
#endif
// ----------------------------------------------------------------------------

class RunableTasks final : public RunableTasksBase {
public:
    RunableTasks() {}
#   if 1
    RunableTasks
        (std::vector<SharedPtr<EveryFrameTask>> && every_frame_tasks_,
         std::vector<SharedPtr<LoaderTask>> && loader_tasks_,
         std::vector<SharedPtr<BackgroundTask>> && background_tasks_);
#   endif
    RunableTasks
        (std::vector<SharedPtr<EveryFrameTask>> && every_frame_tasks_,
         std::vector<SharedPtr<LoaderTask>> && loader_tasks_,
         RunableBackgroundTasks && background_tasks_);

    void run_existing_tasks(LoaderTask::Callbacks &, Real elapsed_seconds);
#   if 0
    RunableTasks combine_tasks_with(RunableTasks &&);
#   endif

    RunableTasks combine_with
        (std::vector<SharedPtr<EveryFrameTask>> &&,
         std::vector<SharedPtr<LoaderTask>> &&,
         std::vector<SharedPtr<BackgroundTask>> &&) &&;
private:
    std::vector<SharedPtr<EveryFrameTask>> m_every_frame_tasks;
    std::vector<SharedPtr<LoaderTask>> m_loader_tasks;
    RunableBackgroundTasks m_background_tasks;
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

    void run_tasks(Real elapsed_seconds);

private:
    MultiReceiver m_multireceiver;
    RunableTasks m_runable_tasks;
};

// ----------------------------------------------------------------------------

template <typename T>
/* protected static */ void RunableTasksBase::insert_moved_shared_ptrs
    (std::vector<SharedPtr<T>> & dest, TaskView<T> source)
{
    using std::make_move_iterator;

    dest.insert(dest.end(), make_move_iterator(source.begin()),
                make_move_iterator(source.end()));
}
