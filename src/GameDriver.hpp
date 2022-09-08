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

    void setup(Platform::ForLoaders & forloaders) {
        LoaderCallbacks loadercallbacks{ forloaders, player_entities(), m_single_systems, m_scene };
#       if 0
        handle_loader_tuple(initial_load(forloaders));
#       endif
        initial_load(loadercallbacks);
        m_player_entities = loadercallbacks.player_entites();
    }

    void update(Real seconds, Platform::Callbacks &);

protected:
    using PlayerEntities = Loader::PlayerEntities;
    using SinglesSystemPtr = Loader::SinglesSystemPtr;
    using LoaderCallbacks = Loader::Callbacks;
#   if 0
    using EntityVec = Loader::EntityVec;
    using SingleSysVec = Loader::SingleSysVec;
#   if 0
    using TriggerSysVec = Loader::TriggerSysVec;
#   endif
#   endif
    virtual void initial_load(LoaderCallbacks &) = 0;

    PlayerEntities & player_entities() { return m_player_entities; }

    Scene & scene() { return m_scene; }

    virtual void update_(Real seconds) = 0;

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

    // have to break design a little here for this iteration
    virtual point_and_plane::Driver & ppdriver() = 0;

private:
    //void handle_loader_tuple(Loader::LoaderTuple &&);

    Scene m_scene;
    PlayerEntities m_player_entities;
    std::vector<SinglesSystemPtr> m_single_systems;
#   if 0
    TriggerSysVec m_triggers;
#   endif
    std::vector<UniquePtr<Preloader>> m_preloaders;
};
