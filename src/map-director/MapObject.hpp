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
#include "ScaleComputation.hpp"

#include <ariajanke/cul/HashMap.hpp>
#include <ariajanke/cul/StringUtil.hpp>
#include <ariajanke/cul/Either.hpp>

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
    using MapObjectRefContainer = std::vector<const MapObject *>;
    using MapObjectRefConstIterator = MapObjectRefContainer::const_iterator;

    virtual ~MapObjectRetrieval() {}

    virtual const MapObject * seek_object_by_id(int) const = 0;

    virtual const MapObjectGroup * seek_group_by_id(int) const = 0;

    virtual View<MapObjectRefConstIterator> seek_referrers_by_id(int) const = 0;
};

// ----------------------------------------------------------------------------

class MapObjectBase {
public:
    enum class FieldType { attribute, property };

    struct LoadFailed final {};

protected:
    constexpr MapObjectBase() {}
};

// ----------------------------------------------------------------------------

template <Real Vector::*>
class MapObjectVectorMemberFraming final : public MapObjectBase {
public:
    constexpr MapObjectVectorMemberFraming
        (FieldType field_type, const char * name, bool required):
        m_field_type(field_type), m_name(name), m_required(required) {}

    Either<LoadFailed, Vector> operator ()
        (Either<LoadFailed, Vector> &&, const MapObject & object) const;

private:
    FieldType m_field_type;
    const char * m_name;
    bool m_required;
};

// ----------------------------------------------------------------------------

class MapObjectVectorFraming final : public MapObjectBase {
public:
    template <Real Vector::* kt_member_pointer>
    using MemberFraming = MapObjectVectorMemberFraming<kt_member_pointer>;

    constexpr MapObjectVectorFraming
        (const MemberFraming<&Vector::x> & x_framing,
         const MemberFraming<&Vector::y> & y_framing,
         const MemberFraming<&Vector::z> & z_framing):
        m_x_framing(x_framing),
        m_y_framing(y_framing),
        m_z_framing(z_framing) {}

    Either<LoadFailed, Vector> operator () (const MapObject & object) const {
        return m_z_framing(m_y_framing(m_x_framing(Vector{}, object), object), object);
    }

private:
    MemberFraming<&Vector::x> m_x_framing;
    MemberFraming<&Vector::y> m_y_framing;
    MemberFraming<&Vector::z> m_z_framing;
};

// ----------------------------------------------------------------------------

class MapObjectFraming final : public MapObjectBase {
public:
    template <Real Vector::* kt_member_pointer>
    using VectorMemberFraming = MapObjectVectorMemberFraming<kt_member_pointer>;

    using VectorXFraming = VectorMemberFraming<&Vector::x>;
    using VectorYFraming = VectorMemberFraming<&Vector::y>;
    using VectorZFraming = VectorMemberFraming<&Vector::z>;

    static constexpr const auto k_point_object_framing =
        MapObjectVectorFraming
            {VectorXFraming{FieldType::attribute, "x"        , true },
             VectorYFraming{FieldType::property , "elevation", false},
             VectorZFraming{FieldType::attribute, "y"        , true }};

    static MapObjectFraming load_from(const TiXmlElement & map_element);

    MapObjectFraming() {}

    explicit MapObjectFraming(const ScaleComputation & scale):
        m_map_pixel_scale(scale) {}

    Either<LoadFailed, Vector> get_position_from
        (const MapObject & object,
         const MapObjectVectorFraming & framing = k_point_object_framing) const
    {
        return framing(object).map([this] (Vector && r) {
            return m_map_pixel_scale.of(r);
        });
    }

private:
    ScaleComputation m_map_pixel_scale;
};

// ----------------------------------------------------------------------------

class MapObject final : public MapObjectBase {
public:
    using GroupContainer = std::vector<MapObjectGroup>;
    using GroupConstIterator = GroupContainer::const_iterator;
    using MapObjectRefContainer = MapObjectRetrieval::MapObjectRefContainer;
    using MapObjectRefConstIterator =
        MapObjectRetrieval::MapObjectRefConstIterator;
    using XmlElementContainer = std::vector<const TiXmlElement *>;
    using XmlElementConstIterator = XmlElementContainer::const_iterator;
    using MapObjectContainer = std::vector<MapObject>;
    template <typename T>
    using EnableOptionalNumeric =
        std::enable_if_t<std::is_arithmetic_v<T>, Optional<T>>;

    struct CStringHasher final {
        std::size_t operator () (const char *) const;
    };

    struct CStringEqual final {
        bool operator () (const char *, const char *) const;
    };

    using NameObjectMap = cul::HashMap<const char *, const MapObject *, CStringHasher, CStringEqual>;

    struct NameLessThan final {
        bool operator () (const MapObject *, const MapObject *) const;
    };

    static NameObjectMap find_first_visible_named_objects
        (const MapObjectContainer &);

    /// @return objects in BFS group order
    static MapObjectContainer load_objects_from
        (View<GroupConstIterator> groups,
         View<XmlElementConstIterator> elements);

    static MapObject load_from
        (const TiXmlElement &,
         const MapObjectGroup & parent_group);

    static constexpr const auto k_properties_tag = "properties";
    static constexpr const auto k_property_tag = "property";
    static constexpr const auto k_name_attribute = "name";
    static constexpr const auto k_value_attribute = "value";
    static constexpr const auto k_id_attribute = "id";

    MapObject() {}

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric(FieldType type, const char * name) const;

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric_property(const char * name) const
        { return get_numeric<T>(FieldType::property, name); }

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric_attribute(const char * name) const
        { return get_numeric<T>(FieldType::attribute, name); }

    const MapObjectGroup * get_group_property(const char * name) const;

    const MapObject * get_object_property(const char * name) const;

    View<MapObjectRefConstIterator> get_referrers() const
        { return m_parent_retrieval->seek_referrers_by_id(id()); }

    const char * get_string_property(const char * name) const;

    const char * get_string_attribute(const char * name) const;

    const char * name() const;

    int id() const;

    const MapObjectGroup * parent_group() const { return m_parent_group; }

    const MapObject * seek_by_object_name(const char * object_name) const;

    void set_by_id_retrieval(const MapObjectRetrieval & retrieval)
        { m_parent_retrieval = &retrieval; }

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

    static int verify_has_id(Optional<int> maybe_id);

    MapObject(const MapObjectGroup & parent_group,
              ValuesMap && values):
             m_parent_group(&parent_group),
             m_values(std::move(values)) {}

    const char * get_string(FieldType, const char * name) const;

    const MapObjectGroup * m_parent_group = nullptr;
    ValuesMap m_values = ValuesMap{Key{}};
    const MapObjectRetrieval * m_parent_retrieval = nullptr;
};

// ----------------------------------------------------------------------------

template <Real Vector::* kt_member_pointer>
Either<MapObjectBase::LoadFailed, Vector>
    MapObjectVectorMemberFraming<kt_member_pointer>::operator ()
    (Either<LoadFailed, Vector> && ei, const MapObject & object) const
{
    return ei.chain([&object, this] (Vector && r) -> Either<LoadFailed, Vector> {
        if (auto num = object.get_numeric<Real>(m_field_type, m_name)) {
            auto & member = (r.*kt_member_pointer) = *num;
            if constexpr (kt_member_pointer == &Vector::z) {
                member *= -1;
                member += k_tile_top_left.z;
            }
            if constexpr (kt_member_pointer == &Vector::x) {
                member += k_tile_top_left.x;
            }
        } else if (m_required) {
            return LoadFailed{};
        }
        return std::move(r);
    });
}

// ----------------------------------------------------------------------------

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, Optional<T>>
    MapObject::get_numeric(FieldType type, const char * name) const
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
