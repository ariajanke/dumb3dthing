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

template <typename T>
class Future {
public:
    virtual ~Future() {}

    virtual bool is_ready() const noexcept = 0;

    virtual bool is_lost() const noexcept = 0;

    virtual T && retrieve() = 0;

};

using FutureStringPtr = UniquePtr<Future<std::string>>;

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

        /** Makes a special future string type, promising the contents of a
         *  file.
         *
         *  @note This design is in place in enable compatibility without
         *        having to use a blocking call with Web Assembly.
         *
         *  @return
         */
        virtual FutureStringPtr promise_file_contents(const char *) = 0;
    };

    class Callbacks : public ForDriver, public ForLoaders {};

    static Callbacks & null_callbacks();
};
