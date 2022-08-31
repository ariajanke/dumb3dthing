#include "../platform.hpp"
#include "../../Texture.hpp"
#include "../../RenderModel.hpp"

#include <emscripten.h>

#include <common/Util.hpp>

// ----------------------------- Texture Operations ---------------------------

EM_JS(int, from_js_create_texture, (), {
    return jsPlatform.createTexture();
});

// platform should keep track of the texture
// should be the case that: jsPlatform.UTF8ToString == Module.UTF8ToString
EM_ASYNC_JS(void, from_js_load_texture, (int handle, const char * filename), {
    await jsPlatform.getTexture(handle).load(jsPlatform.UTF8ToString(filename));
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
    jsPlatform.getTexture(handle).bind();
});

// ------------------------- RenderModel Operations ---------------------------

EM_JS(int, from_js_create_render_model, (), {

});

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
    ~WebGlRenderModel() final {

    }

    // no transformations -> needs to be done seperately
    void render() const final {

    }

    bool is_loaded() const noexcept final {

    }

private:
    void load_(const Vertex   * vertex_beg  , const Vertex   * vertex_end,
               const unsigned * elements_beg, const unsigned * elements_end) final
    {

    }
};

class WebGlPlatform final : public Platform::Callbacks {
public:
    void render_scene(const Scene &) final;

    Entity make_renderable_entity() const final
        {  }

    SharedPtr<Texture> make_texture() const final
        { return make_shared<WebGlTexture>(); }

    SharedPtr<RenderModel> make_render_model() const final
        { return make_shared<WebGlRenderModel>(); }

    void set_camera_entity(EntityRef ref) final
        { m_camera_ent = ref; }

private:
    EntityRef m_camera_ent;
};

} // end of <anonymous> namespace


