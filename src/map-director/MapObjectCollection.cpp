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

using XmlElementContainer = MapObject::XmlElementContainer;

class MapObjectReferrersInserter final {
public:
    using ObjectViewMap = MapObjectReferrers::ObjectViewMap;

    void add(const MapObject & referrer, const MapObject & target);

    MapObjectReferrers finish() &&;

private:
    struct RefPair final {
        const MapObject * referrer = nullptr;
        const MapObject * target = nullptr;
    };

    static bool target_order(const RefPair & lhs, const RefPair & rhs)
        { return lhs.target < rhs.target; }

    std::vector<RefPair> m_object_pairs;
};

MapObjectReferrers
    referrers_from
    (const MapObjectRetrieval & object_retrieval,
     const XmlElementContainer & group_elements);

} // end of <anonymous> namespace

MapObjectReferrers::MapObjectReferrers
    (MapObjectContainer && object_refs,
     ObjectViewMap && view_map):
    m_object_refs(std::move(object_refs)),
    m_view_map(std::move(view_map)) {}

View<MapObjectReferrers::MapObjectConstIterator>
    MapObjectReferrers::get_referrers(int id) const
{
    auto itr = m_view_map.find(id);
    if (itr == m_view_map.end()) {
        return View{m_object_refs.end(), m_object_refs.end()};
    }
    return itr->second;
}

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
    auto [groups, elements] = MapObjectGroup::initialize_for_map(map_element);
    auto objects = MapObject::load_objects_from
        (View<GroupConstIterator>{groups.begin(), groups.end()},
         View{elements.cbegin(), elements.cend()});
    load(std::move(groups), std::move(objects), std::move(elements));
}

const MapObject * MapObjectCollection::seek_by_name(const char * name) const {
    auto itr = m_names_to_objects.find(name);
    if (itr == m_names_to_objects.end())
        { return nullptr; }
    return itr->second;
}

/* private */ void MapObjectCollection::load
    (GroupContainer && groups,
     MapObjectContainer && objects,
     XmlElementContainer && group_elements)
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
    for (auto & object : m_map_objects) {
        object.set_by_id_retrieval(m_id_maps);
    }
    m_id_maps.set_referrers(referrers_from(m_id_maps, group_elements));
}

// ----------------------------------------------------------------------------

void MapObjectCollection::/* private */ IdsToElementsMap::
    set_object_id_map(const MapObjectContainer & objects)
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

namespace {

void MapObjectReferrersInserter::add
    (const MapObject & referrer, const MapObject & target)
{
    RefPair pair;
    pair.referrer = &referrer;
    pair.target = &target;
    m_object_pairs.push_back(pair);
}

MapObjectReferrers MapObjectReferrersInserter::finish() && {
    if (m_object_pairs.empty())
        { return MapObjectReferrers{}; }

    std::sort(m_object_pairs.begin(), m_object_pairs.end(), target_order);
    std::vector<const MapObject *> referrers;
    referrers.reserve(m_object_pairs.size());
    for (const auto & pair : m_object_pairs) {
        referrers.push_back(pair.referrer);
    }

    ObjectViewMap view_map{0};
    auto last_referrer_itr = referrers.begin();
    auto * last_target = m_object_pairs.front().target;
    for (std::size_t idx = 1; idx != referrers.size(); ++idx) {
        auto * target = m_object_pairs[idx].target;
        auto referrer_itr = referrers.begin() + idx;
        if (target != last_target) {
            view_map.emplace(last_target->id(), last_referrer_itr, referrer_itr);
            last_referrer_itr = referrer_itr;
        }
        last_target = target;
    }
    view_map.emplace(last_target->id(), last_referrer_itr, referrers.end());
    return MapObjectReferrers{std::move(referrers), std::move(view_map)};
}

MapObjectReferrers
    referrers_from
    (const MapObjectRetrieval & object_retrieval,
     const XmlElementContainer & group_elements)
{
    static constexpr const auto k_object_tag = MapObjectGroup::k_object_tag;
    MapObjectReferrersInserter inserter;
    for (auto & group_el : group_elements) {
    for (auto & object_xml : XmlRange{group_el, k_object_tag}) {
        auto * properties = object_xml.
            FirstChildElement(MapObject::k_properties_tag);
        auto * object = object_retrieval.seek_object_by_id
            (object_xml.IntAttribute(MapObject::k_id_attribute));
        if (!properties || !object)
            { continue; }
        for (auto & property : XmlRange{properties, MapObject::k_property_tag}) {
            const char * type = property.Attribute("type");
            if (type && ::strcmp(type, k_object_tag) != 0)
                { continue; }
            auto target = object_retrieval.
                seek_object_by_id(property.
                    IntAttribute(MapObject::k_value_attribute));
            if (!target)
                { continue; }
            inserter.add(*object, *target);
        }
    }}
    return std::move(inserter).finish();
}

} // end of <anonymous> namespace
