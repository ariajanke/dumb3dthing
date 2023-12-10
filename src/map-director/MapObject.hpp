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
    // I should know my parent group
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

    static MapObject load_from
        (const TiXmlElement &,
         const MapObjectGroup & parent_group,
         const MapObjectRetrieval &);

    // objects also in BFS group order
    static std::vector<MapObject> load_from
        (const MapObjectRetrieval & retrieval,
         View<GroupConstIterator>);

    static NameObjectMap find_first_visible_named_objects
        (const std::vector<MapObject> &);

    MapObject() {}

    void load(const DocumentOwningNode &, const MapObjectRetrieval &);

    template <typename T>
    Optional<T> get_property(const char * name) const
        { return get<T>(FieldType::property, name); }

    template <typename T>
    Optional<T> get_attribute(const char * name) const
        { return get<T>(FieldType::attribute, name); }

    // TilEd always requires this
    Optional<const char *> name() const
        { return get_attribute<const char *>("name"); }

    const MapObjectGroup * parent_group() const
        { return m_parent_group; }

    int id() const {
        auto maybe_id = get_attribute<int>("id");
        if (!maybe_id) {
            throw RuntimeError{"object without an id"};
        }
        return *maybe_id;
    }

    const MapObject * seek_by_name(const char * object_name) const;

private:
    template <typename T>
    Optional<T> get(FieldType type, const char * name) const {
        auto itr = m_values.find(Key{name, type});
        if (itr == m_values.end()) return {};
        return MapObjectConversion<T>::convert(itr->second);
    }

    struct KeyHasher final {
        std::size_t operator () (const Key & key) const;
    };

    struct KeyEqual final {
        bool operator () (const Key &, const Key &) const;
    };

    using ValuesMap = cul::HashMap<Key, const char *, KeyHasher, KeyEqual>;

    MapObject(const MapObjectGroup & parent_group, ValuesMap && values, const MapObjectRetrieval & retrieval):
        m_parent_group(&parent_group),
        m_values(std::move(values)),
        m_parent_retrieval(&retrieval) {}

    const MapObjectGroup * m_parent_group = nullptr;
    ValuesMap m_values = ValuesMap{Key{}};
    const MapObjectRetrieval * m_parent_retrieval = nullptr;
};

class MapObjectSomething;

class MapObjectLoader final {
public:
    using ObjectIterator = std::vector<MapObject>::const_iterator;

    virtual void operator ()
        (MapObjectSomething &,
         View<ObjectIterator>
         /* and something about nested loaders? */);
};

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

    static std::vector<const MapObject *>
        _group_name_ordered_objects_for
        (const NameObjectMap & globally_visible_named_objects,
         View<ObjectIterator> all_objects,
         View<Iterator> all_groups);

    // goes unused?
    static bool _group_name_order(const MapObject *, const MapObject *);

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

    static std::vector<const MapObject *>
        group_name_ordered_objects_for
        (const NameObjectMap & globally_visible_named_objects,
         View<ObjectIterator> all_objects,
         View<Iterator> all_groups);

    static bool group_name_order(const MapObject * lhs, const MapObject * rhs)
        { return _group_name_order(lhs, rhs); }
};

// ----------------------------------------------------------------------------

class MapObjectGroup final : public MapObjectGroupBase {
public:
    using ConstIterator = GroupContainer::const_iterator;
    using ObjectReorderContainer = std::vector<const MapObject *>;
    using ObjectNamesIterator = ObjectReorderContainer::const_iterator;

    static GroupContainer initialize_for_map
        (const DocumentOwningNode & map_element);

    struct MapObjectGroupOrdering final {
        std::vector<const MapObject *> group_name_ordered_objects;
        // bfs order *should be* pointer order also
        // (if groups themselves are bfs ordered)
        std::vector<MapObject> group_ordered_objects;
    };

    // ordered by (group, name), assigned to groups
    static MapObjectGroupOrdering assign_groups_objects
        (const NameObjectMap & globally_visible_named_objects,
         std::vector<MapObject> && all_objects,
         View<Iterator> all_groups);

    // MapObject::NameLessThan should be built on me
    static bool find_name_predicate
        (const MapObject * obj, const char * object_name)
    { return ::strcmp(obj->name().value_or(""), object_name) < 0; }

    explicit MapObjectGroup(int id): m_id(id) {}

    MapObjectGroup
        (const TiXmlElement & element, const char * name, int id, int rank):
        m_name(name), m_element(&element), m_id(id), m_rank(rank) {}

    void set_parent(const MapObjectGroup & group)
        { m_parent = &group; }

    int rank() const { return m_rank; }

    Optional<const char *> name() const {
        if (m_name[0] == '\0') return {};
        return m_name;
    }

    // at some point, add behavior

    std::vector<MapObject> load_child_objects
        (const MapObjectRetrieval &,
         std::vector<MapObject> &&) const;

    void set_child_objects
        (View<ObjectNamesIterator> name_ordered_objects,
         View<ObjectIterator> child_objects)
    {
        auto any_nullptrs =
            std::any_of(name_ordered_objects.begin(), name_ordered_objects.end(),
                        [] (const MapObject * obj) { return obj == nullptr; });
        if (any_nullptrs) {
            throw InvalidArgument{"named range cannot contain any nullptrs"};
        }
        m_name_ordered_objects = name_ordered_objects;
        m_objects = child_objects;
    }

    View<ObjectIterator> objects() const { return m_objects; }

    View<ConstIterator> groups() const { return m_groups; }

    const MapObject * seek_by_name(const char * object_name) const {
        auto itr = std::lower_bound
            (m_name_ordered_objects.begin(), m_name_ordered_objects.end(),
             object_name, find_name_predicate);
        if (itr == m_name_ordered_objects.end())
            { return nullptr; }
        if (::strcmp((**itr).name().value_or(""), object_name))
            { return nullptr; }
        return *itr;
    }

    int id() const { return m_id; }

    Iterator set_child_groups(Iterator starting_at, Iterator end);

private:
    static const View<ConstIterator> k_empty_group_view;
    static const View<ObjectNamesIterator> k_empty_object_reorder_view;
    static const View<ObjectIterator> k_empty_object_view;


    static void populate_groups_with_objects
        (View<ObjectIterator> all_objects,
         View<Iterator> all_groups,
         View<ObjectNamesIterator> group_name_ordered_objects);

    // v still not yet set by anything
    const MapObjectGroup * m_parent = nullptr;
    const char * m_name = "";
    const TiXmlElement * m_element = nullptr;
    int m_id = 0;
    int m_rank = 0;
    View<ConstIterator> m_groups = k_empty_group_view;
    View<ObjectNamesIterator> m_name_ordered_objects = k_empty_object_reorder_view;
    View<ObjectIterator> m_objects = k_empty_object_view;
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

    static MapObjectCollection load_from(const DocumentOwningNode & map_element);

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
    ObjectReorderContainer m_reordered_objects;
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
