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

#include "Defs.hpp"
#include "platform.hpp"
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

    void add(const SharedPtr<EveryFrameTask> & ptr) final
        { add_to(m_every_frame_tasks, ptr); }

    void add(const SharedPtr<LoaderTask> & ptr) final
        { add_to(m_loader_tasks, ptr); }

    void add(const SharedPtr<BackgroundTask> & ptr) final
        { add_to(m_background_tasks, ptr); }

    void clear_all() {
        m_every_frame_tasks.clear();
        m_loader_tasks.clear();
        m_background_tasks.clear();
    }

    bool has_any_tasks() const {
        return    !m_every_frame_tasks.empty() || !m_loader_tasks.empty()
               || !m_background_tasks.empty();
    }

    TaskView<EveryFrameTask> every_frame_tasks()
        { return View{m_every_frame_tasks.begin(), m_every_frame_tasks.end()}; }

    TaskView<LoaderTask> loader_tasks()
        { return View{m_loader_tasks.begin(), m_loader_tasks.end()}; }

    TaskView<BackgroundTask> background_tasks()
        { return View{m_background_tasks.begin(), m_background_tasks.end()}; }

private:
    template <typename T>
    static void add_to
        (std::vector<SharedPtr<T>> & vec, const SharedPtr<T> & ptr)
    {
        if (ptr)
            vec.push_back(ptr);
    }

    std::vector<SharedPtr<EveryFrameTask>> m_every_frame_tasks;
    std::vector<SharedPtr<LoaderTask>> m_loader_tasks;
    std::vector<SharedPtr<BackgroundTask>> m_background_tasks;
};

class TriangleLinksReceiver : public virtual LoaderTask::Callbacks {
public:
    using LoaderTask::Callbacks::add;

    void add(const SharedPtr<TriangleLink> & ptr) final {
        assert(ptr);
        m_links.push_back(ptr);
    }

    void add_triangle_links_to(point_and_plane::Driver &);

private:
    std::vector<SharedPtr<TriangleLink>> m_links;
};

class EntitiesReceiver : public virtual LoaderTask::Callbacks {
public:
    using LoaderTask::Callbacks::add;

    void add(const Entity & ent) final {
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

    void add_entities_to(Scene & scene) {
        if (m_entities.empty()) return;
        // need to be in source to print out
        scene.add_entities(m_entities);
        scene.update_entities();
        m_entities.clear();
    }

private:
    std::vector<Entity> m_entities;
};

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

    Platform & platform() final {
        using namespace cul::exceptions_abbr;
        if (!m_platform) {
            throw RtError{"MultiReceiver::platform: no platform was assigned"};
        }
        return *m_platform;
    }

    void assign_platform(Platform & platform_)
        { m_platform = &platform_; }

private:
    Platform * m_platform;
};

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
        (std::vector<SharedPtr<T>> & dest, TaskView<T> source)
    {
        using std::make_move_iterator;

        dest.insert(dest.end(), make_move_iterator(source.begin()),
                    make_move_iterator(source.end()));
    }

    template <typename T>
    static TaskView<T> view_of(std::vector<SharedPtr<T>> & vec)
        { return View{vec.begin(), vec.end()}; }

    std::vector<SharedPtr<EveryFrameTask>> m_every_frame_tasks;
    std::vector<SharedPtr<LoaderTask>> m_loader_tasks;
    std::vector<SharedPtr<BackgroundTask>> m_background_tasks;
};

class TasksController final : public LoaderTask::Callbacks {
public:
    void run_tasks(Real elapsed_time) {
        m_old_part.run_existing_tasks(m_multireceiver, elapsed_time);

        m_new_part.replace_tasks_with(m_multireceiver);
        m_new_part.run_existing_tasks(m_multireceiver, elapsed_time);
        m_old_part.take_tasks_from(m_new_part);
    }

    void assign_platform(Platform & platform_)
        { m_multireceiver.assign_platform(platform_); }

    void add_entities_to(Scene & scene)
        { m_multireceiver.add_entities_to(scene); }

    void add_triangle_links_to(point_and_plane::Driver & driver)
        { m_multireceiver.add_triangle_links_to(driver); }

    void add(const SharedPtr<EveryFrameTask> & ptr) final
        { m_multireceiver.add(ptr); }

    void add(const SharedPtr<LoaderTask> & ptr) final
        { m_multireceiver.add(ptr); }

    void add(const SharedPtr<BackgroundTask> & ptr) final
        { m_multireceiver.add(ptr); }

    void add(const Entity & ent) final
        { m_multireceiver.add(ent); }

    void add(const SharedPtr<TriangleLink> & tri) final
        { m_multireceiver.add(tri); }

    Platform & platform()
        { return m_multireceiver.platform(); }

private:
    MultiReceiver m_multireceiver;
    TasksControllerPart m_old_part;
    TasksControllerPart m_new_part;
};

class Driver {
public:
    static UniquePtr<Driver> make_instance();

    virtual ~Driver() {}

    // events may trigger loaders
    virtual void press_key(KeyControl) = 0;

    virtual void release_key(KeyControl) = 0;

    void setup(Platform &);

    void update(Real seconds, Platform &);

protected:
    using PlayerEntities = LoaderTask::PlayerEntities;
    using LoaderCallbacks   = LoaderTask::Callbacks;
#   if 0
    using EveryFrameTaskPtr = SharedPtr<EveryFrameTask>;
    using OccasionalTaskPtr = SharedPtr<OccasionalTask>;
    using LoaderTaskPtr     = SharedPtr<LoaderTask>;
#   endif
    virtual void initial_load(LoaderCallbacks &) = 0;

    PlayerEntities & player_entities() { return m_player_entities; }

    Scene & scene() { return m_scene; }

    virtual void update_(Real seconds) = 0;
#   if 0
    auto get_callbacks(Platform & platform, std::vector<Entity> & entities) {
        class Impl final : public LoaderCallbacks {
        public:
            Impl(std::vector<Entity> & entities_,
                 PlayerEntities & player_entities_,
                 std::vector<EveryFrameTaskPtr> & every_frame_tasks_,
#               if 0
                 std::vector<OccasionalTaskPtr> & occasional_tasks_,
#               endif
                 std::vector<LoaderTaskPtr> & loader_tasks_,
                 std::vector<SharedPtr<BackgroundTask>> & background_tasks_,
                 Platform & platform_):
                m_entities(entities_),
                m_player_entities(player_entities_),
                m_every_frame_tasks(every_frame_tasks_),
#               if 0
                m_occasional_tasks(occasional_tasks_),
#               endif
                m_loader_tasks(loader_tasks_),
                m_background_tasks(background_tasks_),
                m_platform(platform_)
            {}

            void add(const SharedPtr<EveryFrameTask> & ptr) final
                { if (ptr) m_every_frame_tasks.push_back(ptr); }
#           if 0
            void add(const SharedPtr<OccasionalTask> & ptr) final
                { if (ptr) m_occasional_tasks.push_back(ptr); }
#           endif
            void add(const SharedPtr<LoaderTask> & ptr) final
                { if (ptr) m_loader_tasks.push_back(ptr); }

            void add(const SharedPtr<BackgroundTask> & ptr) final
                { if (ptr) m_background_tasks.push_back(ptr); }

            void add(const Entity & ent) final
                { m_entities.push_back(ent); }

            Platform & platform() final { return m_platform; }

            PlayerEntities player_entites() const final
                { return m_player_entities; }

            void set_player_entities(const PlayerEntities & entities) final
                { m_player_entities = entities; }
        private:
            std::vector<Entity> & m_entities;
            PlayerEntities & m_player_entities;
            std::vector<EveryFrameTaskPtr> & m_every_frame_tasks;
#           if 0
            std::vector<OccasionalTaskPtr> & m_occasional_tasks;
#           endif
            std::vector<LoaderTaskPtr> & m_loader_tasks;
            std::vector<SharedPtr<BackgroundTask>> & m_background_tasks;
            Platform & m_platform;
        };
        return Impl{entities, m_player_entities, m_every_frame_tasks,
#                   if 0
                    m_occasional_tasks,
#                   endif
                    m_loader_tasks, m_background_tasks,
                    platform};
    }
#   endif
    // have to break design a little here for this iteration
    virtual point_and_plane::Driver & ppdriver() = 0;

    void on_entities_changed(std::vector<Entity> &);

private:

    // architectural changes:
    // concepts:
    // + occasional tasks
    // + every frame tasks
    // = loaders
    // - preloaders

    // how do I get loaders into...?
    // should they come from everyframes?

    Scene m_scene;
    PlayerEntities m_player_entities;
    TasksController m_tasks_controller;
#   if 0
    std::vector<EveryFrameTaskPtr> m_every_frame_tasks;
#   if 0
    std::vector<OccasionalTaskPtr> m_occasional_tasks;
#   endif
    std::vector<LoaderTaskPtr> m_loader_tasks;
    std::vector<SharedPtr<BackgroundTask>> m_background_tasks;
#   endif
};
