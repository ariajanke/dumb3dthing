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
#include "MapObjectCollection.hpp"

#include <tinyxml2.h>

#include <cstring>

namespace {

using ObjectGroupContainer = MapObjectGroup::GroupContainer;
using MapObjectContainer = MapObjectGroup::ObjectContainer;

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

// ----------------------------------------------------------------------------

/* static */ MapObjectFraming MapObjectFraming::load_from
    (const TiXmlElement & map_element)
{ return MapObjectFraming{ScaleComputation::pixel_scale_from_map(map_element)}; }

// ----------------------------------------------------------------------------

bool MapObject::CStringEqual::operator ()
    (const char * lhs, const char * rhs) const
{
    if (!lhs || !rhs) {
        return lhs == rhs;
    }
    return ::strcmp(lhs, rhs) == 0;
}


// ----------------------------------------------------------------------------

bool MapObject::NameLessThan::operator ()
    (const MapObject * lhs, const MapObject * rhs) const
{
    assert(lhs && rhs);
    return MapObjectGroup::find_name_predicate(lhs, rhs->name());
}

// ----------------------------------------------------------------------------

/* static */ std::vector<MapObject> MapObject::load_objects_from
    (View<GroupConstIterator> groups,
     View<std::vector<const TiXmlElement *>::const_iterator> elements)
{
    if (elements.end() - elements.begin() != groups.end() - groups.begin()) {
        throw InvalidArgument{"must be same size (assumed lock stepped)"};
    }
    std::vector<MapObject> objects;
    auto el_itr = elements.begin();
    for (const auto & group : groups) {
        objects = group.load_child_objects(std::move(objects), *(*el_itr++));
    }
    return objects;
}

/* static */ MapObject MapObject::load_from
    (const TiXmlElement & object_element,
     const MapObjectGroup & parent_group)
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
    return MapObject{parent_group, std::move(map)};
}

/* static */ MapObject::NameObjectMap
    MapObject::find_first_visible_named_objects
    (const std::vector<MapObject> & objects)
{
    NameObjectMap map{nullptr};
    map.reserve(objects.size());
    for (auto & object : objects) {
        (void)map.insert(object.name(), &object);
    }
    return map;
}

const MapObjectGroup * MapObject::get_group_property(const char * name) const {
    auto maybe_id = get_numeric_property<int>(name);
    if (!maybe_id)
        { return nullptr; }
    return m_parent_retrieval->seek_group_by_id(*maybe_id);
}

const MapObject * MapObject::get_object_property(const char * name) const {
    auto maybe_id = get_numeric<int>(FieldType::property, name);
    if (!maybe_id)
        { return nullptr; }
    return m_parent_retrieval->seek_object_by_id(*maybe_id);
}

const char * MapObject::get_string_property(const char * name) const
    { return get_string(FieldType::property, name); }

const char * MapObject::get_string_attribute(const char * name) const
    { return get_string(FieldType::attribute, name); }

int MapObject::id() const {
    return verify_has_id(get_numeric_attribute<int>("id"));
}

const char * MapObject::name() const {
    auto name_ = get_string_attribute("name");
    if (name_) return name_;
    return "";
}

const MapObject * MapObject::seek_by_object_name
    (const char * object_name) const
{
    if (!m_parent_group) return nullptr;
    return m_parent_group->seek_by_name(object_name);
}

/* private static */ int MapObject::verify_has_id(Optional<int> maybe_id) {
    if (maybe_id) return *maybe_id;
    throw RuntimeError{"objects are expect to always have ids"};
}

/* private */ MapObject::MapObject
    (const MapObjectGroup & parent_group,
     ValuesMap && values,
     const MapObjectRetrieval & retrieval):
    m_parent_group(&parent_group),
    m_values(std::move(values)),
    m_parent_retrieval(&retrieval) {}

/* private */ const char * MapObject::get_string
    (FieldType type, const char * name) const
{
    auto itr = m_values.find(Key{name, type});
    if (itr == m_values.end()) return nullptr;
    return itr->second;
}

// ----------------------------------------------------------------------------

std::size_t MapObject::/* private */ KeyHasher::
    operator () (const Key & key) const
{
    std::size_t temp = key.type == FieldType::attribute ? 0 : ~0;
    return temp ^ CStringHasher{}(key.name);
}

bool MapObject::/* private */ KeyEqual::
    operator () (const Key & lhs, const Key & rhs) const
{ return lhs.type == rhs.type && CStringEqual{}(lhs.name, rhs.name); }
