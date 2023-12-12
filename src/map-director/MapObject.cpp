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
using MapObjectReorderContainer = MapObjectGroup::ObjectReorderContainer;
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

const MapObject * MapObject::seek_by_name(const char * object_name) const {
    if (!m_parent_group) return nullptr;
    return m_parent_group->seek_by_name(object_name);
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

bool MapObject::CStringEqual::operator ()
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
    return ::strcmp(lhs->name(), rhs->name()) < 0;
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
    MapObject::load_objects_from
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
        (void)map.insert(object.name(), &object);
    }
    return map;
}

const MapObject * MapObject::get_object_property(const char * name) const {
    auto maybe_id = get_arithmetic<int>(FieldType::property, name);
    if (!maybe_id)
        { return nullptr; }
    // seek by id...
    //return m_parent_group->see;
    throw "";
}

const char * MapObject::name() const {
    auto name_ = get_string_attribute("name");
    if (name_) return name_;
    return "";
}

// ----------------------------------------------------------------------------

std::size_t MapObject::KeyHasher::operator () (const Key & key) const {
    std::size_t temp = key.type == FieldType::attribute ? 0 : ~0;
    return temp ^ CStringHasher{}(key.name);
}

bool MapObject::KeyEqual::operator () (const Key & lhs, const Key & rhs) const {
    return lhs.type == rhs.type && CStringEqual{}(lhs.name, rhs.name);
}
