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

#include "Definitions.hpp"

class Platform;
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

class BackgroundTask {
public:
    class Continuation {
    public:
        [[nodiscard]] static Continuation & task_completion();

        virtual Continuation & wait_on(const SharedPtr<BackgroundTask> &) = 0;

        virtual ~Continuation() {}
    };

    class ContinuationStrategy {
    public:
        virtual ~ContinuationStrategy() {}

        virtual Continuation & continue_() = 0;

        [[nodiscard]] Continuation & finish_task()
            { return Continuation::task_completion(); }
    };

    using Callbacks = TaskCallbacks;

    virtual ~BackgroundTask() {}

    [[nodiscard]] virtual Continuation & in_background
        (Callbacks &, ContinuationStrategy &) = 0;

    template <typename Func>
    static SharedPtr<BackgroundTask> make(Func && f_);
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
/* static */ SharedPtr<BackgroundTask> BackgroundTask::make(Func && f_) {
    class Impl final : public BackgroundTask {
    public:
        Impl(Func && f_): m_f(std::move(f_)) {}

        Continuation & in_background
            (Callbacks & callbacks, ContinuationStrategy & strategy) final
        { return m_f(callbacks, strategy); }

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
