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
}

class Driver {
public:
    static UniquePtr<Driver> make_instance();

    virtual ~Driver() {}

    // events may trigger loaders
    virtual void press_key(KeyControl) = 0;

    virtual void release_key(KeyControl) = 0;

    void setup(Platform & forloaders);

    void update(Real seconds, Platform &);

protected:
    using PlayerEntities = LoaderTask::PlayerEntities;
    using LoaderCallbacks   = LoaderTask::Callbacks;
    using EveryFrameTaskPtr = SharedPtr<EveryFrameTask>;
    using OccasionalTaskPtr = SharedPtr<OccasionalTask>;
    using LoaderTaskPtr     = SharedPtr<LoaderTask>;

    virtual void initial_load(LoaderCallbacks &) = 0;

    PlayerEntities & player_entities() { return m_player_entities; }

    Scene & scene() { return m_scene; }

    virtual void update_(Real seconds) = 0;

    auto get_callbacks(Platform & platform, std::vector<Entity> & entities) {
        class Impl final : public LoaderCallbacks {
        public:
            Impl(std::vector<Entity> & entities_,
                 PlayerEntities & player_entities_,
                 std::vector<EveryFrameTaskPtr> & every_frame_tasks_,
                 std::vector<OccasionalTaskPtr> & occasional_tasks_,
                 std::vector<LoaderTaskPtr> & loader_tasks_,
                 Platform & platform_):
                m_entities(entities_),
                m_player_entities(player_entities_),
                m_every_frame_tasks(every_frame_tasks_),
                m_occasional_tasks(occasional_tasks_),
                m_loader_tasks(loader_tasks_),
                m_platform(platform_)
            {}

            void add(const SharedPtr<EveryFrameTask> & ptr) final
                { m_every_frame_tasks.push_back(ptr); }

            void add(const SharedPtr<OccasionalTask> & ptr) final
                { m_occasional_tasks.push_back(ptr); }

            void add(const SharedPtr<LoaderTask> & ptr) final
                { m_loader_tasks.push_back(ptr); }

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
            std::vector<OccasionalTaskPtr> & m_occasional_tasks;
            std::vector<LoaderTaskPtr> & m_loader_tasks;
            Platform & m_platform;
        };
        return Impl{entities, m_player_entities, m_every_frame_tasks,
                    m_occasional_tasks, m_loader_tasks, platform};
    }

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
    std::vector<EveryFrameTaskPtr> m_every_frame_tasks;
    std::vector<OccasionalTaskPtr> m_occasional_tasks;
    std::vector<LoaderTaskPtr> m_loader_tasks;
};

inline void Driver::setup(Platform & forloaders) {
    std::vector<Entity> entities;
    auto callbacks = get_callbacks(forloaders, entities);
    initial_load(callbacks);
    on_entities_changed(entities);
    m_scene.add_entities(entities);
}
