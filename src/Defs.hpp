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

// glm includes a boat-load of macros, but is too useful to ignore
// physical analog:
// facing north, using classic ltr x-y plane
// therefore:
// y+ is up
// y- is down
// x+ is east
// x- is west
// z- is south
// z+ is north
// the translation Vector(1, 0, 0) is a 1 "meter" displacement from the origin

#include <SFML/System/Vector2.hpp>

#include <common/Grid.hpp>
#include <common/VectorUtils.hpp>
#include <common/Vector2.hpp>
#include <common/Vector3.hpp>
#include <common/sf/VectorTraits.hpp>
#include <SFML/System/Vector3.hpp>

// wipe glm stuff...
#if 0
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <glm/mat4x4.hpp>

#include <glm/geometric.hpp>

#include <glm/gtx/projection.hpp>
#endif

#include <ariajanke/ecs3/AvlTreeEntity.hpp>
#include <ariajanke/ecs3/Scene.hpp>

#include <variant>

using Entity    = ecs::AvlTreeEntity;
using Scene     = ecs::SceneOf<Entity>;
using EntityRef = ecs::EntityRef;

#include <memory>
#include <vector>

#include <iosfwd>

// switching to SFML's vector library
// too many implicit conversion issues
using Real      = double;
using Vector    = cul::Vector3<double>;
using Vector2   = cul::Vector2<double>;
using Vector2I  = cul::Vector2<int>;
#if 0
using Matrix4x4 = glm::mat<4, 4, Real, glm::highp>;

template <typename T>
T identity_matrix();
#endif
using cul::normalize;
using cul::magnitude;
using cul::angle_between;
using cul::convert_to;
using cul::project_onto;
using cul::project_onto_plane;

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
#if 0
// do I really want matricies here?
template <>
inline Matrix4x4 identity_matrix() {
    Matrix4x4 rv;
    rv[0][0] = 1; rv[0][1] = 0; rv[0][2] = 0; rv[0][3] = 0;
    rv[1][0] = 0; rv[1][1] = 1; rv[1][2] = 0; rv[1][3] = 0;
    rv[2][0] = 0; rv[2][1] = 0; rv[2][2] = 1; rv[2][3] = 0;
    rv[3][0] = 0; rv[3][1] = 0; rv[3][2] = 0; rv[3][3] = 1;
    return rv;
}

template <>
inline glm::mat4 identity_matrix()
    { return glm::mat4(identity_matrix<Matrix4x4>()); }
#endif
#if 0
inline void * pointer_offset(unsigned x) {
    unsigned char * origin = nullptr;
    return origin + x;
}
#endif

class TriangleSegment;

Vector rotate_around_up(Vector, Real);

// I may have used these incorrectly!
// the "dir" parameter does not correspond to how the standard defines the function
Vector next_after(Vector r, Vector dir);

Vector2 next_after(Vector2 r, Vector2 dir);

// therefore purpose the following for that purpose:

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
#if 0
[[deprecated]] Vector2 find_closest_point_to_line(Vector2, Vector2, Vector2 external_point);

[[deprecated]] bool is_exact_same_value(Real, Real);
#endif
bool are_very_close(Vector, Vector);

bool are_very_close(Vector2, Vector2);

bool are_very_close(Real, Real);

constexpr const Real k_pi = cul::k_pi_for_type<Real>;
// infinity is not a real number...
constexpr const Real k_inf = std::numeric_limits<Real>::infinity();
constexpr const Vector k_up{0, 1, 0};

template <typename ... Types>
using Tuple = std::tuple<Types...>;

using cul::Grid;

std::ostream & operator << (std::ostream & out, const Vector & r);

std::ostream & operator << (std::ostream & out, const Vector2 & r);

class ShaderProgram;

class Renderable {
public:
    virtual ~Renderable();

    virtual void render(const ShaderProgram &) const = 0;
};

namespace point_and_plane {
    class TriangleLinks;
}

struct BadBranchException final : public std::runtime_error {
    BadBranchException(int line, const char * file):
        std::runtime_error(  "Bad \"impossible\" branch hit at: "
                           + std::string{file} + " line "
                           + std::to_string(line))
    {}
};

void print_links(std::ostream &, const point_and_plane::TriangleLinks &);

template <typename FullType>
struct VectorLike {
    using LikeBase = VectorLike<FullType>;

    VectorLike() {}

    VectorLike(Real x_, Real y_, Real z_):
        value(x_, y_, z_) {}

    explicit VectorLike(const Vector & r): value(r) {}

    Vector & operator = (const Vector & r)
        { return (value = r); }

    Vector value;
};

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

struct Translation final : public VectorLike<Translation> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
};

struct YRotation final : public ScalarLike<YRotation> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
};

struct TextureTranslation final : public Vector2Like<TextureTranslation> {
    using LikeBase::LikeBase;
};

class Texture;
class RenderModel;

template <typename Head, typename ... Types>
Tuple<Head, Types...> validate_graphical_components
    (Head && obj, Types && ... args)
{
    using AcceptedTypes = cul::TypeList<SharedCPtr<Texture>, SharedCPtr<RenderModel>>;
    static_assert(AcceptedTypes::kt_occurance_count<Head>,
        "Type not contained in accepted graphical types.");
    using std::make_tuple, std::tuple_cat;
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
