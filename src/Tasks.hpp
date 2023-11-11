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

enum class BackgroundCompletion {
    finished, in_progress
};

class BackgroundDelayTask;

class BackgroundTaskCompletion final {
public:
    enum class Status { finished, in_progress, delay_until, invalid };

    static const BackgroundTaskCompletion k_finished;

    static const BackgroundTaskCompletion k_in_progress;

    template <typename Func>
    static BackgroundTaskCompletion delay_until(Func && f);

    explicit BackgroundTaskCompletion
        (SharedPtr<BackgroundDelayTask> && delay_task_):
        BackgroundTaskCompletion(Status::delay_until, std::move(delay_task_)) {}

    bool operator == (const BackgroundTaskCompletion &) const;

    SharedPtr<BackgroundDelayTask> move_out_delay_task();

private:
    static SharedPtr<BackgroundDelayTask> verify_return_task
        (SharedPtr<BackgroundDelayTask> &&);

    BackgroundTaskCompletion
        (Status status_,
         SharedPtr<BackgroundDelayTask> && delay_task_);

    Status m_status;
    SharedPtr<BackgroundDelayTask> m_delay_task;
};

class BackgroundTask {
public:
    using Callbacks = TaskCallbacks;

    virtual ~BackgroundTask() {}

    virtual BackgroundTaskCompletion operator () (Callbacks &) = 0;

    template <typename Func>
    static SharedPtr<BackgroundTask> make(Func && f_);
};

class BackgroundDelayTask : public BackgroundTask {
public:
    BackgroundTaskCompletion operator () (Callbacks &) final;

    void set_return_task(SharedPtr<BackgroundTask> && task_);

    virtual BackgroundTaskCompletion on_delay(Callbacks &) = 0;

private:
    SharedPtr<BackgroundTask> m_return_task;
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
    BackgroundTaskCompletion operator () (Callbacks &) final;
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

        BackgroundTaskCompletion operator () (Callbacks & callbacks) final
            { return m_f(callbacks); }

    private:
        Func m_f;
    };
    return make_shared<Impl>(std::move(f_));
}

template <typename Func>
/* static */ BackgroundTaskCompletion BackgroundTaskCompletion::delay_until
    (Func && f)
{
    class Impl final : public BackgroundDelayTask {
    public:
        Impl(Func && f_): m_f(std::move(f_)) {}

        BackgroundTaskCompletion on_delay(Callbacks & callbacks) final
            { return m_f(callbacks); }
    private:
        Func m_f;
    };

    return BackgroundTaskCompletion
        {Status::delay_until, make_shared<Impl>(std::move(f))};
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
