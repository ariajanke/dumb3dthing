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

#include "MapObjectCollection.hpp"

#include <tinyxml2.h>

namespace {

MapObjectGroup::ConstIterator empty_group_container_iterator() {
    static MapObjectGroup::GroupContainer group_container;
    return group_container.end();
}

MapObjectGroup::ObjectIterator empty_object_container_iterator() {
    static MapObjectGroup::ObjectContainer object_container;
    return object_container.end();
}

std::vector<const MapObject *> name_sorted
    (const MapObjectGroupBase::NameObjectMap & globally_visible_named_objects)
{
    std::vector<const MapObject *> name_sorted;
    name_sorted.reserve(globally_visible_named_objects.size());
    for (auto pair : globally_visible_named_objects) {
        name_sorted.emplace_back(pair.second);
    }
    std::sort(name_sorted.begin(), name_sorted.end(), MapObject::NameLessThan{});
    return name_sorted;
}

} // end of <anonymous> namespace

/* private static */ const View<MapObjectGroup::ConstIterator>
    MapObjectGroup::k_empty_group_view = View<MapObjectGroup::ConstIterator>
    {empty_group_container_iterator(), empty_group_container_iterator()};

/* private static */ const View<MapObjectGroup::ObjectIterator>
    MapObjectGroup::k_empty_object_view = View<MapObjectGroup::ObjectIterator>
    {empty_object_container_iterator(), empty_object_container_iterator()};

/* protected static */ std::vector<MapObjectGroup>
    MapObjectGroupBase::_initialize_names_and_parents_for_map
    (const DocumentOwningNode & map_element)
{
    std::vector<MapObjectGroup> groups;
    emplace_groups(groups, *map_element, 0);
    emplace_group_children(groups, *map_element, 0);
    return groups;
}

/* protected static */ Optional<MapObjectGroup>
    MapObjectGroupBase::_initialize_from_element
    (const TiXmlElement & element, int rank)
{
    auto name = element.Attribute("name");
    auto id   = element.IntAttribute("id");
    if (!id)
        { return {}; }
    if (!name)
        { name = ""; }
    return MapObjectGroup{element, name, id, rank};
}

/* protected static */ std::vector<MapObjectGroup>
    MapObjectGroupBase::_set_groups_and_ranks_for
    (std::vector<MapObjectGroup> && groups)
{
    set_groups_and_ranks_for(View{groups.begin(), groups.end()});
    return std::move(groups);
}

/* private static */ void MapObjectGroupBase::emplace_group_children
    (std::vector<MapObjectGroup> & groups,
     const TiXmlElement & any_element,
     int current_rank)
{
    for (auto * tag : k_group_tags) {
    for (auto & node : XmlRange{any_element, tag}) {
        emplace_groups(groups, node, current_rank + 1);
    }}
    for (auto * tag : k_group_tags) {
    for (auto & node : XmlRange{any_element, tag}) {
        emplace_group_children(groups, node, current_rank + 1);
    }}
}

/* private static */ void MapObjectGroupBase::emplace_groups
    (std::vector<MapObjectGroup> & groups,
     const TiXmlElement & any_element,
     int current_rank)
{
    for (auto * tag : k_group_tags) {
    for (auto & node : XmlRange{any_element, tag}) {
        auto group = _initialize_from_element(node, current_rank);
        if (!group)
            { continue; }
        groups.emplace_back(std::move(*group));
    }}
}

/* private static */ void MapObjectGroupBase::set_groups_and_ranks_for
    (View<Iterator> groups, int current_rank)
{
    if (groups.begin() == groups.end())
        { return; }
    auto next_rank_beg = groups.begin();
    while (next_rank_beg != groups.end()) {
        if (next_rank_beg->rank() != current_rank)
            { break; }
        ++next_rank_beg;
    }
    auto current_rank_end = next_rank_beg;
    for (auto itr = groups.begin(); itr != current_rank_end; ++itr) {
        auto next = itr->set_child_groups(next_rank_beg, groups.end());
        for (auto jtr = next_rank_beg; jtr != next; ++jtr) {
            jtr->set_parent(*itr);
        }
        next_rank_beg = next;
    }
    set_groups_and_ranks_for
        (View{current_rank_end, groups.end()}, current_rank + 1);
}

// ----------------------------------------------------------------------------

/* static */ Optional<MapObjectGroup>
    MapObjectGroupForTests::initialize_from_element
    (const TiXmlElement & el, int rank)
    { return _initialize_from_element(el, rank); }

// ----------------------------------------------------------------------------

/* static */ MapObjectGroup::GroupContainer
    MapObjectGroup::initialize_for_map
    (const DocumentOwningNode & map_element)
{
    return MapObjectGroupBase::_set_groups_and_ranks_for
        (MapObjectGroupBase::_initialize_names_and_parents_for_map(map_element));
}

/* static */ std::vector<MapObject>
    MapObjectGroup::assign_groups_objects
    (const NameObjectMap & globally_visible_named_objects,
     std::vector<MapObject> && all_objects,
     View<Iterator> all_groups)
{
    // make sure objects are group ordered
#   ifdef MACRO_DEBUG
    static auto group_order = []
        (const MapObjectGroup & lhs, const MapObjectGroup & rhs)
        { return lhs.rank() < rhs.rank(); };
    auto is_in_group_order =
        std::is_sorted(all_groups.begin(), all_groups.end(), group_order);
    if (!is_in_group_order) {
        throw InvalidArgument{"Groups were not given in pointer order"};
    }
#   endif

    auto object_itr = all_objects.begin();
    for (auto & group : all_groups) {
        auto next = std::find_if
            (object_itr, all_objects.end(),
             [&group](const MapObject & object)
             { return object.parent_group() != &group; });
        group.set_child_objects(View<ObjectIterator>{object_itr, next});
        object_itr = next;
    }

    for (auto & group : all_groups) {
        NameObjectMap groups_name_map = globally_visible_named_objects;
        for (auto pair_ref : groups_name_map) {
            auto found_object = group.seek_by_name(pair_ref.first);
            if (!found_object) { continue; }
            pair_ref.second = found_object;
        }
        for (auto & object : group.objects()) {
            auto itr = groups_name_map.find(object.name());
#           if MACRO_DEBUG
            assert(itr != groups_name_map.end());
#           endif
            itr->second = &object;
        }

        group.set_object_name_map(std::move(groups_name_map));
    }

    return std::move(all_objects);
}

/* static */ bool MapObjectGroup::find_name_predicate
    (const MapObject * obj, const char * object_name)
{ return ::strcmp(obj->name(), object_name) < 0; }

MapObjectGroup::MapObjectGroup
    (const TiXmlElement & element, const char * name, int id, int rank):
    m_name(name), m_element(&element), m_id(id), m_rank(rank) {}

MapObjectGroup::Iterator
    MapObjectGroup::set_child_groups
    (Iterator starting_at, Iterator end)
{
    auto group_end = [=] {
        auto itr = starting_at;
        for (auto * tag : k_group_tags) {
        for (auto & node : XmlRange{m_element, tag}) {
            if (itr == end) return itr;
#           ifdef MACRO_DEBUG
            assert(itr->rank() == rank() + 1);
#           endif
            (void)node;
            ++itr;
        }}
        return itr;
    } ();
    m_groups = View<ConstIterator>{starting_at, group_end};
    return group_end;
}

std::vector<MapObject>
    MapObjectGroup::load_child_objects
    (const MapObjectRetrieval & retrieval,
     std::vector<MapObject> && objects) const
{
    for (auto & obj_el : XmlRange{m_element, "object"}) {
        objects.emplace_back(MapObject::load_from(obj_el, *this, retrieval));
    }
    return std::move(objects);
}

const MapObject * MapObjectGroup::seek_by_name(const char * object_name) const {
    auto itr = m_object_name_map.find(object_name);
    if (itr != m_object_name_map.end()) {
        return itr->second;
    } else if (m_parent) {
        return m_parent->seek_by_name(object_name);
    }
    return nullptr;
}

void MapObjectGroup::set_child_objects(View<ObjectIterator> child_objects)
    { m_objects = child_objects; }

void MapObjectGroup::set_object_name_map(NameObjectMap && name_map)
    { m_object_name_map = std::move(name_map); }

void MapObjectGroup::set_parent(const MapObjectGroup & group)
    { m_parent = &group; }

// ----------------------------------------------------------------------------

/* static */ MapObjectCollection MapObjectCollection::load_from
    (const DocumentOwningNode & map_element)
{
    MapObjectCollection collection;
    collection.load(map_element);
    return collection;
}

void MapObjectCollection::load(const DocumentOwningNode & map_element) {
    using GroupConstIterator = MapObjectGroup::ConstIterator;
    auto groups = MapObjectGroup::initialize_for_map(map_element);
    auto objects = MapObject::load_objects_from
        (m_id_maps,
         View<GroupConstIterator>{groups.begin(), groups.end()});
    load(std::move(groups), std::move(objects));
}

const MapObject * MapObjectCollection::seek_by_name(const char * name) const {
    auto itr = m_names_to_objects.find(name);
    if (itr == m_names_to_objects.end())
        { return nullptr; }
    return itr->second;
}

/* private */ void MapObjectCollection::load
    (GroupContainer && groups, std::vector<MapObject> && objects)
{
    auto global_names = MapObject::find_first_visible_named_objects(objects);
    m_map_objects = MapObjectGroup::assign_groups_objects
        (global_names, std::move(objects), View{groups.begin(), groups.end()});
    m_groups = std::move(groups);
    m_names_to_objects = std::move(global_names);
    m_id_maps.set_group_id_map(m_groups);
    m_id_maps.set_object_id_map(m_map_objects);
    m_top_level_groups_end = std::find_if
        (m_groups.begin(),
         m_groups.end(),
         [](const MapObjectGroup & group)
         { return group.has_parent(); });
}

// ----------------------------------------------------------------------------

void MapObjectCollection::/* private */ IdsToElementsMap::
    set_object_id_map(const std::vector<MapObject> & objects)
{
    m_id_to_object.reserve(objects.size());
    for (const auto & object : objects) {
        m_id_to_object.insert(object.id(), &object);
    }
}

void MapObjectCollection::/* private */ IdsToElementsMap::
    set_group_id_map(const GroupContainer & groups)
{
    m_id_to_group.reserve(groups.size());
    for (const auto & group : groups) {
        m_id_to_group.insert(group.id(), &group);
    }
}

const MapObjectGroup * MapObjectCollection::/* private */ IdsToElementsMap::
    seek_group_by_id(int id) const
    { return seek_in(m_id_to_group, id); }

const MapObject * MapObjectCollection::/* private */ IdsToElementsMap::
    seek_object_by_id(int id) const
    { return seek_in(m_id_to_object, id); }

template <typename T>
/* private static */ T MapObjectCollection::/* private */ IdsToElementsMap::
    seek_in(const IntHashMap<T> & hash_map, int id)
{
    auto itr = hash_map.find(id);
    if (itr == hash_map.end())
        { return nullptr; }
    return itr->second;
}
