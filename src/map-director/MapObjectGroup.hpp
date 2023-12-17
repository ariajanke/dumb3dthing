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
    static Tuple<GroupContainer, std::vector<const TiXmlElement *>>
        _initialize_names_and_parents_for_map
        (const DocumentOwningNode & map_element);

    // parent, groups
    static Tuple<GroupContainer, std::vector<const TiXmlElement *>>
        _set_groups_and_ranks_for(Tuple<GroupContainer, std::vector<const TiXmlElement *>> &&);

    static Optional<MapObjectGroup> _initialize_from_element
        (const TiXmlElement &, int rank);

private:
    static void emplace_group_children
        (GroupContainer &,
         std::vector<const TiXmlElement *> &,
         const TiXmlElement & any_element,
         int current_rank);

    static void emplace_groups
        (GroupContainer &,
         std::vector<const TiXmlElement *> &,
         const TiXmlElement & any_element,
         int current_rank);

    static void set_groups_and_ranks_for
        (View<Iterator> groups,
         View<std::vector<const TiXmlElement *>::const_iterator> elements,
         int current_rank = 0);
};

// ----------------------------------------------------------------------------

class MapObjectGroupForTests final : public MapObjectGroupBase {
public:
    static auto initialize_names_and_parents_for_map
        (const DocumentOwningNode & map_element)
        { return _initialize_names_and_parents_for_map(map_element); }

    static auto set_groups_and_ranks_for
        (Tuple<GroupContainer, std::vector<const TiXmlElement *>> && containers)
        { return _set_groups_and_ranks_for(std::move(containers)); }

    static Optional<MapObjectGroup> initialize_from_element
        (const TiXmlElement & el, int rank);
};

// ----------------------------------------------------------------------------

class MapObjectGroup final : public MapObjectGroupBase {
public:
    using ConstIterator = GroupContainer::const_iterator;

    // ordered by (group, name), assigned to groups
    static std::vector<MapObject> assign_groups_objects
        (const NameObjectMap & globally_visible_named_objects,
         std::vector<MapObject> && all_objects,
         View<Iterator> all_groups);

    static bool find_name_predicate
        (const MapObject * obj, const char * object_name);

    static Tuple<GroupContainer, std::vector<const TiXmlElement *>>
        initialize_for_map
        (const DocumentOwningNode & map_element);

    MapObjectGroup() {}

    explicit MapObjectGroup(int id): m_id(id) {}

    MapObjectGroup
        (const char * name, int id, int rank);

    View<ConstIterator> groups() const { return m_groups; }

    bool has_parent() const { return static_cast<bool>(m_parent); }

    int id() const { return m_id; }

    std::vector<MapObject> load_child_objects
        (std::vector<MapObject> &&,
         const TiXmlElement &) const;

    const char * name() const { return m_name; }

    View<ObjectIterator> objects() const { return m_objects; }

    int rank() const { return m_rank; }

    const MapObject * seek_by_name(const char * object_name) const;

    void set_child_objects(View<ObjectIterator> child_objects);

    Iterator set_child_groups
        (const TiXmlElement & group_xml, Iterator starting_at, Iterator end);

    void set_object_name_map(NameObjectMap && name_map);

    void set_parent(const MapObjectGroup & group);

private:
    static const View<ConstIterator> k_empty_group_view;
    static const View<ObjectIterator> k_empty_object_view;

    const MapObjectGroup * m_parent = nullptr;
    const char * m_name = "";
    int m_id = 0;
    int m_rank = 0;
    View<ConstIterator> m_groups = k_empty_group_view;
    View<ObjectIterator> m_objects = k_empty_object_view;
    NameObjectMap m_object_name_map{nullptr};
};
