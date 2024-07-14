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

#include "Definitions.hpp"
#include "platform.hpp"

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

    Vector & operator += (const Vector & r)
        { return (value += r); }

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

template <typename T>
bool are_very_close(const VectorLike<T> & lhs, const VectorLike<T> & rhs)
    { return are_very_close(lhs.value, rhs.value); }

// --------------------------- Graphical Components ---------------------------

// What defines a graphical component: it is used by all platforms in rendering
// entities.

struct ModelTranslation final : public VectorLike<ModelTranslation> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
    using LikeBase::operator+=;
};

struct PlayerRecovery final : public VectorLike<PlayerRecovery> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
    using LikeBase::operator+=;
};

struct YRotation final : public ScalarLike<YRotation> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
};

struct XRotation final : public ScalarLike<XRotation> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
};

struct TextureTranslation final : public Vector2Like<TextureTranslation> {
    using LikeBase::LikeBase;
};

struct Camera final {
    Vector position, target, up = k_up;
};

struct ModelVisibility final {
    bool & operator = (bool b) { return (value = b); }

    explicit operator bool () const noexcept
        { return value; }

    bool value = true;
};

struct ModelScale final : public VectorLike<ModelScale> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
    using LikeBase::operator+=;
};

inline bool should_be_visible(const EcsOpt<ModelVisibility> & vis)
    { return vis ? vis->value : true; }

// ----------------------------- Other Components -----------------------------

struct Velocity final : public VectorLike<Velocity> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
};

// Even though this is a "graphical" component, it is not included with that
// group. The driver is in no way expected to read this component.
struct VisibilityChain final {
    static constexpr const Real k_to_next = 1.2;
    ecs::EntityRef next;
    Real time_spent = 0;
    bool visible = true;
};

struct TranslationFromParent final {
    TranslationFromParent() {}
    TranslationFromParent(EntityRef par_, Vector trans_):
        parent(par_), translation(trans_) {}
    EntityRef parent;
    Vector translation;
};

struct DragCamera final {
    Vector position = Vector{100, 100, 100};
    Real max_distance = 6;
};

struct JumpVelocity final : public VectorLike<JumpVelocity> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
};
