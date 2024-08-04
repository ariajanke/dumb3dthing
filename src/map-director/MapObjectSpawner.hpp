/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "ProducableGrid.hpp"
#include "MapElementValuesMap.hpp"
#include "MapObject.hpp"

class MapObjectSpawner final {
public:
    class EntityCreator {
    public:
        template <typename Func>
        static auto make(Func && f) {
            class Impl final : public EntityCreator {
            public:
                Impl(Func && f): m_impl(std::move(f)) {}

                Entity operator () () const { return m_impl(); }

            private:
                Func m_impl;
            };
            return Impl{std::move(f)};
        }

        virtual ~EntityCreator() {}

        virtual Entity operator () () const = 0;
    };

    static void spawn_tree
        (const MapItemPropertiesRetrieval &,
         const EntityCreator &,
         AssetsRetrieval &,
         const MapObjectFraming & = MapObjectFraming{});

    static void spawn_grass
        (const MapItemPropertiesRetrieval &,
         const EntityCreator &,
         AssetsRetrieval &,
         const MapObjectFraming & = MapObjectFraming{});
};
