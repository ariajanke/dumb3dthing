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

#include <ariajanke/cul/Grid.hpp>
#include <ariajanke/cul/VectorUtils.hpp>
#include <ariajanke/cul/Vector3.hpp>
#include <ariajanke/cul/OptionalEither.hpp>
#include <ariajanke/ecs3/AvlTreeEntity.hpp>
#include <ariajanke/ecs3/HashTableEntity.hpp>
#include <ariajanke/ecs3/Scene.hpp>
#include <ariajanke/ecs3/SingleSystem.hpp>

#include <variant>
#include <memory>
#include <iosfwd>
#include <optional>

// ----------------------------- Type Definitions -----------------------------

#ifndef MACRO_SCALAR_TYPE_FOR_VECTORS
using Real      = double;
#else
using Real      = MACRO_SCALAR_TYPE_FOR_VECTORS;
#endif

using Entity    = ecs::AvlTreeEntity;
using Scene     = ecs::SceneOf<Entity>;
using EntityRef = ecs::EntityRef;

using Vector    = cul::Vector3<Real>;
using Vector2   = cul::Vector2<Real>;
using Vector2I  = cul::Vector2<int>;

using Size2  = cul::Size2<Real>;
using Size2I = cul::Size2<int>;

using RectangleI = cul::Rectangle<int>;

template <typename T>
using SharedPtr = std::shared_ptr<T>;
template <typename T>
using WeakPtr = std::weak_ptr<T>;
template <typename T, typename Del = std::default_delete<T>>
using UniquePtr = std::unique_ptr<T, Del>;

template <typename ... Types>
using Variant = std::variant<Types...>;
template <typename ... Types>
using Tuple = std::tuple<Types...>;
template <typename T>
using EcsOpt = ecs::Optional<T>;
template <typename T>
using Optional = std::optional<T>;
template <typename L, typename R>
using OptionalEither = cul::OptionalEither<L, R>;
template <typename L, typename R>
using Either = cul::Either<L, R>;
using cul::Grid;
using cul::View;
using cul::TypeList;

class TriangleSegment;
class Texture;
class RenderModel;

class ShaderProgram;

class TriangleLink;

class BadBranchException final : public std::runtime_error {
public:
    BadBranchException(int line, const char * file);
};

// --------------------------- Everywhere Functions ---------------------------

using cul::normalize;
using cul::magnitude;
// using cul::angle_between; -> favor internal implementation
using cul::convert_to;
using cul::project_onto;
using cul::project_onto_plane;
using std::make_shared;
using std::get_if;
using std::make_tuple;
using std::make_unique;

std::ostream & operator << (std::ostream & out, const Vector & r);

std::ostream & operator << (std::ostream & out, const Vector2 & r);

std::ostream & operator << (std::ostream & out, const TriangleSegment &);

bool are_very_close(Vector, Vector);

bool are_very_close(Vector2, Vector2);

bool are_very_close(Real, Real);

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

constexpr const bool k_promised_files_take_at_least_one_frame = false;
