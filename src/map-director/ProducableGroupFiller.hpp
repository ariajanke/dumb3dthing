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

#pragma once

#include "ProducableGrid.hpp"

#include "../Definitions.hpp"

struct TileLocation final {
    Vector2I on_map;
    Vector2I on_tileset;
};

/// How to fill out a grid with a tile group.
///
///
class ProducableGroupFiller {
public:
    virtual ~ProducableGroupFiller() {}
#   if 0
    class ProducableGroupCreation {
    public:
        virtual ~ProducableGroupCreation() {}

        virtual void reserve(std::size_t number_of_members) = 0;

        virtual void add_member(const TileLocation &) = 0;

        virtual SharedPtr<ProducableGroup_> finish() = 0;
    };

    template <typename T>
    class VectorBasedGroupCreation : public ProducableGroupCreation {
    public:
        static_assert(std::is_base_of_v<ProducableTile, T>);

        virtual T instantiate_member(const TileLocation &) = 0;

        void reserve(std::size_t number_of_members) final
            { m_members.reserve(number_of_members); }

        void add_member(const TileLocation & tile_loc) final
            { m_members.emplace_back(instantiate_member(tile_loc)); }

        SharedPtr<ProducableGroup_> finish() final {
            struct Impl final : public ProducableGroup_ {
                std::vector<T> members;
            };
            auto impl = make_shared<Impl>();
            impl->members = std::move(m_members);
            m_members.clear();
            return impl;
        }

    private:
        std::vector<T> m_members;
    };

    class CallbackWithCreator {
    public:
        virtual ~CallbackWithCreator();

        virtual void operator () (ProducableGroupCreation &) const = 0;
    };

    virtual void make_group(CallbackWithCreator &) const {}
#   endif
    virtual ProducableGroupTileLayer operator ()
        (const std::vector<TileLocation> &, ProducableGroupTileLayer &&) const = 0;
};
