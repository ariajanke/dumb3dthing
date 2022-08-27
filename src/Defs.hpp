/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include <common/Grid.hpp>
#include <common/VectorUtils.hpp>
#include <common/Vector3.hpp>

#include <ariajanke/ecs3/AvlTreeEntity.hpp>
#include <ariajanke/ecs3/Scene.hpp>
#include <ariajanke/ecs3/SingleSystem.hpp>

#include <variant>
#include <memory>
#include <iosfwd>

// ----------------------------- Type Definitions -----------------------------

using Entity    = ecs::AvlTreeEntity;
using Scene     = ecs::SceneOf<Entity>;
using EntityRef = ecs::EntityRef;

using Real      = double;
using Vector    = cul::Vector3<double>;
using Vector2   = cul::Vector2<double>;
using Vector2I  = cul::Vector2<int>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T>
using SharedCPtr = std::shared_ptr<const T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

template <typename T>
using WeakCPtr = std::weak_ptr<const T>;

template <typename T, typename Del = std::default_delete<T>>
using UniquePtr = std::unique_ptr<T, Del>;

template <typename ... Types>
using Variant = std::variant<Types...>;

template <typename ... Types>
using Tuple = std::tuple<Types...>;

template <typename T>
using Opt = ecs::Optional<T>;

using cul::Grid;

class TriangleSegment;
class Texture;
class RenderModel;

class ShaderProgram;
namespace point_and_plane {
    class TriangleLinks;
}

class BadBranchException final : public std::runtime_error {
public:
    BadBranchException(int line, const char * file);
};

enum class KeyControl {
    forward,
    backward,
    left,
    right,
    jump,

    pause,
    advance,
    print_info,
    restart
};

// ---------------------------- Component Helpers -----------------------------

template <typename FullType>
struct VectorLike {
    using LikeBase = VectorLike<FullType>;

    VectorLike() {}

    VectorLike(Real x_, Real y_, Real z_):
        value(x_, y_, z_) {}

    explicit VectorLike(const Vector & r): value(r) {}

    Vector & operator = (const Vector & r)
        { return (value = r); }

    Vector operator * (Real scalar) const noexcept
        { return value*scalar; }


    Vector value;
};

template <typename T>
Vector operator + (const VectorLike<T> & v, const Vector & r) noexcept
    { return v.value + r; }

template <typename T>
Vector operator + (const Vector & r, const VectorLike<T> & v) noexcept
    { return v.value + r; }

template <typename FullType>
struct Vector2Like {
    using LikeBase = Vector2Like<FullType>;

    Vector2Like() {}

    Vector2Like(Real x_, Real y_):
        value(x_, y_) {}

    explicit Vector2Like(const Vector2 & r): value(r) {}

    Vector2 & operator = (const Vector2 & r)
        { return (value = r); }

    Vector2 value;
};

template <typename FullType>
struct ScalarLike {
    using LikeBase = ScalarLike<FullType>;
    ScalarLike() {}

    explicit ScalarLike(Real x): value(x) {}

    Real & operator = (Real x)
        { return (value = x); }

    Real value = 0;
};

// --------------------------- Everywhere Functions ---------------------------

using cul::normalize;
using cul::magnitude;
using cul::angle_between;
using cul::convert_to;
using cul::project_onto;
using cul::project_onto_plane;
using std::make_shared;
using std::get_if;
// using std::get; <- too short and contextually vague for comfort
using std::make_tuple;
using std::make_unique;

[[deprecated]] Vector rotate_around_up(Vector, Real);

std::ostream & operator << (std::ostream & out, const Vector & r);

std::ostream & operator << (std::ostream & out, const Vector2 & r);

bool are_very_close(Vector, Vector);

bool are_very_close(Vector2, Vector2);

bool are_very_close(Real, Real);

void print_links(std::ostream &, const point_and_plane::TriangleLinks &);

/** Gets the "nextafter" vector following a given direction
 *  @param r starting position
 *  @param dir direction to go in
 *  @return vector with the smallest possible difference from r in the
 *          direction of dir
 *  @note gotcha here with TriangleSurface::point_at it may not necessarily be
 *        the case that for some given TriangleSurface ts, Vector r, Vector dir
 *        that:
 *        (ts.point_at(r) - ts.point_at(next_in_direction(r, dir) == Vector(0, 0, 0))
 */
Vector next_in_direction(Vector r, Vector dir);

/** @copydoc next_in_direction(Vector,Vector) */
Vector2 next_in_direction(Vector2 r, Vector2 dir);

// ----------------------------- Global Constants -----------------------------

constexpr const Real k_pi  = cul::k_pi_for_type<Real>;
constexpr const Real k_inf = std::numeric_limits<Real>::infinity();

// facing north, using classic ltr x-y plane
// y+ is up   , y- is down
// x+ is east , x- is west
// z- is south, z+ is north

constexpr const Vector k_east {1, 0, 0};
constexpr const Vector k_up   {0, 1, 0};
constexpr const Vector k_north{0, 0, 1};

// ------------------------------ prototype stuff -----------------------------

template <typename Head, typename ... Types>
Tuple<Head, Types...> validate_graphical_components
    (Head && obj, Types && ... args)
{
    using AcceptedTypes = cul::TypeList<SharedCPtr<Texture>, SharedCPtr<RenderModel>>;
    static_assert(AcceptedTypes::kt_occurance_count<Head>,
        "Type not contained in accepted graphical types.");
    using std::tuple_cat;
    if constexpr (sizeof...(Types) > 0) {
        return tuple_cat(make_tuple(std::move( obj )),
                         validate_graphical_components(std::forward<Types>(args)...));
    } else {
        return make_tuple( std::move(obj) );
    }
}

template <typename ... Types>
void add_components(Entity & e, Tuple<Types...> && tup) {
    if constexpr (sizeof...(Types) > 1) {
        e.add<Types...>() = std::move(tup);
    } else if constexpr (sizeof...(Types) == 1) {
        using Head = typename cul::TypeList<Types...>::template TypeAtIndex<0>;
        e.add<Head>() = std::get<Head>(tup);
    }
}
