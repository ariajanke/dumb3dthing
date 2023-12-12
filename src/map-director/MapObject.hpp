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

#include <cstring>

class MapObject;
class MapObjectGroup;

class DocumentOwningNode final {
public:
    static Optional<DocumentOwningNode> load_root(std::string && file_contents);

    DocumentOwningNode() {}

    DocumentOwningNode make_with_same_owner
        (const TiXmlElement & same_document_element) const;

    const TiXmlElement * operator -> () const { return &element(); }

    const TiXmlElement & operator * () const { return element(); }

    const TiXmlElement & element() const;

    explicit operator bool() const { return m_element; }

private:
    struct Owner {
        virtual ~Owner() {}
    };

    DocumentOwningNode
        (const SharedPtr<Owner> & owner, const TiXmlElement & element_):
        m_owner(owner), m_element(&element_) {}

    SharedPtr<Owner> m_owner;
    const TiXmlElement * m_element = nullptr;
};

// ----------------------------------------------------------------------------

class MapObjectRetrieval {
public:
    static const MapObjectRetrieval & null_instance() {
        class Impl final : public MapObjectRetrieval {
            const MapObject * seek_object_by_id(int) const final
                { return nullptr; }

            const MapObjectGroup * seek_group_by_id(int) const final
                { return nullptr; }
        };
        static Impl impl;
        return impl;
    }

    virtual ~MapObjectRetrieval() {}

    virtual const MapObject * seek_object_by_id(int) const = 0;

    virtual const MapObjectGroup * seek_group_by_id(int) const = 0;
};

class MapObjectGroup;

class MapObject final {
public:
    using GroupContainer = std::vector<MapObjectGroup>;
    using GroupConstIterator = GroupContainer::const_iterator;

    enum class FieldType { attribute, property };

    struct CStringHasher final {
        std::size_t operator () (const char *) const;
    };

    struct CStringEqual final {
        bool operator () (const char *, const char *) const;
    };

    struct NameLessThan final {
        bool operator () (const MapObject *, const MapObject *) const;
    };

    struct Key final {
        Key() {}

        Key(const char * name_, FieldType type_):
            name(name_), type(type_) {}

        const char * name = "";
        FieldType type = FieldType::attribute;
    };

    using NameObjectMap = cul::HashMap<const char *, const MapObject *, CStringHasher, CStringEqual>;

    static NameObjectMap find_first_visible_named_objects
        (const std::vector<MapObject> &);

    static MapObject load_from
        (const TiXmlElement &,
         const MapObjectGroup & parent_group,
         const MapObjectRetrieval &);

    /// @return objects in BFS group order
    static std::vector<MapObject> load_objects_from
        (const MapObjectRetrieval & retrieval,
         View<GroupConstIterator>);

    MapObject() {}

    template <typename T>
    std::enable_if_t<std::is_arithmetic_v<T>, Optional<T>>
        get_numeric_property(const char * name) const
        { return get_arithmetic<T>(FieldType::property, name); }

    template <typename T>
    std::enable_if_t<std::is_arithmetic_v<T>, Optional<T>>
        get_numeric_attribute(const char * name) const
        { return get_arithmetic<T>(FieldType::attribute, name); }

    const MapObjectGroup * get_group_property(const char * name) const;

    const MapObject * get_object_property(const char * name) const;

    const char * get_string_property(const char * name) const;

    const char * get_string_attribute(const char * name) const;

    const char * name() const;

    int id() const;

    const MapObjectGroup * parent_group() const { return m_parent_group; }

    const MapObject * seek_by_object_name(const char * object_name) const;

private:
    struct KeyHasher final {
        std::size_t operator () (const Key & key) const;
    };

    struct KeyEqual final {
        bool operator () (const Key &, const Key &) const;
    };

    using ValuesMap = cul::HashMap<Key, const char *, KeyHasher, KeyEqual>;

    static int verify_has_id(Optional<int> maybe_id);

    MapObject(const MapObjectGroup & parent_group,
              ValuesMap &&,
              const MapObjectRetrieval &);

    const char * get_string(FieldType, const char * name) const;

    template <typename T>
    std::enable_if_t<std::is_arithmetic_v<T>, Optional<T>>
        get_arithmetic(FieldType type, const char * name) const;

    const MapObjectGroup * m_parent_group = nullptr;
    ValuesMap m_values = ValuesMap{Key{}};
    const MapObjectRetrieval * m_parent_retrieval = nullptr;
};

// ----------------------------------------------------------------------------

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, Optional<T>>
    MapObject::get_arithmetic(FieldType type, const char * name) const
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
