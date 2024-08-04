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

#include <ariajanke/ecs3/SingleSystem.hpp>
#include <ariajanke/cul/OptionalEither.hpp>

class Texture;

class RenderModel;

enum class KeyControl {
    forward,
    backward,
    left,
    right,
    jump,
    pause,
    advance,
    print_info,
    restart,
    camera_left,
    camera_right
};

template <typename T>
class Future {
public:
    struct Lost final {};

    virtual ~Future() {}

    virtual OptionalEither<Lost, T> retrieve() = 0;
};

// TODO refactor allowing only one unique retriever
using FutureStringPtr = SharedPtr<Future<std::string>>;

class PlatformAssetsStrategy {
public:
    virtual ~PlatformAssetsStrategy() {}

    /// @returns created platform dependant texture.
    virtual SharedPtr<Texture> make_texture() const = 0;

    /// @returns created platform dependant render model
    virtual SharedPtr<RenderModel> make_render_model() const = 0;

    /** @returns special future string type, promising the contents of a
     *           file.
     *
     *  @note This design is in place in enable compatibility without
     *        having to use a blocking call with Web Assembly.
     */
    // TODO should be const, as there are no side-effects outside the file
    // loading process
    virtual FutureStringPtr promise_file_contents(const char *) const = 0;
};

class ScenePresentation {
public:
    virtual ~ScenePresentation() {}

    /** Renders an entire scene, using graphical components of every entity
     */
    virtual void render_scene(const Scene &) = 0;

    /** There's only one camera per load, use it wisely!
     *
     *  Perhaps save the camera for the player.
     */
    virtual void set_camera_entity(EntityRef) = 0;

    // TODO I need a UI at some point
    // for now, it can be very simple
    // just a set of lines for map loading warnings and errors
};

/** Represents the platform on which the application runs. This class is a way
 *  for the driver/loaders/certain systems to obtain platform specific
 *  functionality.
 *
 *  @note run platform dependant code
 */
class Platform : public PlatformAssetsStrategy, public ScenePresentation {};
