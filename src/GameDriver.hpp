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

#include <memory>

struct Camera {
    Vector position, target, up = k_up;
};

enum class KeyControl {
    forward,
    backward,
    left,
    right,
    jump,

    pause,
    advance,
    print_info,
    restart
};

class GameDriver {
public:
    static UniquePtr<GameDriver> make_model_viewer(const char * filename);

    virtual ~GameDriver() {}

    virtual void setup() = 0;

    virtual void update(Real seconds) = 0;
    virtual void press_key(KeyControl) = 0;
    virtual void release_key(KeyControl) = 0;

    virtual void render(ShaderProgram &) const = 0;
    virtual Camera camera() const = 0;
};

// each driver type may have their own builtin systems
// there's another difficult part in this... things like the p and p driver
class DriverN {
public:
    static UniquePtr<DriverN> make_instance();

    using LoaderVec = std::vector<UniquePtr<Loader>>;

    // events may trigger loaders
    virtual void press_key(KeyControl) = 0;

    virtual void release_key(KeyControl) = 0;

    void setup(Platform::ForLoaders & forloaders)
        { handle_loader_tuple(initial_load(forloaders)); }

    void update(Real seconds, Platform::Callbacks & callbacks) {
        update_(seconds);
        for (auto & single : m_singles) {
            (*single)(m_scene);
        }

        UniquePtr<Loader> loader;
        for (auto & trigger : m_triggers) {
            auto uptr = (*trigger)(m_scene);
            if (!uptr) continue;
            if (loader) {
                // emit warning
            } else {
                loader = std::move(uptr);
            }
        }
        if (loader) {
            if (loader->reset_dynamic_systems()) {
                m_singles.clear();
                m_triggers.clear();
            }

            // wipe entities requesting deletion from triggers before loads
            m_scene.update_entities();
            handle_loader_tuple((*loader)(m_player_entities, callbacks));
        }
        m_scene.update_entities();
        callbacks.render_scene(m_scene);
    }

protected:
    using PlayerEntities = Loader::PlayerEntities;
    using EntityVec = Loader::EntityVec;
    using SingleSysVec = Loader::SingleSysVec;
    using TriggerSysVec = Loader::TriggerSysVec;

    virtual Loader::LoaderTuple initial_load(Platform::ForLoaders &) = 0;

    PlayerEntities & player_entities() { return m_player_entities; }

    Scene & scene() { return m_scene; }

    virtual void update_(Real seconds) = 0;

private:
    void handle_loader_tuple(Loader::LoaderTuple && tup) {
        auto player_ents = std::get<PlayerEntities>(tup);
        auto & ents = std::get<EntityVec>(tup);
        auto & singles = std::get<SingleSysVec>(tup);
        auto & triggers = std::get<TriggerSysVec>(tup);
        auto optionally_replace = [this](Entity & e, Entity new_) {
            if (!new_) return;
            if (e) e.request_deletion();
            e = new_;

            m_scene.add_entity(e);
        };
        optionally_replace(m_player_entities.physical, player_ents.physical);
        optionally_replace(m_player_entities.renderable, player_ents.renderable);

        m_scene.add_entities(ents);
        for (auto & single : singles) {
            m_singles.emplace_back(std::move(single));
        }
        for (auto & trigger : triggers) {
            m_triggers.emplace_back(std::move(trigger));
        }
    }

    Scene m_scene;
    PlayerEntities m_player_entities;
    SingleSysVec m_singles;
    TriggerSysVec m_triggers;
};
