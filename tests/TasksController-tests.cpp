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

#include "../src/TasksController.hpp"

#include "../src/point-and-plane.hpp"

#include "test-helpers.hpp"

namespace {

using namespace cul::tree_ts;

using ContinuationStrategy = BackgroundTask::ContinuationStrategy;
using Continuation = BackgroundTask::Continuation;
using NewTaskEntry = ReturnToTasksCollection::NewTaskEntry;

struct ReturnToTasksCollectionTrackReturnTask final {};
struct ReturnToTasksCollectionAddReturnTaskTo final {};

SharedPtr<BackgroundTask> make_finishing_task() {
    return BackgroundTask::make([]
        (TaskCallbacks &, ContinuationStrategy & strat) -> Continuation &
        { return strat.finish_task(); });
}

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

describe<TasksReceiver>("MultiReceiver for EveryFrameTasks")([] {
    auto task = EveryFrameTask::make([] (TaskCallbacks &, Real) {});
    MultiReceiver mrecv;
    mrecv.add(task);
    mark_it("add adds to view", [&] {
        auto view = mrecv.every_frame_tasks();
        return test_that(   view.begin() != view.end()
                         && *view.begin() == task);
    });
    mark_it("has any tasks after an add", [&] {
        return test_that(mrecv.has_any_tasks());
    });
    mark_it("clear_all, clears all tasks", [&] {
        mrecv.clear_all();
        return test_that(!mrecv.has_any_tasks());
    });
});

describe<TriangleLinksReceiver>("MultiReceiver for triangles")([] {
    using EventHandler = point_and_plane::EventHandler;
    class DriverWithDefaults : public point_and_plane::Driver {
    public:
        void add_triangle(const SharedPtr<const TriangleLink> &) override {}

        void remove_triangle(const SharedPtr<const TriangleLink> &) override {}

        Driver & update() override { return *this; };

        void clear_all_triangles() override {}

        PpState operator () (const PpState & s, const EventHandler &) const override
            { return s; }
    };
    MultiReceiver mrecv;
    mark_it("throws if driver is not set", [&] {
        return expect_exception<RtError>([&] {
            mrecv.add(make_shared<TriangleLink>());
        });
    });
    mark_it("adds a triangle", [&] {
        class Driver final : public DriverWithDefaults {
        public:
            void add_triangle(const SharedPtr<const TriangleLink> &) final
                { m_add_triangles = true; }

            bool has_triangles() const { return m_add_triangles; }

        private:
            bool m_add_triangles = false;
        };

        Driver driver;
        mrecv.assign_point_and_plane_driver(driver);
        mrecv.add(make_shared<TriangleLink>());
        return test_that(driver.has_triangles());
    });
    mark_it("removes a triangle", [&] {
        class Driver final : public DriverWithDefaults {
        public:
            void remove_triangle(const SharedPtr<const TriangleLink> &) final
                { m_removes_triangles = true; }

            bool has_removed_triangles() const { return m_removes_triangles; }

        private:
            bool m_removes_triangles = false;
        };

        Driver driver;
        mrecv.assign_point_and_plane_driver(driver);
        mrecv.remove(make_shared<TriangleLink>());
        return test_that(driver.has_removed_triangles());
    });
});

describe<EntitiesReceiver>("MultiReceiver for entities #add").
    depends_on<TasksReceiver>()([]
{
    MultiReceiver mrecv;
    Entity e = Entity::make_sceneless_entity();
    mark_it("adding an entity automatically adds an everyframe task from "
            "that entity", [&]
    {
        auto & task
            = e.add<SharedPtr<EveryFrameTask>>()
            = EveryFrameTask::make([] (TaskCallbacks &, Real) {});
        mrecv.add(e);
        return test_that(   mrecv.has_any_tasks()
                         && *mrecv.every_frame_tasks().begin() == task);
    });
    mark_it("add does not remove the everyframe task from the entity", [&] {
        e.add<SharedPtr<EveryFrameTask>>();
        mrecv.add(e);
        return test_that(e.has<SharedPtr<EveryFrameTask>>());
    });
    mark_it("adding an entity automatically adds the background task from an "
            "entity", [&]
    {
        auto task
            = e.add<SharedPtr<BackgroundTask>>()
            = BackgroundTask::make
                ([] (TaskCallbacks &, ContinuationStrategy & s) -> Continuation &
                 { return s.finish_task(); });
        mrecv.add(e);
        return test_that(   mrecv.has_any_tasks()
                         && *mrecv.background_tasks().begin() == task);
    });
    mark_it("adding an entity automatically removes background task from that "
            "entity", [&]
    {
        e.add<SharedPtr<BackgroundTask>>()
            = BackgroundTask::make
                ([] (TaskCallbacks &, ContinuationStrategy & s) -> Continuation &
                 { return s.finish_task(); });
        mrecv.add(e);
        return test_that(!e.get<SharedPtr<BackgroundTask>>());
    });

});

describe<ReturnToTasksCollectionTrackReturnTask>
    ("ReturnToTasksCollection#add_return_task_to")([]
{
    ReturnToTasksCollection col;
    std::vector<NewTaskEntry> vec;
    auto task = make_finishing_task();
    auto return_task = make_finishing_task();
    mark_it("does nothing with nullptr", [&] {
        col.add_return_task_to(ElementCollector{vec}, nullptr);
        return test_that(vec.empty());
    }).
    mark_it("throws on untracked task", [&] {
        return expect_exception<InvalidArgument>([&]
            { col.add_return_task_to(ElementCollector{vec}, task); });
    }).
    next([&] {
        col.track_return_task(task, return_task, 2);
        col.add_return_task_to(ElementCollector{vec}, task);
    }).
    mark_it("does not add if counter remains above 0", [&] {
        return test_that(vec.empty());
    }).
    mark_it("adds when counter hits 0", [&] {
        col.add_return_task_to(ElementCollector{vec}, task);
        return test_that(vec.size() == 1 &&
                         vec.front().task == task &&
                         vec.front().return_to_task == return_task);
    });
});

describe<ReturnToTasksCollectionAddReturnTaskTo>
    ("ReturnToTasksCollection#track_return_task").
    depends_on<ReturnToTasksCollectionTrackReturnTask>()([]
{
    ReturnToTasksCollection col;
    mark_it("throws InvalidArugument on tracking nullptr", [&] {
        return expect_exception<InvalidArgument>([&]
            { col.track_return_task(nullptr, nullptr, 1); });
    }).
    mark_it("throws InvalidArugument on 0 or fewer watied on tasks", [&] {
        auto task = make_finishing_task();
        return expect_exception<InvalidArgument>([&]
            { col.track_return_task(task, nullptr, 0); });
    });
});

describe<TaskContinuationComplete>
    ("TaskContinuationComplete#add_new_entries_to").
    depends_on<ReturnToTasksCollectionAddReturnTaskTo>()([]
{
    TaskContinuationComplete continuation;
    ReturnToTasksCollection col;
    std::vector<NewTaskEntry> vec;
    ElementCollector collector{vec};
    auto task = make_finishing_task();
    auto return_to_task = make_finishing_task();
    mark_it("no waited on tasks, is not added to return to collection", [&] {
        continuation.add_waited_on_tasks_to(task, nullptr, collector, col);
        return expect_exception<InvalidArgument>([&] {
            col.add_return_task_to(collector, task);
        });
    }).
    next([&] {
        continuation.wait_on(task);
        continuation.add_waited_on_tasks_to(return_to_task, nullptr, collector, col);
    }).
    mark_it("waited on tasks are added to new tasks", [&] {
        return test_that(vec.size() == 1);
    }).
    mark_it("can return to a task from a collection, if there are waited on "
            "tasks", [&]
    {
        vec.clear();
        col.add_return_task_to(collector, return_to_task);
        return test_that(vec.size() == 1 &&
                         vec.front().task == return_to_task);
    });
});

return 1;

} ();
