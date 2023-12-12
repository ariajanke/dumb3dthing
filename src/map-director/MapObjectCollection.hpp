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

#include "MapObject.hpp"

class MapObjectGroupBase {
public:
    using GroupContainer = MapObject::GroupContainer;
    using Iterator = GroupContainer::iterator;
    using NameObjectMap = MapObject::NameObjectMap;
    using ObjectContainer = std::vector<MapObject>;
    using ObjectIterator = ObjectContainer::const_iterator;

    static constexpr const std::array k_group_tags =
        { "objectgroup", "group" };

protected:
    // names, ranks, returned in BFS order
    static GroupContainer _initialize_names_and_parents_for_map
        (const DocumentOwningNode & map_element);

    // parent, groups
    static GroupContainer _set_groups_and_ranks_for(GroupContainer &&);

    static Optional<MapObjectGroup> _initialize_from_element
        (const TiXmlElement &, int rank);

private:
    static void emplace_group_children
        (GroupContainer &,
         const TiXmlElement & any_element,
         int current_rank);

    static void emplace_groups
        (GroupContainer &,
         const TiXmlElement & any_element,
         int current_rank);

    static void set_groups_and_ranks_for
        (View<Iterator> groups, int current_rank = 0);
};

// ----------------------------------------------------------------------------

class MapObjectGroupForTests final : public MapObjectGroupBase {
public:
    static GroupContainer initialize_names_and_parents_for_map
        (const DocumentOwningNode & map_element)
        { return _initialize_names_and_parents_for_map(map_element); }

    static GroupContainer set_groups_and_ranks_for
        (GroupContainer && groups)
        { return _set_groups_and_ranks_for(std::move(groups)); }

    static Optional<MapObjectGroup> initialize_from_element
        (const TiXmlElement & el, int rank);
};

// ----------------------------------------------------------------------------

class MapObjectGroup final : public MapObjectGroupBase {
public:
    using ConstIterator = GroupContainer::const_iterator;
    using ObjectReorderContainer = std::vector<const MapObject *>;
    using ObjectNamesIterator = ObjectReorderContainer::const_iterator;

    static GroupContainer initialize_for_map
        (const DocumentOwningNode & map_element);

    // ordered by (group, name), assigned to groups
    static std::vector<MapObject> assign_groups_objects
        (const NameObjectMap & globally_visible_named_objects,
         std::vector<MapObject> && all_objects,
         View<Iterator> all_groups);

    // MapObject::NameLessThan should be built on me
    static bool find_name_predicate
        (const MapObject * obj, const char * object_name)
    { return ::strcmp(obj->name(), object_name) < 0; }

    explicit MapObjectGroup(int id): m_id(id) {}

    MapObjectGroup
        (const TiXmlElement & element, const char * name, int id, int rank):
        m_name(name), m_element(&element), m_id(id), m_rank(rank) {}

    View<ConstIterator> groups() const { return m_groups; }

    bool has_parent() const { return static_cast<bool>(m_parent); }

    int id() const { return m_id; }

    std::vector<MapObject> load_child_objects
        (const MapObjectRetrieval &,
         std::vector<MapObject> &&) const;

    const char * name() const { return m_name; }

    View<ObjectIterator> objects() const { return m_objects; }

    int rank() const { return m_rank; }

    const MapObject * seek_by_name(const char * object_name) const {
        auto itr = m_object_name_map.find(object_name);
        if (itr != m_object_name_map.end()) {
            return itr->second;
        } else if (m_parent) {
            return m_parent->seek_by_name(object_name);
        }
        return nullptr;
    }

    void set_child_objects(View<ObjectIterator> child_objects)
        { m_objects = child_objects; }

    Iterator set_child_groups(Iterator starting_at, Iterator end);

    void set_object_name_map(NameObjectMap && name_map)
        { m_object_name_map = std::move(name_map); }

    void set_parent(const MapObjectGroup & group)
        { m_parent = &group; }

private:
    static const View<ConstIterator> k_empty_group_view;
    static const View<ObjectIterator> k_empty_object_view;

    const MapObjectGroup * m_parent = nullptr;
    const char * m_name = "";
    // v probably shouldn't, or at least null out later
    const TiXmlElement * m_element = nullptr;
    int m_id = 0;
    int m_rank = 0;
    View<ConstIterator> m_groups = k_empty_group_view;
    View<ObjectIterator> m_objects = k_empty_object_view;
    NameObjectMap m_object_name_map{nullptr};
};

// ----------------------------------------------------------------------------

class MapObjectCollection final : public MapObjectRetrieval {
public:
    using GroupContainer = MapObject::GroupContainer;
    using ObjectReorderContainer = MapObjectGroup::ObjectReorderContainer;

    static MapObjectCollection load_from(const DocumentOwningNode & map_element) {
        MapObjectCollection collection;
        collection.load(map_element);
        return collection;
    }

    MapObjectCollection() {}

    void load(const DocumentOwningNode & map_element);

    const MapObjectRetrieval * seek_group(const char *) const
        { return nullptr; }

    const MapObject * seek_by_name(const char *) const { return nullptr; }

    const MapObject * seek_object_by_id(int id) const {
        auto itr = m_id_to_object.find(id);
        if (itr == m_id_to_object.end())
            { return nullptr; }
        return itr->second;
    }

    const MapObjectGroup * top_level_group() const {
        if (m_groups.empty())
            { return nullptr; }
        return &m_groups.front();
    }

private:
    cul::HashMap<int, const MapObject *> m_id_to_object{0};
    std::vector<MapObject> m_map_objects;
    GroupContainer m_groups;
};
