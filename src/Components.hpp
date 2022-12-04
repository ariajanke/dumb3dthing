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

struct Translation final : public VectorLike<Translation> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
    using LikeBase::operator+=;
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

struct Visible final {
    bool & operator = (bool b) { return (value = b); }

    explicit operator bool () const noexcept
        { return value; }

    bool value = true;
};

inline bool should_be_visible(const Opt<Visible> & vis)
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

class EveryFrameTask;
class OccasionalTask;
class LoaderTask;
class BackgroundTask;

class TaskCallbacks {
public:
    virtual ~TaskCallbacks() {}

    virtual void add(const SharedPtr<EveryFrameTask> &) = 0;

    virtual void add(const SharedPtr<LoaderTask> &) = 0;

    virtual void add(const SharedPtr<BackgroundTask> &) = 0;

    virtual void add(const Entity &) = 0;

    virtual Platform & platform() = 0;
};

/** @brief An every frame task is retained and ran every frame.
 *
 * When the scheduler/driver becomes the sole owner, it removes the task
 * without any further calls.
 */
class EveryFrameTask {
public:
    using Callbacks = TaskCallbacks;

    virtual ~EveryFrameTask() {}

    virtual void on_every_frame(Callbacks &, Real et) = 0;

    template <typename Func>
    static SharedPtr<EveryFrameTask> make(Func && f_);
};


enum class BackgroundCompletion {
    finished, in_progress
};

class BackgroundTask {
public:
    using Callbacks = TaskCallbacks;

    virtual ~BackgroundTask() {}

    virtual BackgroundCompletion operator () (Callbacks &) = 0;

    template <typename Func>
    static SharedPtr<BackgroundTask> make(Func && f_);
};

// An occasional task, is called only once, before being removed by the
// scheduler/driver. It should not be possible that driver/scheduler is the
// sole owner, as the owning entity should survive until the end of the frame,
// when usual frame clean up occurs.
class OccasionalTask : public BackgroundTask {
public:
    using Callbacks = TaskCallbacks;

    virtual ~OccasionalTask() {}

    virtual void on_occasion(Callbacks &) = 0;

    template <typename Func>
    static SharedPtr<OccasionalTask> make(Func && f_);

private:
    BackgroundCompletion operator () (Callbacks & callbacks) final {
        on_occasion(callbacks);
        return BackgroundCompletion::finished;
    }
};

class LoaderTask {
public:
    struct PlayerEntities final {
        PlayerEntities() {}
        PlayerEntities(Entity physical_, Entity renderable_):
            physical(physical_), renderable(renderable_)
        {}
        Entity physical, renderable;
    };

    class Callbacks : public TaskCallbacks {
    public:
        using TaskCallbacks::add;

        virtual void add(const SharedPtr<TriangleLink> &) = 0;

        virtual void remove(const SharedPtr<const TriangleLink> &) = 0;
    };

    virtual ~LoaderTask() {}

    /**
     *
     */
    virtual void operator () (Callbacks &) const = 0;

    template <typename Func>
    static SharedPtr<LoaderTask> make(Func && f_);
};

template <typename Func>
/* static */ SharedPtr<EveryFrameTask> EveryFrameTask::make(Func && f_) {
    class Impl final : public EveryFrameTask {
    public:
        Impl(Func && f_): m_f(std::move(f_)) {}

        void on_every_frame(Callbacks & callbacks, Real et) final
            { m_f(callbacks, et); }

    private:
        Func m_f;
    };
    return make_shared<Impl>(std::move(f_));
}

template <typename Func>
/* static */ SharedPtr<OccasionalTask> OccasionalTask::make(Func && f_) {
    class Impl final : public OccasionalTask {
    public:
        Impl(Func && f_): m_f(std::move(f_)) {}

        void on_occasion(Callbacks & callbacks) final
            { m_f(callbacks); }

    private:
        Func m_f;
    };
    return make_shared<Impl>(std::move(f_));
}

template <typename Func>
/* static */ SharedPtr<BackgroundTask> BackgroundTask::make(Func && f_) {
    class Impl final : public BackgroundTask {
    public:
        Impl(Func && f_): m_f(std::move(f_)) {}

        BackgroundCompletion operator () (Callbacks & callbacks) final
            { return m_f(callbacks); }

    private:
        Func m_f;
    };
    return make_shared<Impl>(std::move(f_));
}

template <typename Func>
/* static */ SharedPtr<LoaderTask> LoaderTask::make(Func && f_) {
    class Impl final : public LoaderTask {
    public:
        explicit Impl(Func && f_): m_f(std::move(f_)) {}

        void operator () (Callbacks & callbacks) const final
            { m_f(callbacks); }

    private:
        Func m_f;
    };
    return make_shared<Impl>(std::move(f_));
}
