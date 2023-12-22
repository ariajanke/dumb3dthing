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

#include "MapObjectGroup.hpp"

class MapObjectReferrers final {
public:
    using MapObjectContainer = MapObject::MapObjectRefContainer;
    using MapObjectConstIterator = MapObject::MapObjectRefConstIterator;
    using ObjectViewMap = cul::HashMap<int, View<MapObjectConstIterator>>;

    MapObjectReferrers() {}

    MapObjectReferrers(MapObjectContainer &&, ObjectViewMap &&);

    View<MapObjectConstIterator> get_referrers(int) const;

private:
    MapObjectContainer m_object_refs;
    ObjectViewMap m_view_map{0};
};

// ----------------------------------------------------------------------------

class MapObjectCollection final {
public:
    using GroupContainer = MapObject::GroupContainer;
    using GroupIterator = GroupContainer::const_iterator;
    using NameObjectMap = MapObjectGroup::NameObjectMap;
    using XmlElementContainer = MapObject::XmlElementContainer;
    using MapObjectContainer = MapObject::MapObjectContainer;

    static MapObjectCollection load_from
        (const DocumentOwningNode & map_element);

    MapObjectCollection(): m_top_level_groups_end(m_groups.end()) {}

    void load(const DocumentOwningNode & map_element);

    const MapObject * seek_object_by_id(int id) const
        { return m_id_maps.seek_object_by_id(id); }

    const MapObjectGroup * seek_group_by_id(int id) const
        { return m_id_maps.seek_group_by_id(id); }

    View<GroupIterator> top_level_groups() const
        { return View{m_groups.begin(), m_top_level_groups_end}; }

    const MapObject * seek_by_name(const char * name) const;

private:
    template <typename T>
    using IntHashMap = cul::HashMap<int, T>;

    class IdsToElementsMap final : public MapObjectRetrieval {
    public:
        IdsToElementsMap() {}

        void set_object_id_map(const std::vector<MapObject> &);

        void set_group_id_map(const GroupContainer &);

        void set_referrers(MapObjectReferrers && referrers)
            { m_referrers = std::move(referrers); }

        const MapObjectGroup * seek_group_by_id(int id) const final;

        const MapObject * seek_object_by_id(int id) const final;

        View<MapObjectRefConstIterator>
            seek_referrers_by_id(int id) const final
            { return m_referrers.get_referrers(id); }

    private:
        template <typename T>
        static T seek_in(const IntHashMap<T> & hash_map, int id);

        MapObjectReferrers m_referrers;
        IntHashMap<const MapObject *> m_id_to_object{0};
        IntHashMap<const MapObjectGroup *> m_id_to_group{0};
    };

    void load
        (GroupContainer &&,
         MapObjectContainer &&,
              XmlElementContainer && group_elements);

    IdsToElementsMap m_id_maps;
    NameObjectMap m_names_to_objects{nullptr};
    std::vector<MapObject> m_map_objects;
    GroupContainer m_groups;
    GroupIterator m_top_level_groups_end;
};
