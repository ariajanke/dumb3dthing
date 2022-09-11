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
#include "platform/platform.hpp"
#include "Components.hpp"

namespace point_and_plane {
class Driver;
}

class Driver {
public:
    using LoaderVec = std::vector<UniquePtr<Loader>>;

    static UniquePtr<Driver> make_instance();

    virtual ~Driver() {}

    // events may trigger loaders
    virtual void press_key(KeyControl) = 0;

    virtual void release_key(KeyControl) = 0;

    void setup(Platform::ForLoaders & forloaders);

    void update(Real seconds, Platform::Callbacks &);

protected:
    using PlayerEntities = LoaderTask::PlayerEntities;
#   if 0
    using SinglesSystemPtr = Loader::SinglesSystemPtr;
#   endif
    using LoaderCallbacks   = LoaderTask::Callbacks;
    using EveryFrameTaskPtr = SharedPtr<EveryFrameTask>;
    using OccasionalTaskPtr = SharedPtr<OccasionalTask>;
    using LoaderTaskPtr     = SharedPtr<LoaderTask>;

    virtual void initial_load(LoaderCallbacks &) = 0;

    PlayerEntities & player_entities() { return m_player_entities; }

    Scene & scene() { return m_scene; }

    virtual void update_(Real seconds) = 0;

    auto get_callbacks(Platform::ForLoaders & platform) {
        class Impl final : public LoaderCallbacks {
        public:
            Impl(Scene & scene_,
                 PlayerEntities & player_entities_,
                 std::vector<EveryFrameTaskPtr> & every_frame_tasks_,
                 std::vector<OccasionalTaskPtr> & occasional_tasks_,
                 std::vector<LoaderTaskPtr> & loader_tasks_,
                 Platform::ForLoaders & platform_):
                m_scene(scene_),
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
                { m_scene.add_entity(ent); }

            Platform::ForLoaders & platform() final { return m_platform; }

            PlayerEntities player_entites() const final
                { return m_player_entities; }

            void set_player_entities(const PlayerEntities & entities) final
                { m_player_entities = entities; }
        private:
            Scene & m_scene;
            PlayerEntities & m_player_entities;
            std::vector<EveryFrameTaskPtr> & m_every_frame_tasks;
            std::vector<OccasionalTaskPtr> & m_occasional_tasks;
            std::vector<LoaderTaskPtr> & m_loader_tasks;
            Platform::ForLoaders & m_platform;
        };
        return Impl{m_scene, m_player_entities, m_every_frame_tasks,
                    m_occasional_tasks, m_loader_tasks, platform};
    }

#   if 0
    auto get_preload_checker() {
        class Impl final : public PreloadSpawner::Adder {
        public:
            explicit Impl(std::vector<UniquePtr<Preloader>> & preloaders):
                m_preloaders(preloaders) {}

            void add_preloader(UniquePtr<Preloader> && uptr) const final {
                if (!uptr) return;
                m_preloaders.emplace_back(std::move(uptr));
            }

        private:
            std::vector<UniquePtr<Preloader>> & m_preloaders;
        };

        return Impl{m_preloaders};
    }
#   endif
    // have to break design a little here for this iteration
    virtual point_and_plane::Driver & ppdriver() = 0;

    void on_entities_changed();

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
#   if 0 // removing dynamic systems for now
    std::vector<SinglesSystemPtr> m_single_systems;

    std::vector<UniquePtr<Preloader>> m_preloaders;
#   endif
    std::vector<EveryFrameTaskPtr> m_every_frame_tasks;
    std::vector<OccasionalTaskPtr> m_occasional_tasks;
    std::vector<LoaderTaskPtr> m_loader_tasks;
};

inline void Driver::setup(Platform::ForLoaders & forloaders) {
#       if 0
    LoaderCallbacks loadercallbacks{ forloaders, player_entities(), m_single_systems, m_scene };

    initial_load(loadercallbacks);
    m_player_entities = loadercallbacks.player_entites();
#       else
    auto callbacks = get_callbacks(forloaders);
    initial_load(callbacks);
    on_entities_changed();
#       endif
}
