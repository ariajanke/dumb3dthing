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

#include "../Definitions.hpp"
#include "ParseHelpers.hpp"

#include <ariajanke/cul/HashMap.hpp>
#include <ariajanke/cul/StringUtil.hpp>
#include <ariajanke/cul/Either.hpp>

#include <cstring>

// danger: does not own source element
class MapElementValuesMap final {
public:
    enum class FieldType { attribute, property };

    static constexpr const auto k_properties_tag = "properties";
    static constexpr const auto k_property_tag = "property";
    static constexpr const auto k_name_attribute = "name";
    static constexpr const auto k_value_attribute = "value";

    template <typename T>
    using EnableOptionalNumeric =
        std::enable_if_t<std::is_arithmetic_v<T>, Optional<T>>;

    struct CStringHasher final {
        std::size_t operator () (const char *) const;
    };

    struct CStringEqual final {
        bool operator () (const char *, const char *) const;
    };

    // danger: does not own source element
    void load(const TiXmlElement &);

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric(FieldType type, const char * name) const;

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric_attribute(const char * name) const
        { return get_numeric<T>(FieldType::attribute, name); }

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric_property(const char * name) const
        { return get_numeric<T>(FieldType::property, name); }

    const char * get_string(FieldType, const char * name) const;

    const char * get_string_attribute(const char * name) const;

    const char * get_string_property(const char * name) const;

private:
    struct Key final {
        Key() {}

        Key(const char * name_, FieldType type_):
            name(name_), type(type_) {}

        const char * name = "";
        FieldType type = FieldType::attribute;
    };

    struct KeyHasher final {
        std::size_t operator () (const Key & key) const;
    };

    struct KeyEqual final {
        bool operator () (const Key &, const Key &) const;
    };

    using ValuesMap = cul::HashMap<Key, const char *, KeyHasher, KeyEqual>;

    ValuesMap m_values = ValuesMap{Key{}};
};

// ----------------------------------------------------------------------------

class MapElementValuesAggregable {
public:
    using FieldType = MapElementValuesMap::FieldType;

    template <typename T>
    using EnableOptionalNumeric = MapElementValuesMap::EnableOptionalNumeric<T>;

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric(FieldType type, const char * name) const
        { return m_values_map.get_numeric<T>(type, name); }

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric_attribute(const char * name) const
        { return get_numeric<T>(FieldType::attribute, name); }

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric_property(const char * name) const
        { return get_numeric<T>(FieldType::property, name); }

    const char * get_string(FieldType field_type, const char * name) const
        { return m_values_map.get_string(field_type, name); }

    const char * get_string_attribute(const char * name) const
        { return m_values_map.get_string_attribute(name); }

    const char * get_string_property(const char * name) const
        { return m_values_map.get_string_property(name); }

protected:
    MapElementValuesAggregable() {}

    MapElementValuesAggregable(MapElementValuesMap && values_map):
        m_values_map(std::move(values_map)) {}

    void set_map_element_values_map(MapElementValuesMap && values_map)
        { m_values_map = std::move(values_map); }

private:
    MapElementValuesMap m_values_map;
};

// ----------------------------------------------------------------------------

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, Optional<T>>
    MapElementValuesMap::get_numeric(FieldType type, const char * name) const
{
    T i = 0;
    auto as_str = get_string(type, name);
    if (!as_str)
        { return {}; }
    auto end = as_str + ::strlen(as_str);
    if (cul::string_to_number(as_str, end, i))
        return i;
    return {};
}
