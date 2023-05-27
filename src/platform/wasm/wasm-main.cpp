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

#include "../../platform.hpp"
#include "../../Texture.hpp"
#include "../../RenderModel.hpp"
#include "../../GameDriver.hpp"
#include "../../Components.hpp"

#include <emscripten.h>

#include <ariajanke/cul/Util.hpp>

#include <set>
#include <iostream>

EM_JS(void, from_js_log_line, (const char * str), {
    console.log(Module.UTF8ToString(str));
});

// ----------------------------- Texture Operations ---------------------------

EM_JS(int, from_js_create_texture, (), {
    return jsPlatform.createTexture();
});

// platform should keep track of the texture
// should be the case that: jsPlatform.UTF8ToString == Module.UTF8ToString
EM_JS(void, from_js_load_texture, (int handle, const char * filename), {
    // Still have a possible problem when trying to read textuer width/height
    // get the string conversion function from Module and yeet that name!
    const texture = jsPlatform.getTexture(handle);
    texture.load(Module.UTF8ToString(filename));
    texture.setUnit(0); // all 0 for now...
});

EM_JS(void, from_js_destroy_texture, (int handle), {
    jsPlatform.destroyTexture(handle);
});

EM_JS(int, from_js_get_height, (int handle), {
    return jsPlatform.getTexture(handle).height();
});

EM_JS(int, from_js_get_width, (int handle), {
    return jsPlatform.getTexture(handle).width();
});

// Platform knows the context
EM_JS(void, from_js_bind_texture, (int handle), {
    jsPlatform.bindTexture(handle);
});

// ------------------------- RenderModel Operations ---------------------------

EM_JS(int, from_js_create_render_model, (), {
    return jsPlatform.createRenderModel();
});

EM_JS(void, from_js_load_render_model, (
    int handle,
    const float * positionsBeg  , const float * positionsEnd  ,
    const float * txPositionsBeg, const float * txPositionsEnd,
    const uint16_t * elementsBeg, const uint16_t * elementsEnd),
{
    const sizeOfF32 = 4;
    const sizeOfU16 = 2;
    const positions   = Module.HEAPF32.slice(positionsBeg   / sizeOfF32, positionsEnd   / sizeOfF32);
    const txPositions = Module.HEAPF32.slice(txPositionsBeg / sizeOfF32, txPositionsEnd / sizeOfF32);
    const elements    = Module.HEAPU16.slice(elementsBeg    / sizeOfU16, elementsEnd    / sizeOfU16);
    jsPlatform.getRenderModel(handle).load(positions, txPositions, elements);
});

EM_JS(void, from_js_render_render_model, (int handle), {
    jsPlatform.renderRenderModel(handle);
});

EM_JS(void, from_js_destroy_render_model, (int handle), {
    jsPlatform.destroyRenderModel(handle);
});

// ---------------------------- Matrix Operations -----------------------------

EM_JS(void, from_js_reset_model_matrix, (), {
    jsPlatform.modelMatrix.reset();
});

EM_JS(void, from_js_model_matrix_rotate_y, (float angle), {
    jsPlatform.modelMatrix.rotateY(angle);
});

EM_JS(void, from_js_model_matrix_translate, (float x, float y, float z), {
    jsPlatform.modelMatrix.translate([x, y, z]);
});

EM_JS(void, from_js_model_matrix_apply, (), {
    jsPlatform.modelMatrix.apply();
});

// did not enjoy this...
EM_JS(void, from_js_view_matrix_look_at,
      (float eyeX, float eyeY, float eyeZ,
       float cenX, float cenY, float cenZ,
       float upX, float upY, float upZ),
{
    jsPlatform.viewMatrix.lookAt(
        [eyeX, eyeY, eyeZ],
        [cenX, cenY, cenZ],
        [upX , upY , upZ ]);
});

EM_JS(void, from_js_view_matrix_apply, (), {
    jsPlatform.viewMatrix.apply();
});

// ----------------------------------------------------------------------------

EM_JS(void, from_js_promise_file_contents_as_string, (const void * instance, const char * filename), {
    jsPlatform.promiseFileContents(Module.UTF8ToString(filename), instance);
});

// I'm going to need other functions for matrix transformations/operations

namespace {

using namespace cul::exceptions_abbr;

class WebGlTexture final : public Texture {
public:
    static constexpr int k_no_handle = -1;

    WebGlTexture() {}

    WebGlTexture(const WebGlTexture &) = delete;

    WebGlTexture(WebGlTexture &&) = delete;

    ~WebGlTexture() final {
        if (m_handle == k_no_handle) return;
        from_js_destroy_texture(m_handle);
    }

    WebGlTexture & operator = (const WebGlTexture &) = delete;

    WebGlTexture & operator = (WebGlTexture &&) = delete;

    bool load_from_file_no_throw(const char * filename) noexcept final {
        if (m_handle == k_no_handle) {
            m_handle = from_js_create_texture();
        }

        // How to handle problems with loading?
        from_js_load_texture(m_handle, filename);
        return true;
    }

    void load_from_memory(int, int, const void *) final {
        throw RtError{"WebGlTexture::load_from_memory: Loading texture from "
                      "memory not supported on this platform."};
    }

    int width () const final { return from_js_get_width (m_handle); }

    int height() const final { return from_js_get_height(m_handle); }

    void bind_texture(/* there is a rendering context in WebGL */) const final {
        from_js_bind_texture(m_handle);
    }

private:
    int m_handle = k_no_handle;
};

class WebGlRenderModel final : public RenderModel {
public:
    static constexpr const int k_no_handle = -1;

    ~WebGlRenderModel() final {
        if (m_handle == k_no_handle) return;
        from_js_destroy_render_model(m_handle);
    }

    // no transformations -> needs to be done seperately
    void render() const final
        { from_js_render_render_model(m_handle); }

    bool is_loaded() const noexcept final
        { return m_handle != k_no_handle; }

private:
    template <typename T>
    Tuple<const T *, const T *> get_begin_and_end(const std::vector<T> & vec) {
        if (vec.empty()) return make_tuple(nullptr, nullptr);
        return make_tuple(&vec.front(), &vec.back() + 1);
    }

    void load_(const Vertex   * vertex_beg  , const Vertex   * vertex_end,
               const unsigned * elements_beg, const unsigned * elements_end) final
    {
        if (m_handle == k_no_handle) {
            m_handle = from_js_create_render_model();
        }

        // since we're stuck on a single thread anyhow
        static std::vector<float> positions;
        static std::vector<float> txPositions;
        static std::vector<uint16_t> elements;

        positions.clear();
        txPositions.clear();
        elements.clear();

        positions  .reserve((vertex_end - vertex_beg)*3);
        txPositions.reserve((vertex_end - vertex_beg)*2);
        elements   .reserve(elements_end - elements_beg);
        for (auto * vtx = vertex_beg; vtx != vertex_end; ++vtx) {
            positions.push_back(vtx->position.x);
            positions.push_back(vtx->position.y);
            positions.push_back(vtx->position.z);

            txPositions.push_back(vtx->texture_position.x);
            txPositions.push_back(vtx->texture_position.y);
        }
        for (auto * el = elements_beg; el != elements_end; ++el) {
            elements.push_back(*el);
        }
        auto [p_beg, p_end] = get_begin_and_end(positions);
        auto [t_beg, t_end] = get_begin_and_end(txPositions);
        auto [e_beg, e_end] = get_begin_and_end(elements);
        from_js_load_render_model(m_handle, p_beg, p_end, t_beg, t_end, e_beg, e_end);
    }

    int m_handle = k_no_handle;
};

class WebFutureString final : public Future<std::string> {
public:
    explicit WebFutureString() {}

    ~WebFutureString() {}

    OptionalEither<Lost, std::string> operator () () final {
        if (m_fulfilled) {
            m_lost = true;
            m_fulfilled = false;
            return std::move(m_contents);
        } else if (m_lost) {
            return Lost{};
        } else {
            return {};
        }
    }

    void set_aside(std::size_t len)
        { m_contents.resize(len); }

    void * data() { return m_contents.data(); }

    void mark_as_fulfilled()
        { m_fulfilled = true; }

private:
    std::string m_contents;

    bool m_fulfilled = false;
    bool m_lost = false;
};

class WebGlPlatform final : public Platform {
public:
    static WebGlPlatform & instance() {
        static WebGlPlatform inst;
        return inst;
    }

    void render_scene(const Scene & scene) final {
        if (Entity e{m_camera_ent}) {
#           if 0
            from_js_log_line("[cpp]: setting camera");
#           endif
            const auto & cam = e.get<Camera>();
            // something with camera...
            from_js_view_matrix_look_at(
                cam.position.x, cam.position.y, cam.position.z,
                cam.target  .x, cam.target  .y, cam.target  .z,
                cam.up      .x, cam.up      .y, cam.up      .z);
        }

        from_js_view_matrix_apply();

        for (auto & ent : scene) {
            if (!ent.has_all
                <SharedPtr<const Texture>, SharedPtr<const RenderModel>>
                ())
            { continue; }
            if (auto * vis = ent.ptr<Visible>()) {
                if (!vis->value)
                    continue;
            }
            from_js_reset_model_matrix();
            if (auto * translation = ent.ptr<Translation>()) {
                const auto & r = translation->value;
                from_js_model_matrix_translate(r.x, r.y, r.z);
#               if 0
                from_js_log_line("[cpp]: applying translation");
#               endif
            }
            if (auto * y_rotation = ent.ptr<YRotation>()) {
                from_js_model_matrix_rotate_y(y_rotation->value);
#               if 0
                from_js_log_line("[cpp]: applying y rotation");
#               endif
            }
#           if 0
            from_js_log_line("[cpp]: rendering model");
#           endif
            auto [texture, render_model] = ent.get
                <SharedPtr<const Texture>, SharedPtr<const RenderModel>>();
            from_js_model_matrix_apply();
            texture->bind_texture();
            render_model->render();
        }
    }

    Entity make_renderable_entity() const final
        { return Entity::make_sceneless_entity(); }

    SharedPtr<Texture> make_texture() const final
        { return make_shared<WebGlTexture>(); }

    SharedPtr<RenderModel> make_render_model() const final
        { return make_shared<WebGlRenderModel>(); }

    void set_camera_entity(EntityRef ref) final
        { m_camera_ent = ref; }

    FutureStringPtr promise_file_contents(const char * filename) final {
        auto uptr = make_unique<WebFutureString>();
        from_js_promise_file_contents_as_string(uptr.get(), filename);
        return uptr;
    }

private:
    EntityRef m_camera_ent;
};

static UniquePtr<GameDriver> s_driver;

} // end of <anonymous> namespace

int main() {
    return 0;
}

extern "C" {

EMSCRIPTEN_KEEPALIVE void to_js_start_up() {
    from_js_log_line("[cpp]: driver started");
    s_driver = GameDriver::make_instance();
    s_driver->setup(WebGlPlatform::instance());
}

EMSCRIPTEN_KEEPALIVE void to_js_press_key(int key) {
    from_js_log_line("[cpp]: press key hit");
    s_driver->press_key(static_cast<KeyControl>(key));
}

EMSCRIPTEN_KEEPALIVE void to_js_release_key(int key) {
    from_js_log_line("[cpp]: release key hit");
    s_driver->release_key(static_cast<KeyControl>(key));
}

EMSCRIPTEN_KEEPALIVE void to_js_update(float et_in_seconds) {
    try {
        s_driver->update(et_in_seconds, WebGlPlatform::instance());
    } catch (std::exception & ex) {
        from_js_log_line(ex.what());
    }
}

EMSCRIPTEN_KEEPALIVE void * to_js_prepare_content_buffer(void * handle, std::size_t length) {
#   if 0
    from_js_log_line("[cpp]: prepared buffer for future");
#   endif
    auto & future = *reinterpret_cast<WebFutureString *>(handle);
    future.set_aside(length);
    return future.data();
}

EMSCRIPTEN_KEEPALIVE void to_js_mark_fulfilled(void * handle) {
#   if 0
    from_js_log_line("[cpp]: mark future as fulfilled");
#   endif
    auto & future = *reinterpret_cast<WebFutureString *>(handle);
    future.mark_as_fulfilled();
}

} // end of extern "C"
