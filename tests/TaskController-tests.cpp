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
// forward declarations
#include "../src/PointAndPlaneDriver.hpp"

#include <ariajanke/cul/TreeTestSuite.hpp>

#define mark_it mark_source_position(__LINE__, __FILE__).it

using namespace cul::exceptions_abbr;

template <typename ExpType, typename Func>
cul::tree_ts::TestAssertion expect_exception(Func && f) {
    try {
        f();
    } catch (ExpType &) {
        return cul::tree_ts::test_that(true);
    }
    return cul::tree_ts::test_that(false);
}

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;
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
    mark_it("add auto adds everyframe task from an entity", [&] {
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
    mark_it("add auto adds bac  kground task from an entity", [&] {
        auto task
            = e.add<SharedPtr<BackgroundTask>>()
            = BackgroundTask::make([] (TaskCallbacks &) { return BackgroundCompletion::finished; });
        mrecv.add(e);
        return test_that(   mrecv.has_any_tasks()
                         && *mrecv.background_tasks().begin() == task);
    });
    mark_it("add removes background task from entity", [&] {
        e.add<SharedPtr<BackgroundTask>>()
            = BackgroundTask::make([] (TaskCallbacks &) { return BackgroundCompletion::finished; });
        mrecv.add(e);
        return test_that(!e.get<SharedPtr<BackgroundTask>>());
    });

});
// can't really test add_entities_to...
return 1;

} ();
