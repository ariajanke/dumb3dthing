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

template <typename T>
class MapObjectConversion final {
public:
    static Optional<T> convert(const char *) { return {}; }
};

class MapObject;

class MapObjectRetrieval {
public:
    static const MapObjectRetrieval & null_instance() {
        class Impl final : public MapObjectRetrieval {
            const MapObjectRetrieval * seek_group(const char *) const { return nullptr; }

            const MapObject * seek_by_name(const char *) const { return nullptr; }

            const MapObject * seek_object_by_id(int) const { return nullptr; }
        };
        static Impl impl;
        return impl;
    }

    virtual ~MapObjectRetrieval() {}

    virtual const MapObjectRetrieval * seek_group(const char *) const = 0;

    // current group -> down too lowest
    // fall back onto first visible "global"
    //
    // what if two names in two different sub-groups
    // or even the same group? Need to rethink disambiguation...
    virtual const MapObject * seek_by_name(const char *) const = 0;

    virtual const MapObject * seek_object_by_id(int) const = 0;
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

    // objects also in BFS group order
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

    const MapObject * get_object_property(const char * name) const;

    const MapObjectGroup * get_group_property(const char * name) const;

    const char * get_string_property(const char * name) const
        { return get_string(FieldType::property, name); }

    const char * get_string_attribute(const char * name) const
        { return get_string(FieldType::attribute, name); }

    // TilEd always requires this
    const char * name() const;

    const MapObjectGroup * parent_group() const
        { return m_parent_group; }

    int id() const {
        return verify_has_id(get_numeric_attribute<int>("id"));
    }

    const MapObject * seek_by_name(const char * object_name) const;

private:
    struct KeyHasher final {
        std::size_t operator () (const Key & key) const;
    };

    struct KeyEqual final {
        bool operator () (const Key &, const Key &) const;
    };

    using ValuesMap = cul::HashMap<Key, const char *, KeyHasher, KeyEqual>;

    static int verify_has_id(Optional<int> maybe_id) {
        if (maybe_id) return *maybe_id;
        throw RuntimeError{"objects are expect to always have ids"};
    }

    MapObject(const MapObjectGroup & parent_group, ValuesMap && values, const MapObjectRetrieval & retrieval):
        m_parent_group(&parent_group),
        m_values(std::move(values)),
        m_parent_retrieval(&retrieval) {}

    const char * get_string(FieldType type, const char * name) const {
        auto itr = m_values.find(Key{name, type});
        if (itr == m_values.end()) return nullptr;
        return itr->second;
    }

    template <typename T>
    std::enable_if_t<std::is_arithmetic_v<T>, Optional<T>>
        get_arithmetic(FieldType type, const char * name) const
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


    const MapObjectGroup * m_parent_group = nullptr;
    ValuesMap m_values = ValuesMap{Key{}};
    const MapObjectRetrieval * m_parent_retrieval = nullptr;
};
#if 0
// ----------------------------------------------------------------------------

class MapObjectGroupBase {
public:
    using GroupContainer = MapObject::GroupContainer;
    using Iterator = GroupContainer::iterator;
    using NameObjectMap = MapObject::NameObjectMap;
    using ObjectContainer = std::vector<MapObject>;
    using ObjectIterator = ObjectContainer::const_iterator;

    static constexpr const std::array k_group_tags =
        { "objectgroup", "group" };

protected:
    // names, ranks, returned in BFS order
    static GroupContainer _initialize_names_and_parents_for_map
        (const DocumentOwningNode & map_element);

    // parent, groups
    static GroupContainer _set_groups_and_ranks_for(GroupContainer &&);

    static Optional<MapObjectGroup> _initialize_from_element
        (const TiXmlElement &, int rank);

private:
    static void emplace_group_children
        (GroupContainer &,
         const TiXmlElement & any_element,
         int current_rank);

    static void emplace_groups
        (GroupContainer &,
         const TiXmlElement & any_element,
         int current_rank);

    static void set_groups_and_ranks_for
        (View<Iterator> groups, int current_rank = 0);
};

// ----------------------------------------------------------------------------

class MapObjectGroupForTests final : public MapObjectGroupBase {
public:
    static GroupContainer initialize_names_and_parents_for_map
        (const DocumentOwningNode & map_element)
        { return _initialize_names_and_parents_for_map(map_element); }

    static GroupContainer set_groups_and_ranks_for
        (GroupContainer && groups)
        { return _set_groups_and_ranks_for(std::move(groups)); }

    static Optional<MapObjectGroup> initialize_from_element
        (const TiXmlElement & el, int rank);
};

// ----------------------------------------------------------------------------

class MapObjectGroup final : public MapObjectGroupBase {
public:
    using ConstIterator = GroupContainer::const_iterator;
    using ObjectReorderContainer = std::vector<const MapObject *>;
    using ObjectNamesIterator = ObjectReorderContainer::const_iterator;

    static GroupContainer initialize_for_map
        (const DocumentOwningNode & map_element);

    // ordered by (group, name), assigned to groups
    static std::vector<MapObject> assign_groups_objects
        (const NameObjectMap & globally_visible_named_objects,
         std::vector<MapObject> && all_objects,
         View<Iterator> all_groups);

    // MapObject::NameLessThan should be built on me
    static bool find_name_predicate
        (const MapObject * obj, const char * object_name)
    { return ::strcmp(obj->name(), object_name) < 0; }

    explicit MapObjectGroup(int id): m_id(id) {}

    MapObjectGroup
        (const TiXmlElement & element, const char * name, int id, int rank):
        m_name(name), m_element(&element), m_id(id), m_rank(rank) {}

    void set_parent(const MapObjectGroup & group)
        { m_parent = &group; }

    int rank() const { return m_rank; }

    const char * name() const { return m_name; }

    // at some point, add behavior

    std::vector<MapObject> load_child_objects
        (const MapObjectRetrieval &,
         std::vector<MapObject> &&) const;

    void set_child_objects(View<ObjectIterator> child_objects)
        { m_objects = child_objects; }

    View<ObjectIterator> objects() const { return m_objects; }

    View<ConstIterator> groups() const { return m_groups; }

    const MapObject * seek_by_name(const char * object_name) const {
        auto itr = m_object_name_map.find(object_name);
        if (itr != m_object_name_map.end()) {
            return itr->second;
        } else if (m_parent) {
            return m_parent->seek_by_name(object_name);
        }
        return nullptr;
    }

    int id() const { return m_id; }

    Iterator set_child_groups(Iterator starting_at, Iterator end);

    void set_object_name_map(NameObjectMap && name_map)
        { m_object_name_map = std::move(name_map); }

    bool has_parent() const { return static_cast<bool>(m_parent); }

private:
    static const View<ConstIterator> k_empty_group_view;
    static const View<ObjectNamesIterator> k_empty_object_reorder_view;
    static const View<ObjectIterator> k_empty_object_view;

    const MapObjectGroup * m_parent = nullptr;
    const char * m_name = "";
    // v probably shouldn't, or at least null out later
    const TiXmlElement * m_element = nullptr;
    int m_id = 0;
    int m_rank = 0;
    View<ConstIterator> m_groups = k_empty_group_view;
    View<ObjectIterator> m_objects = k_empty_object_view;
    NameObjectMap m_object_name_map{nullptr};
};

struct MapObjectGroupAssignment final {
    const MapObjectGroup * group = nullptr;
    const MapObject * object = nullptr;
};

// heap ordered groups and then
// hierarchical ordered groups (not possible, though won't a heap work?)

// name lookup
// find range by name

class MapObjectCollection final : public MapObjectRetrieval {
public:
    using GroupContainer = MapObject::GroupContainer;
    using ObjectReorderContainer = MapObjectGroup::ObjectReorderContainer;

    static MapObjectCollection load_from(const DocumentOwningNode & map_element) {
        MapObjectCollection collection;
        collection.load(map_element);
        return collection;
    }

    MapObjectCollection() {}

    void load(const DocumentOwningNode & map_element);

    const MapObjectRetrieval * seek_group(const char *) const
        { return nullptr; }

    const MapObject * seek_by_name(const char *) const { return nullptr; }

    const MapObject * seek_object_by_id(int id) const {
        auto itr = m_id_to_object.find(id);
        if (itr == m_id_to_object.end())
            { return nullptr; }
        return itr->second;
    }

    const MapObjectGroup * top_level_group() const {
        if (m_groups.empty())
            { return nullptr; }
        return &m_groups.front();
    }

private:
    cul::HashMap<int, const MapObject *> m_id_to_object{0};
    std::vector<MapObject> m_map_objects;
    GroupContainer m_groups;
};

template <>
class MapObjectConversion<const char *> final {
public:
    static Optional<const char *> convert(const char * str)
        { return str; }
};

template <>
class MapObjectConversion<int> final {
public:
    static Optional<int> convert(const char * str) {
        int temp;
        if (cul::string_to_number(str, str + ::strlen(str), temp))
            return temp;
        return {};
    }
};

// I forgor
template <>
class MapObjectConversion<Real> final {
public:
    static Optional<Real> convert(const char * str) {
        Real temp;
        if (cul::string_to_number(str, str + ::strlen(str), temp))
            return temp;
        return {};
    }
};
#endif
