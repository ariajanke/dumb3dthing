#include "../platform.hpp"
#include "../../Texture.hpp"
#include "../../RenderModel.hpp"
#include "../../GameDriver.hpp"

#include <emscripten.h>

#include <common/Util.hpp>

// ----------------------------- Texture Operations ---------------------------

EM_JS(int, from_js_create_texture, (), {
    return jsPlatform.createTexture();
});

// platform should keep track of the texture
// should be the case that: jsPlatform.UTF8ToString == Module.UTF8ToString
EM_JS(void, from_js_load_texture, (int handle, const char * filename), {
    // Still have a possible problem when trying to read textuer width/height
    jsPlatform.getTexture(handle).load(jsPlatform.UTF8ToString(filename));
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

EM_JS(void, from_js_model_matrix_rotate_y, (float angle), {
    jsPlatform.modelMatrix.rotateY(angle);
});

EM_JS(void, from_js_model_matrix_translate, (float x, float y, float z), {
    jsPlatform.modelMatrix.translate([x, y, z]);
});

// did not enjoy this...
EM_JS(void, from_js_projection_matrix_look_at,
      (float eyeX, float eyeY, float eyeZ,
       float cenX, float cenY, float cenZ,
       float upX, float upY, float upZ),
{
    jsPlatform.projectionMatrix.lookAt(
        [eyeX, eyeY, eyeZ],
        [cenX, cenY, cenZ],
        [upX , upY , upZ ]);
});

// I'm going to need other functions for matrix transformations/operations

namespace {

using namespace cul::exceptions_abbr;

class WebGlTexture final : public Texture {
public:
    static constexpr int k_no_handle = -1;

    WebGlTexture() {}

    WebGlTexture(const OpenGlTexture &) = delete;

    WebGlTexture(OpenGlTexture &&) = delete;

    ~WebGlTexture() final {
        if (m_handle == k_no_handle) return;
        from_js_destroy_texture(m_handle);
    }

    OpenGlTexture & operator = (const OpenGlTexture &) = delete;

    OpenGlTexture & operator = (OpenGlTexture &&) = delete;

    bool load_from_file_no_throw(const char * filename) noexcept final {
        if (m_handle == k_no_handle) {
            m_handle = from_js_create_texture();
        }

        // How to handle problems with loading?
        from_js_load_texture(m_handle, filename);
        m_width  = from_js_get_width (m_handle);
        m_height = from_js_get_height(m_handle);
        return true;
    }

    void load_from_memory(int, int, const void *) final {
        throw RtError{"WebGlTexture::load_from_memory: Loading texture from "
                      "memory not supported on this platform."};
    }

    int width () const final { return m_width ; }

    int height() const final { return m_height; }

    void bind_texture(/* there is a rendering context in WebGL */) const final {
        from_js_bind_texture(m_handle);
    }

private:
    int m_handle = k_no_handle;
    int m_width = 0;
    int m_height = 0;
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
        // since we're stuck on a single thread anyhow
        static std::vector<float> positions;
        static std::vector<float> txPositions;
        static std::vector<uint16_t> elements;

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

class WebGlPlatform final : public Platform::Callbacks {
public:
    static WebGlPlatform & instance() {
        static WebGlPlatform inst;
        return inst;
    }

    void render_scene(const Scene & scene) final {
        for (auto & ent : scene) {
            if (!ent.has_all<SharedPtr<Texture>, SharedPtr<RenderModel>>()) continue;
            if (auto * translation = ent.ptr<Translation>()) {
                const auto & r = translation->value;
                from_js_model_matrix_translate(r.x, r.y, r.z);
            }
            if (auto * y_rotation = ent.ptr<YRotation>()) {
                from_js_model_matrix_rotate_y(y_rotation->value);
            }
            auto [texture, render_model] = ent.get<SharedPtr<Texture>, SharedPtr<RenderModel>>();
            texture->bind();
            render_model->render();
        }
        if (Entity e{m_camera_ent}) {
            const auto & cam = e.get<Camera>();
            // something with camera...
            from_js_projection_matrix_look_at(
                cam.position.x, cam.position.y, cam.position.z,
                cam.target  .x, cam.target  .y, cam.target  .z,
                cam.up      .x, cam.up      .y, cam.up      .z);
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

private:
    EntityRef m_camera_ent;
};

static UniquePtr<Driver> s_driver;

} // end of <anonymous> namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE void to_js_start_up() {
    s_driver = Driver::make_instance();
    s_driver->setup(WebGlPlatform::instance());
}

EMSCRIPTEN_KEEPALIVE void to_js_press_key(int key) {
    s_driver->press_key(static_cast<KeyControl>(key));
}

EMSCRIPTEN_KEEPALIVE void to_js_release_key(int key) {
    s_driver->release_key(static_cast<KeyControl>(key));
}

EMSCRIPTEN_KEEPALIVE void to_js_update(float et_in_seconds) {
    s_driver->update(et_in_seconds, WebGlPlatform::instance());
}

}