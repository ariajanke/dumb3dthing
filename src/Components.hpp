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
#include "platform/platform.hpp"

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

class TaskCallbacks {
public:
    virtual ~TaskCallbacks() {}

    virtual void add(const SharedPtr<EveryFrameTask> &) = 0;

    virtual void add(const SharedPtr<OccasionalTask> &) = 0;

    virtual void add(const SharedPtr<LoaderTask> &) = 0;

    virtual void add(const Entity &) = 0;

    virtual Platform::ForLoaders & platform() = 0;
};

// EveryFrameTasks are retained and ran every frame
// when the scheduler/driver becomes the sole owner, it removes the task
// without any further calls
class EveryFrameTask {
public:
    using Callbacks = TaskCallbacks;

    virtual ~EveryFrameTask() {}

    virtual void on_every_frame(Callbacks &, Real et) = 0;

    template <typename Func>
    static SharedPtr<EveryFrameTask> make(Func && f_) {
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
};

// An occasional task, is called only once, before being removed by the
// scheduler/driver. It should not be possible that driver/scheduler is the
// sole owner, as the owning entity should survive until the end of the frame,
// when usual frame clean up occurs.
class OccasionalTask {
public:
    using Callbacks = TaskCallbacks;

    virtual ~OccasionalTask() {}

    virtual void on_occasion(Callbacks &) = 0;
    template <typename Func>

    static SharedPtr<OccasionalTask> make(Func && f_) {
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
        virtual PlayerEntities player_entites() const = 0;

        virtual void set_player_entities(const PlayerEntities & entities) = 0;
    };

    virtual ~LoaderTask() {}

    /** When the driver uses a loader it does several things with the values
     *  returned to it.
     *
     *  - If the player entities returned are different from what's passed,
     *    then it deletes the old player entities from the scene.
     *  - All new entities are added to the scene
     *  - All non-builtin single systems, and trigger systems are deleted and
     *    replaced with what's returned in the tuple
     *
     *  @note if you want to delete any old entities, the trigger system
     *        responsible for creating this loader should make calls to
     *        request_deletion for each entity to delete
     *
     *  @param player_entities old player entities before this loader was
     *         called, they will be "null" if player entities haven't been
     *         created yet (driver startup)
     *  @param callbacks provided functions by the platform for loaders
     *
     */
    virtual void operator () (Callbacks &) const = 0;

    template <typename Func>
    static SharedPtr<LoaderTask> make(Func && f_) {
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
};
