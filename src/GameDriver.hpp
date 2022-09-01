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

class Driver {
public:
    using LoaderVec = std::vector<UniquePtr<Loader>>;

    static UniquePtr<Driver> make_instance();

    virtual ~Driver() {}

    // events may trigger loaders
    virtual void press_key(KeyControl) = 0;

    virtual void release_key(KeyControl) = 0;

    void setup(Platform::ForLoaders & forloaders)
        { handle_loader_tuple(initial_load(forloaders)); }

    void update(Real seconds, Platform::Callbacks &);

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
    void handle_loader_tuple(Loader::LoaderTuple &&);

    Scene m_scene;
    PlayerEntities m_player_entities;
    SingleSysVec m_singles;
    TriggerSysVec m_triggers;
};
