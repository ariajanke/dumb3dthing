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

    static bool find_name_predicate
        (const MapObject * obj, const char * object_name);

    explicit MapObjectGroup(int id): m_id(id) {}

    MapObjectGroup
        (const TiXmlElement & element, const char * name, int id, int rank);

    View<ConstIterator> groups() const { return m_groups; }

    bool has_parent() const { return static_cast<bool>(m_parent); }

    int id() const { return m_id; }

    std::vector<MapObject> load_child_objects
        (const MapObjectRetrieval &,
         std::vector<MapObject> &&) const;

    const char * name() const { return m_name; }

    View<ObjectIterator> objects() const { return m_objects; }

    int rank() const { return m_rank; }

    const MapObject * seek_by_name(const char * object_name) const;

    void set_child_objects(View<ObjectIterator> child_objects);

    Iterator set_child_groups(Iterator starting_at, Iterator end);

    void set_object_name_map(NameObjectMap && name_map);

    void set_parent(const MapObjectGroup & group);

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

class MapObjectCollection final {
public:
    using GroupContainer = MapObject::GroupContainer;
    using GroupIterator = GroupContainer::const_iterator;
    using ObjectReorderContainer = MapObjectGroup::ObjectReorderContainer;
    using NameObjectMap = MapObjectGroup::NameObjectMap;

    static MapObjectCollection load_from(const DocumentOwningNode & map_element);

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

        const MapObjectGroup * seek_group_by_id(int id) const final;

        const MapObject * seek_object_by_id(int id) const final;

    private:
        template <typename T>
        static T seek_in(const IntHashMap<T> & hash_map, int id);

        IntHashMap<const MapObject *> m_id_to_object{0};
        IntHashMap<const MapObjectGroup *> m_id_to_group{0};
    };

    void load(GroupContainer &&, std::vector<MapObject> &&);

    IdsToElementsMap m_id_maps;
    NameObjectMap m_names_to_objects{nullptr};
    std::vector<MapObject> m_map_objects;
    GroupContainer m_groups;
    GroupIterator m_top_level_groups_end;
};
