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

#include "MapObject.hpp"

#include <tinyxml2.h>

#include <cstring>

namespace {

using MapObjectGroupOrdering = MapObjectGroup::MapObjectGroupOrdering;
using ObjectGroupContainer = MapObjectGroup::GroupContainer;
using MapObjectReorderContainer = MapObjectGroup::ObjectReorderContainer;
using MapObjectContainer = MapObjectGroup::ObjectContainer;

const ObjectGroupContainer k_empty_group_container;
const MapObjectReorderContainer k_empty_reordering_container;
const MapObjectContainer k_empty_object_container;

static const auto k_size_t_length_empty_string = [] {
    std::array<char, sizeof(std::size_t)> buf;
    std::fill(buf.begin(), buf.end(), 0);
    return buf;
} ();

static constexpr const std::size_t k_hash_string_length_limit = 3;

std::size_t limited_string_length(const char * str) {
    std::size_t remaining = k_hash_string_length_limit;
    while (remaining && *str) {
        --remaining;
        ++str;
    }
    return k_hash_string_length_limit - remaining;
}

} // end of <anonymous> namespace

/* static */ Optional<DocumentOwningNode>
    DocumentOwningNode::load_root(std::string && file_contents)
{
    struct OwnerImpl final : public Owner {
        TiXmlDocument document;
    };
    auto owner = make_shared<OwnerImpl>();
    auto & document = owner->document;
    if (document.Parse(file_contents.c_str()) != tinyxml2::XML_SUCCESS) {
        return {};
    }
    return DocumentOwningNode{owner, *document.RootElement()};
}

DocumentOwningNode DocumentOwningNode::make_with_same_owner
    (const TiXmlElement & same_document_element) const
{ return DocumentOwningNode{m_owner, same_document_element}; }

const TiXmlElement & DocumentOwningNode::element() const
    { return *m_element; }

// ----------------------------------------------------------------------------

const MapObject * MapObject::seek_by_name(const char * object_name) const {
    if (!m_parent_group) return nullptr;
    return m_parent_group->seek_by_name(object_name);
}

// ----------------------------------------------------------------------------

/* static */ std::vector<MapObjectGroup>
    MapObjectGroup::initialize_names_and_parents_for_map
    (const DocumentOwningNode & map_element)
{
    std::vector<MapObjectGroup> groups;
    emplace_groups(groups, *map_element, 0);
    emplace_group_children(groups, *map_element, 0);
    return groups;
}

/* static */ Optional<MapObjectGroup>
    MapObjectGroup::initialize_from_element
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

/* static */ std::vector<MapObjectGroup>
    MapObjectGroup::set_groups_and_ranks_for
    (std::vector<MapObjectGroup> && groups)
{
    set_groups_and_ranks_for(View{groups.begin(), groups.end()});
    return std::move(groups);
}

/* static */ MapObjectGroupOrdering
    MapObjectGroup::assign_groups_objects
    (const NameObjectMap & globally_visible_named_objects,
     std::vector<MapObject> && all_objects,
     View<Iterator> all_groups)
{
    // make sure objects are group ordered
    static auto group_order = [](const MapObject & lhs, const MapObject & rhs)
        { return lhs.parent_group() < rhs.parent_group(); };
    auto is_in_group_order = std::is_sorted(all_objects.begin(), all_objects.end(), group_order);
    std::sort(all_objects.begin(), all_objects.end(), group_order);

    // for each group, get a map containing visible names
    std::vector<const MapObject *> group_name_ordered_objects =
    [&all_objects, &all_groups, &globally_visible_named_objects] {
        std::vector<const MapObject *> group_name_ordered_objects;
        group_name_ordered_objects.reserve
            ((all_groups.end() - all_groups.end())*
             globally_visible_named_objects.size());
        auto object_itr = all_objects.begin();
        NameObjectMap temp{nullptr};
        for (auto & group : all_groups) {
            if (object_itr != all_objects.end()) {
                assert(&group == object_itr->parent_group());
            }
            temp = globally_visible_named_objects;
            auto next = object_itr;
            for (; next != all_objects.end(); ++next) {
                auto name = next->name();
                if (!name)
                    { continue; }

                auto named_object_itr = temp.find(*name);
                // if named, it has *got* to be on the table
                assert(named_object_itr != temp.end());
                named_object_itr->second = &*next;

                if (next->parent_group() != &group)
                    { break; }
            }
            for (auto pair : temp) {
                group_name_ordered_objects.emplace_back(pair.second);
            }
        }
        return group_name_ordered_objects;
    } ();

    // okay, everything's sorted, now just modify groups
    auto object_itr = all_objects.begin();
    auto name_itr = group_name_ordered_objects.begin();
    for (auto & group : all_groups) {
        auto end_object_itr = object_itr;
        for (; end_object_itr != all_objects.end(); ++end_object_itr) {
            if (end_object_itr->parent_group() != &group)
                { break; }
        }
        auto end_name_itr = name_itr;
        for (; end_name_itr != group_name_ordered_objects.end(); ++end_name_itr) {
            if ((**end_name_itr).parent_group() != &group)
                { break; }
        }
        group.set_child_objects
            (View<ObjectNamesIterator>{name_itr, end_name_itr},
             View<ObjectIterator>{object_itr, end_object_itr});
    }

    MapObjectGroupOrdering rv;
    rv.group_name_ordered_objects = std::move(group_name_ordered_objects);
    rv.group_ordered_objects = std::move(all_objects);
    return rv;
}

/* private static */ const View<MapObjectGroup::ConstIterator>
    MapObjectGroup::k_empty_group_view = View<MapObjectGroup::ConstIterator>
    {k_empty_group_container.end(), k_empty_group_container.end()};

/* private static */ const View<MapObjectGroup::ObjectNamesIterator>
    MapObjectGroup::k_empty_object_reorder_view =
    View<MapObjectGroup::ObjectNamesIterator>
    {k_empty_reordering_container.end(), k_empty_reordering_container.end()};

/* private static */ const View<MapObjectGroup::ObjectIterator>
    MapObjectGroup::k_empty_object_view = View<MapObjectGroup::ObjectIterator>
    {k_empty_object_container.end(), k_empty_object_container.end()};

/* private static */ void MapObjectGroup::emplace_group_children
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

/* private static */ void MapObjectGroup::emplace_groups
    (std::vector<MapObjectGroup> & groups,
     const TiXmlElement & any_element,
     int current_rank)
{
    for (auto * tag : k_group_tags) {
    for (auto & node : XmlRange{any_element, tag}) {
        auto group = initialize_from_element(node, current_rank);
        if (!group)
            { continue; }
        groups.emplace_back(std::move(*group));
    }}
}

/* private static */ void MapObjectGroup::set_groups_and_ranks_for
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
        next_rank_beg = itr->set_child_groups(next_rank_beg, groups.end());
    }
    set_groups_and_ranks_for
        (View{current_rank_end, groups.end()}, current_rank + 1);
}

/* private */ MapObjectGroup::Iterator
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

/* private */ std::vector<MapObject>
    MapObjectGroup::load_child_objects
    (const MapObjectRetrieval & retrieval,
     std::vector<MapObject> && objects) const
{
    for (auto & obj_el : XmlRange{m_element, "object"}) {
        objects.emplace_back(MapObject::load_from(obj_el, *this, retrieval));
    }
    return std::move(objects);
}

// ----------------------------------------------------------------------------

std::size_t MapObject::CStringHasher::operator () (const char * cstr) const {
    std::size_t temp = 0;
    auto left = limited_string_length(cstr);
    while (left >= sizeof(std::size_t)) {
        temp ^= *reinterpret_cast<const std::size_t *>(cstr);
        cstr += sizeof(std::size_t);
        left -= sizeof(std::size_t);
    }

    assert(left < sizeof(std::size_t));
    auto buf = k_size_t_length_empty_string;
    for (; left; --left) {
        buf[left] = cstr[left - 1];
    }
    return temp ^ *reinterpret_cast<const std::size_t *>(&buf.front());
}

bool MapObject::CStringEqual:: operator ()
    (const char * lhs, const char * rhs) const
{
    if (!lhs || !rhs) {
        return lhs == rhs;
    }
    return ::strcmp(lhs, rhs) == 0;
}

bool MapObject::NameLessThan::operator ()
    (const MapObject * lhs, const MapObject * rhs) const
{
    assert(lhs && rhs);
    return ::strcmp(lhs->name().value_or(""), rhs->name().value_or("")) < 0;
}

template <typename Func>
void for_each_object_kv_pair
    (const TiXmlElement & object_element,
     Func && f)
{
    using FieldType = MapObject::FieldType;
    for (auto attr = object_element.FirstAttribute(); attr; attr = attr->Next()) {
        auto * name = attr->Name();
        const auto * value = attr->Value();
        if (!name || !value) continue;
        f(MapObject::Key{name, FieldType::attribute}, value);
    }
    auto properties = object_element.FirstChildElement("properties");
    if (!properties)
        return;
    for (auto & prop : XmlRange{properties, "property"}) {
        auto * name = prop.Attribute("name");
        auto * value = prop.Attribute("value");
        if (!name || !value) continue;
        f(MapObject::Key{name, FieldType::property}, value);
    }
}

/* static */ MapObject MapObject::load_from
    (const TiXmlElement & object_element,
     const MapObjectGroup & parent_group,
     const MapObjectRetrieval & retrieval)
{
    int count = 0;
    for_each_object_kv_pair
        (object_element,
         [&count] (const Key &, const char *) { ++count; });
    ValuesMap map{Key{}};
    map.reserve(count);
    for_each_object_kv_pair
        (object_element,
         [&map] (const Key & key, const char * value) {
            map.insert(key, value);
         });
    return MapObject{parent_group, std::move(map), retrieval};
}

/* static */ std::vector<MapObject>
    MapObject::load_from
    (const MapObjectRetrieval & retrieval,
     View<GroupConstIterator> groups)
{
    std::vector<MapObject> objects;
    for (const auto & group : groups) {
        objects = group.load_child_objects(retrieval, std::move(objects));
    }
    return objects;
}

/* static */ MapObject::NameObjectMap
    MapObject::find_first_visible_named_objects
    (const std::vector<MapObject> & objects)
{
    NameObjectMap map{nullptr};
    map.reserve(objects.size());
    for (auto & object : objects) {
        if (auto name = object.name()) {
            (void)map.insert(*name, &object);
        }
    }
    return map;
}

// ----------------------------------------------------------------------------

std::size_t MapObject::KeyHasher::operator () (const Key & key) const {
    std::size_t temp = key.type == FieldType::attribute ? 0 : ~0;
    return temp ^ CStringHasher{}(key.name);
}

bool MapObject::KeyEqual::operator () (const Key & lhs, const Key & rhs) const {
    return lhs.type == rhs.type && CStringEqual{}(lhs.name, rhs.name);
}

// ----------------------------------------------------------------------------

void MapObjectCollection::load(const DocumentOwningNode & map_element) {
    using GroupConstIterator = MapObjectGroup::ConstIterator;
    auto groups = MapObjectGroup::set_groups_and_ranks_for
        (MapObjectGroup::initialize_names_and_parents_for_map(map_element));
    auto objects = MapObject::load_from
        (MapObjectRetrieval::null_instance(),
         View<GroupConstIterator>{groups.begin(), groups.end()});
    auto global_names = MapObject::find_first_visible_named_objects(objects);
    auto reordering = MapObjectGroup::assign_groups_objects
        (global_names, std::move(objects), View{groups.begin(), groups.end()});
    m_reordered_objects = std::move(reordering.group_name_ordered_objects);
    m_map_objects = std::move(reordering.group_ordered_objects);
    m_groups = std::move(groups);
    for (auto & map_object : m_map_objects) {
        m_id_to_object.insert(map_object.id(), &map_object);
    }
}
