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
#include "MapElementValuesMap.hpp"

#include "../Definitions.hpp"

struct TileLocation final {
    Vector2I on_map;
    Vector2I on_tileset;
};

/// How to fill out a grid with a group of tiles
class ProducableGroupFiller {
public:
    class ProducableGroupCreation {
    public:
        virtual ~ProducableGroupCreation() {}

        // Note: not an optimization, essential
        virtual void reserve
            (std::size_t number_of_members, const Size2I & grid_size) = 0;

        virtual void set_layer_properties
            (const SharedPtr<const MapElementProperties> &) = 0;

        /// @returns stable, non-null pointer
        virtual ProducableTile & add_member(const TileLocation &) = 0;

        virtual SharedPtr<ProducableGroupOwner> finish() = 0;
    };

    class CallbackWithCreator {
    public:
        template <typename Func>
        static auto make(Func && f_) {
            class Impl final : public CallbackWithCreator {
            public:
                explicit Impl(Func && f_):
                    m_f(std::move(f_)) {}

                void operator () (ProducableGroupCreation & creation) const final
                    { m_f(creation); }

            private:
                Func m_f;
            };

            return Impl{std::move(f_)};
        }

        virtual ~CallbackWithCreator() {}

        virtual void operator () (ProducableGroupCreation &) const = 0;
    };

    virtual ~ProducableGroupFiller() {}

    virtual void make_group(CallbackWithCreator &) const {}
};
