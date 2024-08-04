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

Either<MapObjectVectorFraming::LoadFailed, Vector>
    MapObjectVectorFraming::operator ()
    (const MapItemPropertiesRetrieval & as_aggregable) const
{
    return m_z_framing(m_y_framing(m_x_framing(Vector{}, as_aggregable), as_aggregable), as_aggregable);
}

// ----------------------------------------------------------------------------

/* static */ MapObjectFraming MapObjectFraming::load_from
    (const TiXmlElement & map_element)
{ return MapObjectFraming{ScaleComputation::pixel_scale_from_map(map_element)}; }

Either<MapObjectFraming::LoadFailed, Vector>
    MapObjectFraming::get_position_from
    (const MapItemPropertiesRetrieval & object,
     const MapObjectVectorFraming & framing) const
{
    return framing(object).map([this] (Vector && r) {
        return m_map_pixel_scale.of(r);
    });
}

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
     View<XmlElementConstIterator> elements,
     const DocumentOwningXmlElement & owner)
{
    if (elements.end() - elements.begin() != groups.end() - groups.begin()) {
        throw InvalidArgument{"must be same size (assumed lock stepped)"};
    }
    MapObjectContainer objects;
    auto el_itr = elements.begin();
    for (const auto & group : groups) {
        objects = group.load_child_objects
            (std::move(objects), owner.make_with_same_owner(*(*el_itr++)));
    }
    return objects;
}

/* static */ MapObject MapObject::load_from
    (const DocumentOwningXmlElement & object_element,
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
