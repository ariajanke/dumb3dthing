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

#include "../Defs.hpp"

#include <ariajanke/ecs3/SingleSystem.hpp>

class Texture;

class RenderModel;

class Loader;

//template <typename T>
//class Promise

template <typename T>
class Future {
public:

    virtual ~Future() {}

    virtual bool is_ready() const noexcept = 0;

    virtual bool is_lost() const noexcept = 0;

    virtual T && retrieve() = 0;

};

using FutureString = UniquePtr<Future<std::string>>;

/** Represents the platform on which the application runs. This class is a way
 *  for the driver/loaders/certain systems to obtain platform specific
 *  functionality.
 *
 *  @note run platform dependant code
 */
class Platform {
public:
    class ForDriver {
    public:
        virtual ~ForDriver() {}

        /** Renders an entire scene, all entities that you wish to render, will
         *  need to have been created with the "make_renderable_entity" method.
         *
         *  ...this is only appropiate for use by the driver
         *  ...non const, you are in a sense, modifying the platform
         */
        virtual void render_scene(const Scene &) = 0;
    };

    class ForLoaders {
    public:
        virtual ~ForLoaders() {}

        /** Creates an entity with all platform specific components that will make
         *  it renderable.
         *
         *  ...this is only really appropiate for use by a loader
         *
         *  @returns a sceneless entity, with all necessary "hidden" component
         *           types that makes an entity renderable
         */
        virtual Entity make_renderable_entity() const = 0;

        /** Creates a new texture.
         *
         */
        virtual SharedPtr<Texture> make_texture() const = 0;

        /** Creates a new render model
         */
        virtual SharedPtr<RenderModel> make_render_model() const = 0;

        /** There's only one camera per load, use it wisely!
         *
         *  Perhaps save the camera for the player.
         */
        virtual void set_camera_entity(EntityRef) = 0;

        virtual FutureString promise_file_contents(const char *) = 0;

        // consider adding "side tasks"?
    };

    class Callbacks : public ForDriver, public ForLoaders {};

    static Callbacks & null_callbacks();
};

class LoaderTask;
class OccasionalTask;

class Preloader {
public:
    virtual ~Preloader() {}
    using TeardownTaskFactoryPtr = SharedPtr<std::function<SharedPtr<OccasionalTask>()>>;
    /**
     *  @note calling preloaders may have side-effects/mutate the preloader
     *        once a preloader is "used up" it should throw an exception or
     *        error up in some fashion on subsequent calls
     * @return
     */
    virtual Tuple<SharedPtr<LoaderTask>, TeardownTaskFactoryPtr> operator () () = 0;

    template <typename Func>
    static UniquePtr<Preloader> make(Func && f) {
        class Impl final : public Preloader {
        public:
            explicit Impl(Func && f_):
                m_f(std::move(f_)) {}

            Tuple<SharedPtr<LoaderTask>, TeardownTaskFactoryPtr> operator () () final
                { return m_f(); }

        private:
            Func m_f;
        };
        return make_unique<Impl>(std::move(f));
    }
};
#if 0
/** A loader provides facilities to the driver/controller
 *
 *  [NTS] Consume all loaders immediately?
 *  [NTS] going to outmode loaders, and turn it into a special case of
 *  "occasional task" where everything is more visible
 */
class Loader {
public:
    using SinglesSystemPtr = UniquePtr<ecs::SingleSystemBase<Entity>>;

    struct PlayerEntities final {
        PlayerEntities() {}
        PlayerEntities(Entity physical_, Entity renderable_):
            physical(physical_), renderable(renderable_)
        {}
        Entity physical, renderable;
    };

    class Callbacks final {
    public:
        Callbacks(Platform::ForLoaders & platform_, PlayerEntities ents,
                  std::vector<SinglesSystemPtr> & dynamic_systems,
                  Scene & scene):
            m_platform(platform_),
            m_player_ents(ents),
            m_dynamic_systems(dynamic_systems),
            m_scene(scene) {}

        void add_to_scene(Entity ent) const { return m_scene.add_entity(ent); };

        void add_dynamic_system(SinglesSystemPtr uptr) const
            { m_dynamic_systems.emplace_back(std::move(uptr)); }

        Platform::ForLoaders & platform() const
            { return m_platform; }

        PlayerEntities player_entites() const
            { return m_player_ents; }

        void set_player_entities(const PlayerEntities & entities)
            { m_player_ents = entities; }

    private:
        Platform::ForLoaders & m_platform;
        PlayerEntities m_player_ents;
        std::vector<SinglesSystemPtr> & m_dynamic_systems;
        Scene & m_scene;
    };

    virtual ~Loader() {}

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
     *  @return tuple of items for the driver
     */
    virtual void operator () (Callbacks & callbacks) const = 0;

    virtual bool reset_dynamic_systems() const { return true; }

    template <typename Func>
    static UniquePtr<Loader> make_loader(Func && f) {
        class Impl final : public Loader {
        public:
            explicit Impl(Func && f): m_f(std::move(f)) {}

            void operator () (Callbacks & callbacks) const final
                { return m_f(callbacks); }

        private:
            Func m_f;
        };
        return make_unique<Impl>(std::move(f));
    }
};
#endif
