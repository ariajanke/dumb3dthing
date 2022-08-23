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

namespace {

using std::make_shared;

class NullRenderModel final : public RenderModel {
public:
    void render() const final {}

    bool is_loaded() const noexcept final { return m_loaded; }

private:
    void load_(const Vertex *, const Vertex *,
               const unsigned *, const unsigned *) final
    { m_loaded = true; }

    bool m_loaded = false;
};

class NullTexture final : public Texture {
public:
    ~NullTexture() final {}

private:
    bool load_from_file_no_throw(const char *) noexcept { return true; }

    void load_from_memory(int, int, const void *) final {}

    int width() const final { return 0; }

    int height() const final { return 0; }

    void bind_texture(/* there is a rendering context in WebGL */) const final {}
};

} // end of <anonymous> namespace

Platform::Callbacks & Platform::null_callbacks() {
    class Impl final : public Callbacks {
        void render_scene(const Scene &) final {}

        Entity make_renderable_entity() const final
            { return Entity::make_sceneless_entity(); }

        SharedPtr<Texture> make_texture() const final
            { return make_shared<NullTexture>(); }

        SharedPtr<RenderModel> make_render_model() const final
            { return make_shared<NullRenderModel>(); }

        void set_camera_entity(EntityRef) {}
    };
    static Impl impl;
    return impl;
}
