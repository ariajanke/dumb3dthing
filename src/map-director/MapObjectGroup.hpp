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
    using MapObjectContainer = MapObject::MapObjectContainer;
    using MapObjectConstIterator = MapObjectContainer::const_iterator;
    using XmlElementContainer = MapObject::XmlElementContainer;
    using XmlElementConstIterator = MapObject::XmlElementConstIterator;

    static constexpr const std::array k_group_tags =
        { "objectgroup", "group" };

protected:
    static XmlElementContainer
        _groups_in_bfs_order(const DocumentOwningNode &);

    // returned in BFS order
    static Tuple<GroupContainer, XmlElementContainer>
        _initialize_names_and_parents_for_map
        (const DocumentOwningNode & map_element);

    static Tuple<GroupContainer, XmlElementContainer>
        _set_groups_and_ranks_for(Tuple<GroupContainer, XmlElementContainer> &&);

    static Optional<MapObjectGroup> _initialize_from_element
        (const TiXmlElement &, int rank);

private:
    static void emplace_group_children
        (GroupContainer &,
         XmlElementContainer &,
         const TiXmlElement & any_element,
         int current_rank);

    static void emplace_groups
        (GroupContainer &,
         XmlElementContainer &,
         const TiXmlElement & any_element,
         int current_rank);

    static void set_groups_and_ranks_for
        (View<Iterator> groups,
         View<XmlElementConstIterator> elements,
         int current_rank = 0);
};

// ----------------------------------------------------------------------------

class MapObjectGroupForTests final : public MapObjectGroupBase {
public:
    static auto initialize_names_and_parents_for_map
        (const DocumentOwningNode & map_element)
        { return _initialize_names_and_parents_for_map(map_element); }

    static auto set_groups_and_ranks_for
        (Tuple<GroupContainer, XmlElementContainer> && containers)
        { return _set_groups_and_ranks_for(std::move(containers)); }

    static Optional<MapObjectGroup> initialize_from_element
        (const TiXmlElement & el, int rank);
};

// ----------------------------------------------------------------------------

class MapObjectGroup final : public MapObjectGroupBase {
public:
    using ConstIterator = GroupContainer::const_iterator;

    static MapObjectContainer assign_groups_objects
        (const NameObjectMap & globally_visible_named_objects,
         MapObjectContainer && all_objects,
         View<Iterator> all_groups);

    static bool find_name_predicate
        (const MapObject * obj, const char * object_name);

    static Tuple<GroupContainer, XmlElementContainer>
        initialize_for_map
        (const DocumentOwningNode & map_element);

    static constexpr const auto k_object_tag = "object";

    MapObjectGroup() {}

    explicit MapObjectGroup(int id): m_id(id) {}

    MapObjectGroup
        (const char * name, int id, int rank);

    View<ConstIterator> groups() const { return m_groups; }

    bool has_parent() const { return static_cast<bool>(m_parent); }

    int id() const { return m_id; }

    MapObjectContainer load_child_objects
        (MapObjectContainer &&, const TiXmlElement &) const;

    const char * name() const { return m_name; }

    View<MapObjectConstIterator> objects() const { return m_objects; }

    int rank() const { return m_rank; }

    const MapObject * seek_by_name(const char * object_name) const;

    Iterator set_child_groups
        (const TiXmlElement & group_xml, Iterator starting_at, Iterator end);

    void set_child_objects(View<MapObjectConstIterator> child_objects);

    void set_object_name_map(NameObjectMap && name_map);

    void set_parent(const MapObjectGroup & group);

private:
    static const View<ConstIterator> k_empty_group_view;
    static const View<MapObjectConstIterator> k_empty_object_view;

    const MapObjectGroup * m_parent = nullptr;
    const char * m_name = "";
    int m_id = 0;
    int m_rank = 0;
    View<ConstIterator> m_groups = k_empty_group_view;
    View<MapObjectConstIterator> m_objects = k_empty_object_view;
    NameObjectMap m_object_name_map{nullptr};
};
