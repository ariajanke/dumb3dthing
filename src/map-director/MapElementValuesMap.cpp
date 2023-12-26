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

#include "MapElementValuesMap.hpp"

#include <tinyxml2.h>

namespace {

static const auto k_size_t_length_empty_string = [] {
    std::array<char, sizeof(std::size_t)> buf;
    std::fill(buf.begin(), buf.end(), 0);
    return buf;
} ();

static constexpr const std::size_t k_hash_string_length_limit = 3;

std::size_t limited_string_length(const char *);

template <typename Func>
void for_each_object_kv_pair(const TiXmlElement & object_element, Func &&);

} // end of <anonymous> namespace


std::size_t MapElementValuesMap::CStringHasher::operator () (const char * cstr) const {
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

bool MapElementValuesMap::CStringEqual::operator ()
    (const char * lhs, const char * rhs) const
{
    if (!lhs || !rhs) {
        return lhs == rhs;
    }
    return ::strcmp(lhs, rhs) == 0;
}

// ----------------------------------------------------------------------------

void MapElementValuesMap::load(const TiXmlElement & element) {
    int count = 0;
    for_each_object_kv_pair
        (element,
         [&count] (FieldType, const char *, const char *) { ++count; });
    ValuesMap map{Key{}};
    map.reserve(count);
    for_each_object_kv_pair
        (element,
         [&map] (FieldType field_type, const char * name, const char * value) {
            map.insert(Key{name, field_type}, value);
         });
    m_values = std::move(map);
}

const char * MapElementValuesMap::get_string
    (FieldType field_type, const char * name) const
{
    auto itr = m_values.find(Key{name, field_type});
    if (itr == m_values.end()) return nullptr;
    return itr->second;
}

const char * MapElementValuesMap::get_string_attribute(const char * name) const
    { return get_string(FieldType::attribute, name); }

const char * MapElementValuesMap::get_string_property(const char * name) const
    { return get_string(FieldType::property, name); }

// ----------------------------------------------------------------------------

std::size_t MapElementValuesMap::/* private */ KeyHasher::
    operator () (const Key & key) const
{
    std::size_t temp = key.type == FieldType::attribute ? 0 : ~0;
    return temp ^ CStringHasher{}(key.name);
}

bool MapElementValuesMap::/* private */ KeyEqual::
    operator () (const Key & lhs, const Key & rhs) const
{ return lhs.type == rhs.type && CStringEqual{}(lhs.name, rhs.name); }

namespace {

constexpr const auto k_property_tag = MapElementValuesMap::k_property_tag;

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
    using FieldType = MapElementValuesMap::FieldType;
    for (auto attr = object_element.FirstAttribute(); attr; attr = attr->Next()) {
        auto * name = attr->Name();
        const auto * value = attr->Value();
        if (!name || !value) continue;
        f(FieldType::attribute, name, value);
    }
    auto properties = object_element.FirstChildElement
        (MapElementValuesMap::k_properties_tag);
    if (!properties)
        return;
    for (auto & prop : XmlRange{properties, k_property_tag}) {
        auto * name = prop.Attribute(MapElementValuesMap::k_name_attribute);
        auto * value = prop.Attribute(MapElementValuesMap::k_value_attribute);
        if (!name || !value) continue;
        f(FieldType::property, name, value);
    }
}

} // end of <anonymous> namespace
