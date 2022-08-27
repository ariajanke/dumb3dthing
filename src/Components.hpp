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

#include "Defs.hpp"

// --------------------------- Graphical Components ---------------------------

// What defines a graphical component: it is used by all platforms in rendering
// entities.

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

struct Camera final {
    Vector position, target, up = k_up;
};

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

// Dare I use OOP for component types. Surely this will make purist snarl in
// disgust. I just happen to find encapsulation useful in controlling for valid
// values, and reducing complexity. I also believe it helps keeps things
// organized. Afterall, I'd levy the argument that using raw pointers in lieu
// of std::vector, would be painful, impractical even with helper free
// functions. If I were to use free functions everywhere and carefully avoid
// messing with the pointers themselves, why not a class? And further, why not
// std::vector? At which point I am now using OOP to define components.
class PlayerControl final {
public:
    void press(KeyControl ky) {
        using Kc = KeyControl;
        switch (ky) {
        case Kc::forward: case Kc::backward: case Kc::left: case Kc::right:
            m_dir[to_index(ky)] = true;
            break;
        case Kc::jump:
            m_jump_this_frame = true;
            break;
        default: return;
        }
    }

    void release(KeyControl ky) {
        using Kc = KeyControl;
        switch (ky) {
        case Kc::forward: case Kc::backward: case Kc::left: case Kc::right:
            m_dir[to_index(ky)] = false;
            break;
        case Kc::jump:
            m_jump_this_frame = false;
            break;
        default: return;
        }
    }

    void frame_update() {
        m_jump_pressed_before = m_jump_this_frame;
    }

    /** @returns either a normal or zero vector */
    Vector2 heading() const {
        using Kc = KeyControl;
        auto left  = m_dir[to_index(Kc::left    )];
        auto right = m_dir[to_index(Kc::right   )];
        auto back  = m_dir[to_index(Kc::backward)];
        auto fore  = m_dir[to_index(Kc::forward )];
        bool x_ways = left ^ right;
        bool y_ways = back ^ fore ;
        if (!x_ways && !y_ways) return Vector2{};
        return normalize(Vector2{
            x_ways*(right - left)*1.,
            y_ways*(fore  - back)*1.});
    }

    bool is_starting_jump() const
        { return !m_jump_pressed_before && m_jump_this_frame; }

    bool is_ending_jump() const
        { return m_jump_pressed_before && !m_jump_this_frame; }

private:

    static int to_index(KeyControl ky) {
        using Kc = KeyControl;
        switch (ky) {
        case Kc::forward : return 0; case Kc::backward: return 1;
        case Kc::left    : return 2; case Kc::right   : return 3;
        default: break;
        }
        throw std::runtime_error{""};
    }
    std::array<bool, 4> m_dir = std::array<bool, 4>{};
    bool m_jump_pressed_before = false;
    bool m_jump_this_frame = false;
};
