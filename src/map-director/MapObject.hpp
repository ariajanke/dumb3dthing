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

#include "MapElementValuesMap.hpp"
#include "../Definitions.hpp"
#include "ParseHelpers.hpp"
#include "ScaleComputation.hpp"

#include <ariajanke/cul/HashMap.hpp>
#include <ariajanke/cul/StringUtil.hpp>
#include <ariajanke/cul/Either.hpp>

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

struct MapObjectFramingLoadFailure final {};

// ----------------------------------------------------------------------------

template <Real Vector::*>
class MapObjectVectorMemberFraming final {
public:
    using LoadFailed = MapObjectFramingLoadFailure;
    using FieldType = MapElementValuesMap::FieldType;

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

class MapObjectVectorFraming final {
public:
    using LoadFailed = MapObjectFramingLoadFailure;
    using FieldType = MapElementValuesMap::FieldType;

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

class MapObjectFraming final {
public:
    template <Real Vector::* kt_member_pointer>
    using VectorMemberFraming = MapObjectVectorMemberFraming<kt_member_pointer>;
    using LoadFailed = MapObjectFramingLoadFailure;
    using FieldType = MapElementValuesMap::FieldType;

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

class MapObject final : public MapElementValuesAggregable {
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
        MapElementValuesMap::EnableOptionalNumeric<T>;
    using CStringHasher = MapElementValuesMap::CStringHasher;
    using CStringEqual = MapElementValuesMap::CStringEqual;
    using FieldType = MapElementValuesMap::FieldType;
    using NameObjectMap = cul::HashMap<const char *, const MapObject *, CStringHasher, CStringEqual>;
    struct NameLessThan final {
        bool operator () (const MapObject *, const MapObject *) const;
    };

    static constexpr const auto k_name_attribute = MapElementValuesMap::k_name_attribute;
    static constexpr const auto k_id_attribute = "id";

    static NameObjectMap find_first_visible_named_objects
        (const MapObjectContainer &);

    /// @return objects in BFS group order
    static MapObjectContainer load_objects_from
        (View<GroupConstIterator> groups,
         View<XmlElementConstIterator> elements);

    static MapObject load_from
        (const TiXmlElement &,
         const MapObjectGroup & parent_group);

    MapObject() {}
#   if 0
    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric(FieldType type, const char * name) const
        { return m_values_map.get_numeric<T>(type, name); }

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric_property(const char * name) const
        { return m_values_map.get_numeric_property<T>(name); }

    template <typename T>
    EnableOptionalNumeric<T>
        get_numeric_attribute(const char * name) const
        { return m_values_map.get_numeric_attribute<T>(name); }
#   endif
    const MapObjectGroup * get_group_property(const char * name) const;

    const MapObject * get_object_property(const char * name) const;

    View<MapObjectRefConstIterator> get_referrers() const
        { return m_parent_retrieval->seek_referrers_by_id(id()); }
#   if 0
    const char * get_string_property(const char * name) const;

    const char * get_string_attribute(const char * name) const;
#   endif
    const char * name() const;

    int id() const;

    const MapObjectGroup * parent_group() const { return m_parent_group; }

    const MapObject * seek_by_object_name(const char * object_name) const;

    void set_by_id_retrieval(const MapObjectRetrieval & retrieval)
        { m_parent_retrieval = &retrieval; }

private:
    static int verify_has_id(Optional<int> maybe_id);

    MapObject(const MapObjectGroup & parent_group,
              MapElementValuesMap && values):
             MapElementValuesAggregable(std::move(values)),
             m_parent_group(&parent_group) {}

    const MapObjectGroup * m_parent_group = nullptr;
#   if 0
    MapElementValuesMap m_values_map;
#   endif
    const MapObjectRetrieval * m_parent_retrieval = nullptr;
};

// ----------------------------------------------------------------------------

template <Real Vector::* kt_member_pointer>
Either<MapObjectFramingLoadFailure, Vector>
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
