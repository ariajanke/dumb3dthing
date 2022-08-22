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

#include "platform.hpp"

#include "../Texture.hpp"
#include "../RenderModel.hpp"

Platform::Callbacks & Platform::null_callbacks() {
    class Impl final : public Callbacks {
        void render_scene(const Scene &) final {}

        Entity make_renderable_entity() const final
            { return Entity::make_sceneless_entity(); }

        SharedPtr<Texture> make_texture() const final
            { return Texture::make_null_instance(); }

        SharedPtr<RenderModel> make_render_model() const final
            { return RenderModel::make_null_instance(); }

        void set_camera_entity(EntityRef) {}
    };
    static Impl impl;
    return impl;
}
