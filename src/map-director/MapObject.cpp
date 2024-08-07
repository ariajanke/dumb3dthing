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
#include "MapObjectGroup.hpp"

#include <tinyxml2.h>

namespace {

using ObjectGroupContainer = MapObjectGroup::GroupContainer;
using MapObjectContainer = MapObjectGroup::MapObjectContainer;

} // end of <anonymous> namespace
#if 0
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
#endif
// ----------------------------------------------------------------------------

/* static */ MapObjectFraming MapObjectFraming::load_from
    (const TiXmlElement & map_element)
{ return MapObjectFraming{ScaleComputation::pixel_scale_from_map(map_element)}; }

// ----------------------------------------------------------------------------

bool MapObject::NameLessThan::operator ()
    (const MapObject * lhs, const MapObject * rhs) const
{
    assert(lhs && rhs);
    return MapObjectGroup::find_name_predicate(lhs, rhs->name());
}

// ----------------------------------------------------------------------------

/* static */ MapObjectContainer MapObject::load_objects_from
    (View<GroupConstIterator> groups,
     View<XmlElementConstIterator> elements)
{
    if (elements.end() - elements.begin() != groups.end() - groups.begin()) {
        throw InvalidArgument{"must be same size (assumed lock stepped)"};
    }
    MapObjectContainer objects;
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
    MapElementValuesMap values_map;
    values_map.load(object_element);
    return MapObject{parent_group, std::move(values_map)};
}

/* static */ MapObject::NameObjectMap
    MapObject::find_first_visible_named_objects
    (const MapObjectContainer & objects)
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
#if 0
const char * MapObject::get_string_property(const char * name) const
    { return m_values_map.get_string_property(name); }

const char * MapObject::get_string_attribute(const char * name) const
    { return m_values_map.get_string_attribute(name); }
#endif
int MapObject::id() const {
    return verify_has_id(get_numeric_attribute<int>(k_id_attribute));
}

const char * MapObject::name() const {
    auto name_ = get_string_attribute(k_name_attribute);
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
