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

#include "../../Definitions.hpp"
#include "../../GameDriver.hpp"
#include "../../Configuration.hpp"
#include "../../Components.hpp"
#include "../../point-and-plane.hpp"
#include "GlmVectorTraits.hpp"

#include "RenderModelImpl.hpp"
#include "TextureImpl.hpp"
#include "ShaderProgram.hpp"
#include "GlmDefs.hpp"

#include <iostream>
#include <map>

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <cassert>

// Mapping, by tiles: slopes, flats, pits, voids
// slopes: different values for elavation, however no walls
// flats: single value for elavation
// pits: no geometry, passable, however no floor (and therefore no elavation)
// voids: no geometry, non-passable, like an infinitely high wall

namespace {

constexpr const unsigned     k_window_width  = 800;
constexpr const unsigned     k_window_height = 600;
constexpr const char * const k_window_title  = "Dumb little 3D map project";

struct GlfwLibraryRAII {
    GlfwLibraryRAII() {
        // leaks 72 bytes of memory
        // fixed on glfw 3.2
        glfwInit();
    }
    ~GlfwLibraryRAII() {
        // glfw: terminate, clearing all previously allocated GLFW resources.
        // ------------------------------------------------------------------
        glfwTerminate();
    }
};

struct GLFWwindowDeleter {
    void operator () (GLFWwindow * wptr) const
        { glfwDestroyWindow(wptr); }
};

using GlfwWindowPtr = UniquePtr<GLFWwindow, GLFWwindowDeleter>;

void framebuffer_size_callback(GLFWwindow * window, int width, int height);
void process_input(GLFWwindow * window);

template <typename T>
void print_out_type_info() {
    std::cout << "size " << sizeof(T) << " align " << alignof(T) << std::endl;
}

void print_out_lost_file_content(const char * filename) {
    if constexpr (k_report_lost_file_string_content) {
        std::cout << "Lost file contents for \"" << filename << "\""
                  << std::endl;
    }
}

void print_out_lost_file_content(const std::string & filename)
    { print_out_lost_file_content(filename.c_str()); }

// "cleans up" events before sending them to the driver
class EventProcessor final {
public:
    EventProcessor(GameDriver & driver): m_driver(driver) {}

    void process_input(GLFWwindow * window);

private:
    struct KeyState final {
        bool on_frame_before = false;
        bool on_frame_now    = false;
    };

    struct KeyLessThan final {
        bool operator () (const KeyControl & lhs, const KeyControl & rhs) const noexcept
            { return static_cast<int>(lhs) < static_cast<int>(rhs); }
    };

    std::map<KeyControl, KeyState, KeyLessThan> m_key_state;
    GameDriver & m_driver;
};

class BlockingFileContentPromising final {
public:
    FutureStringPtr promise_file_contents(const char * filename) {
        class Impl final : public Future<std::string> {
        public:
            Impl(const char * filename):
                m_filename(filename),
                m_contents(file_to_string(filename)) {}

            OptionalEither<Lost, std::string> retrieve() final {
                if (m_contents)
                    { return std::move(*m_contents); }
                print_out_lost_file_content(m_filename);
                return Lost{};
            }

        private:
            const char * m_filename = nullptr;
            Optional<std::string> m_contents;
        };
        return make_shared<Impl>(filename);
    }

    void progress_file_promises() {}
};

class SingleFrameFileContentPromising final {
public:
    FutureStringPtr promise_file_contents(const char * filename) {
        m_unprocessed.emplace_back(make_shared<FutureStringImpl>(filename));
        return m_unprocessed.back();
    }

    void progress_file_promises() {
        for (auto & unprocessed : m_unprocessed) {
            unprocessed->progress();
        }
        m_unprocessed.clear();
    }

private:
    class FutureStringImpl final : public Future<std::string> {
    public:
        FutureStringImpl(const char * filename):
            m_filename(filename) {}

        OptionalEither<Lost, std::string> retrieve() final {
            if (!m_loaded)
                { return {}; }
            if (m_contents)
                { return std::move(*m_contents); }
            print_out_lost_file_content(m_filename);
            return Lost{};
        }

        void progress() {
            m_loaded = true;
            m_contents = file_to_string(m_filename.c_str());
        }

    private:
        bool m_loaded = false;
        std::string m_filename;
        Optional<std::string> m_contents;
    };

    std::vector<SharedPtr<FutureStringImpl>> m_unprocessed;
};

class NativePlatformCallbacks final : public Platform {
public:
    using FilePromiser = std::conditional_t
        <k_promised_files_take_at_least_one_frame,
         SingleFrameFileContentPromising,
         BlockingFileContentPromising>;

    explicit NativePlatformCallbacks(ShaderProgram & shader):
        m_shader(shader) {}

    void render_scene(const Scene &) final;

    Entity make_renderable_entity() const final;

    SharedPtr<Texture> make_texture() const final;

    SharedPtr<RenderModel> make_render_model() const final;

    void set_camera_entity(EntityRef) final;

    glm::mat4 get_view() const;

    FutureStringPtr promise_file_contents(const char * filename);

    void progress_file_promises() { m_file_promiser.progress_file_promises(); }

private:
    ShaderProgram & m_shader;
    EntityRef m_camera_ent;
    FilePromiser m_file_promiser;
};

} // end of <anonymous> namespace

// ----------------------------------------------------------------------------

int main() {
    (void)cul::VectorTraits<glm::vec<3, Real, glm::highp>>{};
    // glfw: initialize and configure
    // ------------------------------
    GlfwLibraryRAII glfw_raii; (void)glfw_raii;
    auto gamedriver = GameDriver::make_instance();
    EventProcessor events{*gamedriver};

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#   ifdef __APPLE__
    // uncomment this statement to fix compilation on OS X
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#   endif

    // glfw window creation
    // --------------------
    GlfwWindowPtr window {
        glfwCreateWindow(k_window_width, k_window_height,
                         k_window_title, nullptr, nullptr)
    };

    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(&*window);
    glfwSetFramebufferSizeCallback(&*window, framebuffer_size_callback);

    glfwSetWindowUserPointer(&*window, &events);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    ShaderProgram shader;
    shader.load_builtin();
    NativePlatformCallbacks npcallbacks{shader};
    gamedriver->setup(npcallbacks);

    {
    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "GL error: " << err << std::endl;
    }
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LESS);

    glfwSetTime(0.);
    while (!glfwWindowShouldClose(&*window)) {
        process_input(&*window);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glad_glGetError();

        shader.use();
        glm::mat4 model = identity_matrix<glm::mat4>();
        shader.set_mat4("model", model);
        shader.set_mat4("view" , npcallbacks.get_view());

        int window_width, window_height;
        glfwGetWindowSize(&*window, &window_width, &window_height);
        shader.set_mat4("projection", glm::perspective(
            glm::radians(45.0f), float(window_width) / float(window_height),
            0.001f, 100.0f));

        npcallbacks.progress_file_promises();
        gamedriver->update(1. / 60., npcallbacks);
        glfwSetTime(0.);

        // there's a lot of shader specific things that happens
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------

        glfwSwapBuffers(&*window);
        glfwPollEvents();
    }

    return 0;
}

namespace {

struct PpStateModelMatrixAdjustment final {
    void operator () (PpState & state, glm::mat4 & model) const;
};

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void process_input(GLFWwindow * window) {
    using namespace std;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    reinterpret_cast<EventProcessor *>(glfwGetWindowUserPointer(window))->process_input(window);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// ----------------------------------------------------------------------------

void EventProcessor::process_input(GLFWwindow * window) {
    static const std::map<int, KeyControl> k_key_control_map = {
        std::make_pair(GLFW_KEY_A, KeyControl::left      ),
        std::make_pair(GLFW_KEY_S, KeyControl::backward  ),
        std::make_pair(GLFW_KEY_D, KeyControl::right     ),
        std::make_pair(GLFW_KEY_W, KeyControl::forward   ),
        std::make_pair(GLFW_KEY_L, KeyControl::jump      ),

        std::make_pair(GLFW_KEY_P, KeyControl::pause     ),
        std::make_pair(GLFW_KEY_O, KeyControl::advance   ),
        std::make_pair(GLFW_KEY_I, KeyControl::print_info),

        std::make_pair(GLFW_KEY_F5, KeyControl::restart)
    };

    for (const auto & [glfw_key, driver_key] : k_key_control_map) {
        auto & state = m_key_state[driver_key];
        switch (glfwGetKey(window, glfw_key)) {
        case GLFW_PRESS  : state.on_frame_now = true ; break;
        case GLFW_RELEASE: state.on_frame_now = false; break;
        default: break;
        }
    }
    for (auto & [driver_key, state] : m_key_state) {
        if (state.on_frame_now != state.on_frame_before) {
            if (state.on_frame_before)
                { m_driver.release_key(driver_key); }
            else
                { m_driver.press_key  (driver_key); }
        }
        // finish up for next frame
        state.on_frame_before = state.on_frame_now;
    }
}

// ----------------------------------------------------------------------------

void NativePlatformCallbacks::render_scene(const Scene & scene) {
    m_shader.set_float("tex_alpha", 1.f);
    m_shader.set_vec2("tex_offset", glm::vec2{0.f, 0.f});

    // model matrix... maybe this also should be a component, though that
    // introduces a nasty dependancy
    ecs::make_singles_system<Entity>([] (glm::mat4 & model, ModelTranslation & trans_) {
        model = glm::translate(identity_matrix<glm::mat4>(), convert_to<glm::vec3>(trans_.value));
    }, [] (glm::mat4 & model, YRotation & rot_) {
        // was called "z" rotation...
        model = glm::rotate(model, float(rot_.value), convert_to<glm::vec3>(k_up));
    }, [] (glm::mat4 & model, ModelScale & scale_) {
        model = glm::scale(model, convert_to<glm::vec3>(scale_.value));
    },
    PpStateModelMatrixAdjustment{},
    [] (EcsOpt<ModelVisibility> vis, SharedPtr<const Texture> & texture) {
        if (!should_be_visible(vis)) return;
        texture->bind_texture();
    }
    , [this] (EcsOpt<ModelVisibility> vis, glm::mat4 & model, SharedPtr<const RenderModel> & mod_) {
        if (!should_be_visible(vis)) return;
        m_shader.set_mat4("model", model);
        mod_->render();
    }

    )(scene);
}

Entity NativePlatformCallbacks::make_renderable_entity() const {
    auto e = Entity::make_sceneless_entity();
    e.add<glm::mat4>() = identity_matrix<glm::mat4>();
    return e;
}

SharedPtr<Texture> NativePlatformCallbacks::make_texture() const
    { return make_shared<OpenGlTexture>(); }

SharedPtr<RenderModel> NativePlatformCallbacks::make_render_model() const
    { return make_shared<OpenGlRenderModel>(); }

void NativePlatformCallbacks::set_camera_entity(EntityRef eref)
    { m_camera_ent = eref; }

glm::mat4 NativePlatformCallbacks::get_view() const {
    auto to_glmv3 = [] (const Vector & r) { return convert_to<glm::vec3>(r); };
    if (Entity e{m_camera_ent}) {
        const auto & cam = e.get<Camera>();
        return glm::lookAt
            (to_glmv3(cam.position), to_glmv3(cam.target), to_glmv3(cam.up));
    } else {
        return glm::mat4{
            { 1, 0, 0, 0 },
            { 0, 1, 0, 0 },
            { 0, 0, 1, 0 },
            { 0, 0, 0, 1 }
        };
        throw std::runtime_error{"Camera entity not set, no view possible"};
    }
}

FutureStringPtr NativePlatformCallbacks::promise_file_contents
    (const char * filename)
{ return m_file_promiser.promise_file_contents(filename); }

// ----------------------------------------------------------------------------

void PpStateModelMatrixAdjustment::operator ()
    (PpState & state, glm::mat4 & model) const
{
    auto * on_surface = std::get_if<PpOnSegment>(&state);
    if (!on_surface) return;
    auto norm = on_surface->segment->normal();
    if (are_very_close(norm - k_up, Vector{})) {
        // directly oppose each other, axis not servicable
        // I need a vector orthogonal to up
        Vector axis;
        for (auto v : {Vector{1, 1, 1}, Vector{-1, 1, 1}, Vector{-1, -1, 1}}) {
            axis = normalize(cul::project_onto_plane(v, k_up));
            if (are_very_close(cul::dot(axis, k_up), 0)) break;
        }
        assert(are_very_close(cul::dot(axis, k_up), 0));
        model = glm::rotate(model, float(k_pi), convert_to<glm::vec3>(axis));
        return;
    }

    auto angle = angle_between(norm, k_up);
    auto crp = cross(norm, k_up);
    if (are_very_close(crp, Vector{})) return;
    model = glm::rotate(model, float(angle), convert_to<glm::vec3>(normalize(crp)));
}

} // end of <anonymous> namespace
