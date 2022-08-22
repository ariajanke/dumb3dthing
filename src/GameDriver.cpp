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

#include "GameDriver.hpp"
#include "RenderModel.hpp"
#include "map-loader.hpp"
#include "Texture.hpp"
#include "ShaderProgram.hpp"
#include "GlmVectorTraits.hpp"

#include <common/TestSuite.hpp>
#include <common/StringUtil.hpp>
#include <common/BezierCurves.hpp>

#include <common/VectorUtils.hpp>
#include <common/sf/VectorTraits.hpp>

#include <iostream>
#include <fstream>
#include <set>
#include <map>

#include <ariajanke/ecs3/SingleSystem.hpp>

namespace {

using point_and_plane::location_of;

class TimeWatchingDriver : public GameDriver {
public:
    void update(Real seconds) override
        { m_passed_time += seconds; }

protected:

    Real time_since() const noexcept
        { return m_passed_time; }

private:
    Real m_passed_time = 0;
};

class TimeControl final {
public:
    void press(KeyControl ky) {
        switch (ky) {
        case Kc::advance: m_advance_frame = true; break;
        case Kc::pause: m_paused = !m_paused; break;
        default: return;
        }
    }
    void release(KeyControl ) {
#       if 0
        switch (ky) {
        case Kc::advance: break;
        case Kc::pause: m_paused = false; break;
        default: return;
        }
#       endif
    }

    void frame_update() {
        m_advance_frame = false;
    }

    bool runs_this_frame() const {
        return m_paused ? m_advance_frame : true;
    }

private:
    using Kc = KeyControl;
    bool m_paused = false, m_advance_frame = false;
};

class GameDriverCompleteN final : public DriverN {
public:
    void press_key(KeyControl) final;

    void release_key(KeyControl) final;

private:
    Loader::LoaderTuple initial_load(Platform::ForLoaders &) final;

    void update_(Real seconds) final;

    Vector m_camera_target;
    UniquePtr<point_and_plane::Driver> m_ppdriver = point_and_plane::Driver::make_driver();
    Entity m_player;
    TimeControl m_time_controller;
};

class ModelViewer final : public TimeWatchingDriver {
public:
    ModelViewer(const char * filename);

    void setup() final;
    void render(ShaderProgram &) const final;
    void update(Real) final;
    Camera camera() const final;

    void press_key(KeyControl) final {}
    void release_key(KeyControl) final {}

private:
    void update_model();

    // empty if anything is wrong
    static Entity load_model_from_string(const std::string &);

    Vector m_camera_target = Vector{ 0.5, 0.5, 0.5 };
    Scene m_scene;
    std::function<std::string()> m_reload_model;
    std::string m_filecontents;
    static constexpr const Real k_time_to_reload = 0.5;
    Real m_time_since_reload = 0;

    Entity m_model_entity;
};

struct VisibilityChain final {
    static constexpr const Real k_to_next = 1.2;
    ecs::EntityRef next;
    Real time_spent = 0;
    bool visible = true;
};

template <typename T>
using Opt = ecs::Optional<T>;

struct DragCamera final {
    Vector position = Vector{100, 100, 100};
    Real max_distance = 6;
};

inline bool is_visible(ecs::Optional<VisibilityChain> & vis)
    { return vis ? vis->visible : true; }
#if 0
void run_default_rendering_systems(const Scene & scene, ShaderProgram & shader) {
    shader.set_float("tex_alpha", 1.f);
    shader.set_vec2("tex_offset", glm::vec2{0.f, 0.f});

    // model matrix... maybe this also should be a component, though that
    // introduces a nasty dependancy
    ecs::make_singles_system<Entity>([] (glm::mat4 & model, Translation & trans_) {
        model = glm::translate(identity_matrix<glm::mat4>(), convert_to<glm::vec3>(trans_.value));
    }, [] (glm::mat4 & model, YRotation & rot_) {
        // was called "z" rotation...
        model = glm::rotate(model, float(rot_.value), convert_to<glm::vec3>(k_up));
    },
#   if 0
    [](point_and_plane::State & state, glm::mat4 & model) {
        auto on_surface = std::get_if<point_and_plane::OnSurface>(&state);
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
    },
#   endif
    [] (Opt<VisibilityChain> vis, std::shared_ptr<Texture> & texture) {
        if (!is_visible(vis)) return;
        texture->bind_texture();
    }, [&shader] (Opt<TextureTranslation> translation) {
        shader.set_vec2("tex_offset", convert_to<glm::vec2>(translation ? translation->value : Vector2{}));
    }, [&shader] (Opt<VisibilityChain> vis, glm::mat4 & model, RenderModelPtr mod_) {
        if (!is_visible(vis)) return;
        shader.set_mat4("model", model);
        mod_->render();
    })(scene);
}
#endif
} // end of <anonymous> namespace

/* static */ std::unique_ptr<GameDriver> GameDriver::make_model_viewer(const char * filename)
    { return std::make_unique<ModelViewer>(filename); }

/* static */ UniquePtr<DriverN> DriverN::make_instance()
    { return std::make_unique< GameDriverCompleteN >(); }

namespace {

// more component types
struct ModelTarget final : VectorLike<ModelTarget> {
    using LikeBase::LikeBase;
};

struct Elements final {
    std::vector<int> values;
};

struct Verticies final {
    std::vector<Vertex> values;
};

struct Units final {
    int texture = 1;
    int verticies = 1;
};

struct Velocity final : public VectorLike<Velocity> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
};

struct JumpVelocity final : public VectorLike<JumpVelocity> {
    using LikeBase::LikeBase;
    using LikeBase::operator=;
};

class PlayerControl final {
public:
    void press(KeyControl ky) {
        using Kc = KeyControl;
        switch (ky) {
        case Kc::forward: case Kc::backward: case Kc::left: case Kc::right:
            m_dir[to_index(ky)] = true;
            break;
        case Kc::jump:
            m_jump_this_frame = true;
            break;
        default: return;
        }
    }

    void release(KeyControl ky) {
        using Kc = KeyControl;
        switch (ky) {
        case Kc::forward: case Kc::backward: case Kc::left: case Kc::right:
            m_dir[to_index(ky)] = false;
            break;
        case Kc::jump:
            m_jump_this_frame = false;
            break;
        default: return;
        }
    }

    void frame_update() {
        m_jump_pressed_before = m_jump_this_frame;
    }

    Vector2 heading() const {
        using Kc = KeyControl;
        auto left  = m_dir[to_index(Kc::left    )];
        auto right = m_dir[to_index(Kc::right   )];
        auto back  = m_dir[to_index(Kc::backward)];
        auto fore  = m_dir[to_index(Kc::forward )];
        return Vector2{
            (left ^ right)*(-left + right)*1.,
            (back ^ fore )*(-back + fore )*1.};
    }

    bool is_starting_jump() const
        { return !m_jump_pressed_before && m_jump_this_frame; }

    bool is_ending_jump() const
        { return m_jump_pressed_before && !m_jump_this_frame; }

private:

    static int to_index(KeyControl ky) {
        using Kc = KeyControl;
        switch (ky) {
        case Kc::forward : return 0; case Kc::backward: return 1;
        case Kc::left    : return 2; case Kc::right   : return 3;
        default: break;
        }
        throw std::runtime_error{""};
    }
    std::array<bool, 4> m_dir = std::array<bool, 4>{};
    bool m_jump_pressed_before = false;
    bool m_jump_this_frame = false;
};

/**
 * @brief get_new_velocity
 * @param velocity
 * @param player_control
 * @param pstate
 * @param willed_dir assumed to have gravity subtracted, and a normal vector
 * @param seconds
 * @return
 */
Velocity get_new_velocity
    (const Velocity & velocity,
     const Vector & willed_dir, Real seconds)
{
    constexpr const Real k_max_willed_speed = 5;
    constexpr const Real k_max_acc = 5; // u/s^2
    constexpr const Real k_min_acc = 1;
    constexpr const Real k_unwilled_acc = 3;
    assert(   are_very_close(magnitude(willed_dir), 1)
           || are_very_close(magnitude(willed_dir), 0));
    // unwilled deceleration
    if (are_very_close(magnitude(willed_dir), 0)) {
        auto new_speed = magnitude(velocity.value) - k_unwilled_acc*seconds;
        if (new_speed <= 0) return Velocity{};
        return Velocity{ new_speed*normalize(velocity.value) };
    }

    auto dir_boost = are_very_close(velocity.value, Vector{}) ? 0 : (angle_between(velocity.value, willed_dir) / k_pi);
    auto low_speed_boost = 1 - std::min(magnitude(velocity.value) / k_max_willed_speed, Real(0));
    auto t = dir_boost*low_speed_boost;
    auto acc = (1 - t)*k_min_acc + t*k_max_acc;
    if (t > 0.5) {
        int i = 0;
        ++i;
    }

    auto new_vel = velocity.value + willed_dir*seconds*acc;
    if (   magnitude(velocity.value) > k_max_willed_speed
        && magnitude(new_vel) >= magnitude(velocity.value))
    { return velocity; }

    if (magnitude(new_vel) > k_max_willed_speed)
        { return Velocity{normalize(new_vel)*(k_max_willed_speed*0.9995)}; }

    return Velocity{new_vel};
}

// time, and camera?


// more helpers... more to come likely

inline bool is_nl(char c) { return c == '\n'; }
inline bool is_ws(char c) { return c == ' ' || c == '\t'; }
inline bool is_sc(char c) { return c == ';'; }
inline bool is_cm(char c) { return c == ','; }
inline bool is_pe(char c) { return c == '|'; }

std::vector<Vertex> deduce_texture_positions(std::vector<Vertex> &, const std::vector<int> & elements);

// no go, method was poor design anyway
void test_slope_rotations();
void test_neighbor_map();

template <typename Vec, typename ... Types>
std::enable_if_t<cul::detail::k_are_vector_types<Vec, Types...>, Entity>
//  :eyes:
    make_bezier_strip_model
    (const Tuple<Vec, Types...> & lhs, const Tuple<Vec, Types...> & rhs,
     std::shared_ptr<Texture> texture, int resolution,
     Vector2 texture_offset, Real texture_scale)
{
    std::vector<Vertex> verticies;
    std::vector<int> elements;
    int el = 0;
    // uh oh, I need implementation knowledge here :c
    // so I'm trying to map texture positions correctly
    for (auto [a, b, c] : cul::make_bezier_strip(lhs, rhs, resolution).details_view()) {
        verticies.emplace_back(a.point(), texture_offset + Vector2{1.f*a.on_right(), a.position()}*texture_scale);
        verticies.emplace_back(b.point(), texture_offset + Vector2{1.f*b.on_right(), b.position()}*texture_scale);
        verticies.emplace_back(c.point(), texture_offset + Vector2{1.f*c.on_right(), c.position()}*texture_scale);

        elements.emplace_back(el++);
        elements.emplace_back(el++);
        elements.emplace_back(el++);
    }
    auto mod = RenderModel::make_opengl_instance();
    mod->load<int>(verticies, elements);

    auto ent = Entity::make_sceneless_entity();
    ent.add<
        SharedCPtr<RenderModel>, SharedPtr<Texture>,
        glm::mat4, VisibilityChain
    >() = std::make_tuple(
        std::move(mod), texture,
        identity_matrix<glm::mat4>(), VisibilityChain{}
    );
    return ent;
}

template <typename T>
Entity make_bezier_yring_model();

Entity make_sample_bezier_model(std::shared_ptr<Texture> texture, int resolution) {
    static constexpr const auto k_hump_side = std::make_tuple(
        Vector{ -.5, 0,  1},
        Vector{ -.5, 0,  1} + Vector{-0.5, 1, 0.5},
                //Vector{0, -1, 0},
        Vector{-1, 1, -1} + Vector{   0, 3,   0},
        Vector{-1, 1, -1} + Vector{0, 0, 0.2});
    static constexpr const auto k_low_side = std::make_tuple(
        Vector{0.5, 0,  1},
        Vector{0.5, 0,  1} + Vector{0.5, 1, -0.5},
                //Vector{0, -2, 0},
        Vector{1, 1, -1} + Vector{  0, 1,    0},
        Vector{1, 1, -1} + Vector{0, 0, 0.2});

    constexpr const Vector2 k_offset{5./16, 1./16};
    constexpr const Real k_scale = 1. / 16;

    Entity rv = make_bezier_strip_model(k_hump_side, k_low_side, texture, resolution, k_offset, k_scale);
    rv.add<Translation>() = Vector{ 4, 0, -3 };
    return rv;
}

Entity make_sample_loop(std::shared_ptr<Texture> texture, int resolution) {

    static constexpr const Real k_y = 10;

    static constexpr const Real k_s = 5;
    static constexpr const auto k_neg_side = std::make_tuple(
/*    */Vector{-1, 0,  0},
        Vector{-.5, 0, k_s}, Vector{-.5, k_y, k_s},
        Vector{-.5, k_y, -k_s}, Vector{-.5, 0, -k_s},
        Vector{0, 0, 0});
    static constexpr const auto k_pos_side = std::make_tuple(
/*    */Vector{0, 0,  0},
        Vector{.5, 0, k_s}, Vector{.5, k_y, k_s},
        Vector{.5, k_y, -k_s}, Vector{.5, 0, -k_s},
        Vector{1, 0, 0});
    constexpr const Vector2 k_offset{5./16, 1./16};
    constexpr const Real k_scale = 1. / 16;
    auto rv = make_bezier_strip_model(k_neg_side, k_pos_side, texture, resolution, k_offset, k_scale);
    rv.add<Translation, YRotation>() = std::make_tuple(Vector{4, 0, 0}, k_pi*0.5);
    return rv;
}

struct TranslationFromParent final {
    TranslationFromParent() {}
    TranslationFromParent(EntityRef par_, Vector trans_):
        parent(par_), translation(trans_) {}
    EntityRef parent;
    Vector translation;
};

static constexpr const Vector k_player_start{3.1, 2.1, -2.1};
static constexpr const Vector k_gravity = -k_up*10;
// model entity, physical entity
Tuple<Entity, Entity> make_sample_player(Platform::ForLoaders & callbacks) {
    static const auto get_vt = [](int i) {
        constexpr const Real    k_scale = 1. / 3.;
        constexpr const Vector2 k_offset = Vector2{0, 2}*k_scale;
        auto list = { k_offset,
                      k_offset + k_scale*Vector2(1, 0),
                      k_offset + k_scale*Vector2(0, 1),
                      k_offset + k_scale*Vector2(1, 1) };
        assert(i < int(list.size()));
        return *(list.begin() + i);
    };

    static const auto mk_v = [](double x, double y, double z, int vtidx) {
        Vertex v;
        v.position.x = x*0.5;
        v.position.y = y*0.5;
        v.position.z = z*0.5;
        v.texture_position = get_vt(vtidx);
        return v;
    };

    static constexpr const int k_tl = 0, k_tr = 1, k_bl = 2, k_br = 3;

    std::array verticies = {
        mk_v( 1, -1,  1, k_tl), // 0: tne
        mk_v(-1, -1,  1, k_tr), // 1: tnw
        mk_v(-1,  1,  1, k_bl), // 2: tsw
        mk_v( 1,  1,  1, k_br), // 3: tse
        mk_v(-1,  1, -1, 0), // 4: bsw
        mk_v( 1,  1, -1, 0), // 5: bse
        mk_v( 1, -1, -1, 0), // 6: bne
        mk_v(-1, -1, -1, 0)  // 7: bnw
    };

    std::array<unsigned, 3*2*6> elements = {
        0, 1, 2, /**/ 0, 2, 3, // top    faces
        0, 1, 7, /**/ 0, 6, 7, // north  faces
        2, 3, 4, /**/ 3, 4, 5, // south  faces
        1, 2, 7, /**/ 2, 7, 4, // west   faces
        0, 3, 6, /**/ 3, 5, 6, // east   faces
        4, 6, 7, /**/ 4, 5, 6  // bottom faces
    };

    auto model = RenderModel::make_opengl_instance();
    model->load(&verticies.front(), &verticies.front() + verticies.size(),
                &elements .front(), &elements .front() + elements.size());

    auto physics_ent = Entity::make_sceneless_entity();
    auto model_ent   = callbacks.make_renderable_entity();
#   if 1
    auto tx = Texture::make_opengl_instance();
    tx->load_from_file("ground.png");
    model_ent.add<
        SharedCPtr<Texture>, SharedCPtr<RenderModel>, Translation,
        TranslationFromParent
    >() = std::make_tuple(
        tx, model, Translation{},
        TranslationFromParent{EntityRef{physics_ent}, Vector{0, 0.5, 0}}
    );

    physics_ent.add<PpState>(PpInAir{k_player_start, Vector{}});
    physics_ent.add<Velocity, JumpVelocity, DragCamera, Camera, PlayerControl>();
#   endif
    return std::make_tuple(model_ent, physics_ent);
}

static constexpr auto k_layout =
    "xx   xx\n"
    "  avb  \n"
    "x >1<  \n"
    "  c^d x\n"
    "       \n"
    "x  1  x\n"
    "x1   1x\n";
static constexpr auto k_layout2 =
    "xxxxxxx\n"
    "xxxxxxx\n"
    "xxxxxxx\n"
    "xxx^xxx\n"
    "xxx xxx\n";
static constexpr auto k_layout3 =
    "xx   xx\n"
    "       \n"
    "x      \n"
    "      x\n"
    "       \n"
    "1     1\n"
    "x11111x\n";
static constexpr auto k_layout4 =
    "xx1111111111111111111111111111111111111111111111111111111111xx\n"
    "xx^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^xx\n"
    "1<                                                          >1\n"
    "1<                                                                                                                              >1\n"
    "1<                                                                                                                              >1\n"
    "1<                                                          >1xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxvvxx\n"
    "xxvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx11xx\n"
    "xx1111111111111111111111111111111111111111111111111111111111xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx11xx\n"
    "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx111111111111111111111111111111111111111111111111111111111111111111111xx\n";

ModelViewer::ModelViewer(const char * filename):
    m_reload_model([filename]() -> std::string {
        std::ifstream fin{filename};
        std::string temp_line;
        std::string rv;
        while (std::getline(fin, temp_line)) {
            rv += temp_line + '\n';
        }
        return rv;
    })
{}

void ModelViewer::setup() {
    test_neighbor_map();
    update_model();
}

void ModelViewer::render(ShaderProgram & shader) const {
#   if 0
    run_default_rendering_systems(m_scene, shader);
#   endif
}

void ModelViewer::update(Real et) {
    m_time_since_reload += et;
    if (m_time_since_reload > k_time_to_reload) {
        m_time_since_reload = 0;
        update_model();
    }
    TimeWatchingDriver::update(et);
}

Camera ModelViewer::camera() const {
    Camera rv;
    rv.up = k_up;
    rv.position = m_camera_target + Vector{0, 1, 0}
            + 3.*Vector{std::cos(time_since()), 0, std::sin(time_since())};
    rv.target = m_camera_target;
    return rv;
}

/* private */ void ModelViewer::update_model() {
    auto newcontents = m_reload_model();
    if (newcontents == m_filecontents) return;
    m_filecontents = newcontents;
    try {
        auto e = load_model_from_string(newcontents);
        e.add<glm::mat4>();
        m_scene.clear();
        m_scene.add_entity(e);
        m_scene.update_entities();
    } catch (std::runtime_error & err) {
        std::cout << err.what() << std::endl;
    }
}

/* private static */ Entity ModelViewer::load_model_from_string
    (const std::string & filecontents)
{
    static const std::string k_modeselection = "modeselection";
    using StrItr = std::string::const_iterator;
    using ProcLineFunc = std::string(*)(Entity & e, StrItr beg, StrItr end);
    using std::make_pair;
    static const auto is_end = [](StrItr beg, StrItr end) {
        return std::equal(beg, end, "end");
    };
    using namespace cul::exceptions_abbr;
    static const std::unordered_map<std::string, ProcLineFunc> line_fmap {
        make_pair("units", [](Entity & e, StrItr beg, StrItr end) {
            if (is_end(beg, end))
                { return k_modeselection; }

            Units uns;
            auto fields = { &uns.verticies, &uns.texture };
            auto fitr = fields.begin();
            cul::for_split<is_ws>(beg, end, [&fitr, fields] (StrItr beg, StrItr end) {
                if (fitr == fields.end()) {
                    throw RtError{"Only two units"};
                }
                if (!cul::string_to_number(beg, end, **fitr)) {
                    throw RtError{"All units must be numeric"};
                }
                ++fitr;
                return cul::fc_signal::k_continue;
            });
            e.add<Units>() = uns;
            return std::string{};
        }),
        make_pair("elements", [](Entity & e, StrItr beg, StrItr end) {
            if (is_end(beg, end))
                { return k_modeselection; }

            std::vector<int> elements;
            if (auto * els = e.ptr<Elements>()) {
                elements = std::move(els->values);
            }
            cul::for_split<is_ws>(beg, end, [&elements](StrItr beg, StrItr end) {
                int elval = 0;
                if (!cul::string_to_number(beg, end, elval)) {
                    throw RtError{"All elements must be numeric"};
                }
                elements.push_back(elval);
                return cul::fc_signal::k_continue;
            });
            e.ensure<Elements>().values = std::move(elements);
            return std::string{};
        }),
        make_pair("verticies", [](Entity & e, StrItr beg, StrItr end) {
            if (is_end(beg, end))
                { return k_modeselection; }
            std::vector<Vertex> verticies;
            if (auto * verts = e.ptr<Verticies>())
                { verticies = std::move(verts->values); }
            // this is going to be mega complicated...
            // I want texture positions to be deducible!
            // constants? fractions?
            // other constraints?
            // do we want comments?
            // 0, 0.1, 0.3 | 0.1, 0.4 ; <next vertex>
            cul::for_split<is_sc>(beg, end, [&verticies](StrItr beg, StrItr end) {
                using namespace cul::fc_signal;
                Vertex new_vertex{ Vector{}, Vector2{k_inf, k_inf} };
                int pipe_mode = 0;
                cul::for_split<is_pe>(beg, end, [&](StrItr beg, StrItr end) {
                    if (pipe_mode == 2) {
                        throw RtError{"Only vertex and texture position."};
                    }
                    std::array pos = { &new_vertex.position.x, &new_vertex.position.y, &new_vertex.position.z };
                    std::array tx  = { &new_vertex.texture_position.x, &new_vertex.texture_position.y };
                    Real ** ritr = pipe_mode == 0 ? &pos[0] : &tx[0];
                    Real ** rend = ritr + (pipe_mode == 0 ? pos.size() : tx.size());
                    cul::for_split<is_cm>(beg, end, [&](StrItr beg, StrItr end) {
                        cul::trim<is_ws>(beg, end);
                        if (ritr == rend) {
                            throw RtError{"too many arguments"};
                        }
                        if (!cul::string_to_number(beg, end, **ritr)) {
                            throw RtError{"not numeric"};
                        }
                        ++ritr;
                        return k_continue;
                    });
                    if (pipe_mode == 0 && ritr != rend) {
                        throw RtError{"must specify all three positions"};
                    }
                    ++pipe_mode;
                    return k_continue;
                });
                verticies.push_back(new_vertex);
                return k_continue;
            });
            e.ensure<Verticies>().values = std::move(verticies);
            return std::string{};
        }),
        make_pair("texture", [](Entity & e, StrItr beg, StrItr end) {
            if (is_end(beg, end))
                { return k_modeselection; }
            // filename, that's it!
            // maybe we can use a map for textures too? :3?
            auto tx = Texture::make_opengl_instance();
            tx->load_from_file(std::string{beg, end}.c_str());
            e.add<SharedCPtr<Texture>>() = tx;

            return std::string{};
        }),
        make_pair("yrotation", [](Entity & e, StrItr beg, StrItr end) {
            if (is_end(beg, end))
                { return k_modeselection; }
            // one radian, that's it!
            Real rotation = 0.;
            if (!cul::string_to_number(beg, end, rotation)) {
                throw RtError{"rotation must be numeric"};
            }
            e.add<YRotation>() = rotation;
            return std::string{};
        }),
        make_pair(k_modeselection, [](Entity&, StrItr beg, StrItr end)
            { return std::string{beg, end}; })
    };
    auto e = Entity::make_sceneless_entity();
    assert(line_fmap.find(k_modeselection) != line_fmap.end());
    ProcLineFunc func = line_fmap.find(k_modeselection)->second;
    cul::for_split<is_nl>(filecontents, [&e, &func](StrItr beg, StrItr end) {
        end = std::find(beg, end, '#');
        cul::trim<is_ws>(beg, end);
        if (beg == end) return cul::fc_signal::k_continue;

        auto nextmode = func(e, beg, end);

        if (nextmode.empty()) return cul::fc_signal::k_continue;
        auto nitr = line_fmap.find(nextmode);
        if (nitr == line_fmap.end()) {
            throw RtError{"Cannot find next mode by string \"" + nextmode + "\"."};
        }
        func = nitr->second;
        return cul::fc_signal::k_continue;
    });
    // must verify elements and verticies,
    if (!e.has_all<Elements, Verticies>()) {
        throw RtError{"Must have at least elements and verticies"};
    }
    if (e.get<Elements>().values.size() % 3 != 0) {
        throw RtError{"Number of elements must be divisible by three"};
    }
    {
    const auto & evals = e.get<Elements>().values;
    int vert_count = e.get<Verticies>().values.size();
    bool elements_are_ok = std::all_of(evals.begin(), evals.end(), [vert_count](int idx) { return idx < vert_count && idx > -1; });
    if (!elements_are_ok) {
        throw RtError{"Each element must correspond to a vertex"};
    }
    }
    if (auto * uns = e.ptr<Units>()) {
        auto & verts = e.get<Verticies>().values;
        for (auto & vtx : verts) {
            vtx.position.x /= Real(uns->verticies);
            vtx.position.y /= Real(uns->verticies);
            vtx.position.z /= Real(uns->verticies);

            vtx.position -= Vector{0.5, 0.5, 0.5};

            vtx.texture_position.x /= Real(uns->texture);
            vtx.texture_position.y /= Real(uns->texture);
        }
    }
    // do texture position deduction,
    e.get<Verticies>().values = deduce_texture_positions(e.get<Verticies>().values, e.get<Elements>().values);
    // then load a render model
    auto model_ptr = RenderModel::make_opengl_instance();
    model_ptr->load<int>(e.get<Verticies>().values, e.get<Elements>().values);
    e.add<SharedCPtr<RenderModel>>() = model_ptr;

    return e;
}

// ----------------------------------------------------------------------------

void GameDriverCompleteN::press_key(KeyControl ky) {
    m_player.get<PlayerControl>().press(ky);
    m_time_controller.press(ky);
    if (ky == KeyControl::restart) {
        //setup();
    }
}

void GameDriverCompleteN::release_key(KeyControl ky) {
    m_player.get<PlayerControl>().release(ky);
    m_time_controller.release(ky);
}

Loader::LoaderTuple GameDriverCompleteN::initial_load
    (Platform::ForLoaders & callbacks)
{
    auto tgg_ptr = TileGraphicGenerator::make_builtin(callbacks);
    auto [tlinks, ents] = load_map_graphics(*tgg_ptr, load_map_cell(k_layout4, CharToCell::default_instance())); {}
    auto [renderable, physical] = make_sample_player(callbacks); {}
    callbacks.set_camera_entity(EntityRef{physical});

    // v kinda icky v
    m_player = physical;
    m_ppdriver->clear_all_triangles();
    m_ppdriver->add_triangles(tlinks);

    return std::make_tuple(PlayerEntities{physical, renderable}, ents, SingleSysVec{}, TriggerSysVec{});
}

void GameDriverCompleteN::update_(Real seconds) {
    if (!m_time_controller.runs_this_frame()) {
        m_time_controller.frame_update();
        return;
    }

    //TimeWatchingDriver::update(seconds);
    m_ppdriver->update();
    using std::get_if;
#   if 1
    ecs::make_singles_system<Entity>([seconds](VisibilityChain & vis) {
        if (!vis.visible || !vis.next) return;
        if ((vis.time_spent += seconds) > VisibilityChain::k_to_next) {
            vis.time_spent = 0;
            vis.visible = false;
            Entity{vis.next}.get<VisibilityChain>().visible = true;
        }
    },
#   if 1
    [](TranslationFromParent & trans_from_parent, Translation & trans) {
        auto pent = Entity{trans_from_parent.parent};
        Real s = 1;
        auto & state = pent.get<PpState>();
        if (auto * on_surf = std::get_if<PpOnSurface>(&state)) {
            s *= on_surf->invert_normal ? -1 : 1;
            s *= (angle_between(on_surf->segment->normal(), k_up) > k_pi*0.5) ? -1 : 1;
        }
        trans = location_of(state) + s*trans_from_parent.translation;
    },
#   endif
    [seconds] (PpState & state, Velocity & velocity, PlayerControl & control,
               Camera & camera)
    {
        if (are_very_close(control.heading(), Vector2{})) return;
        Vector player_loc = [state] {
            if (auto * on_surf = get_if<PpOnSurface>(&state)) {
                return on_surf->segment->point_at(on_surf->location);
            }
            auto * in_air = get_if<PpInAir>(&state);
            assert(in_air);
            return in_air->location;
        } ();
        Vector forward = normalize(cul::project_onto_plane(player_loc - camera.position, k_up));
        Vector left    = normalize(cross(k_up, forward));
        // +y is forward, +x is right
        //constexpr Real k_speed = 1;
        auto willed_dir = normalize(control.heading().y*forward - control.heading().x*left);

        velocity = get_new_velocity(velocity, willed_dir, seconds); //velocity.value + willed_dir*k_speed*seconds;

    }, [seconds](PpState & state, Velocity & velocity, Opt<JumpVelocity> jumpvel) {
        if (auto * in_air = std::get_if<point_and_plane::InAir>(&state)) {
            velocity = velocity.value + k_gravity*seconds;
            if (jumpvel) {
                *jumpvel = jumpvel->value + k_gravity*seconds;
            }
        } else if(auto * on_segment = std::get_if<point_and_plane::OnSurface>(&state)) {
#           if 0
            auto & triangle = *on_segment->segment;
            velocity = velocity.value + cul::project_onto_plane(k_gravity*seconds, triangle.normal());
#           endif
        }
    }, [seconds](PpState & state, Velocity & velocity, Opt<JumpVelocity> jumpvel) {
        auto displacement = velocity.value*seconds + (1. / 2)*k_gravity*seconds*seconds;
        if (auto * in_air = std::get_if<point_and_plane::InAir>(&state)) {
            in_air->displacement = displacement + (jumpvel ? jumpvel->value*seconds : Vector{});
        } else if(auto * on_segment = std::get_if<point_and_plane::OnSurface>(&state)) {
            auto & triangle = *on_segment->segment;
            displacement = cul::project_onto_plane(displacement, triangle.normal());
            auto pt_v3 = triangle.point_at(on_segment->location);
            auto new_pos = triangle.closest_point(pt_v3 + displacement);
            on_segment->displacement = new_pos - on_segment->location;
        }
    }, [this](PpState & state, Opt<Velocity> vel) {
        using Triangle = point_and_plane::Triangle;
        class Impl final : public point_and_plane::EventHandler {
        public:
            Impl(Opt<Velocity> & vel): m_vel(vel) {}
            Variant<Vector2, Vector> displacement_after_triangle_hit
                (const Triangle & triangle, const Vector & /*location*/,
                 const Vector & new_, const Vector & intersection) const final
            {
                // for starters:
                // always attach, entirely consume displacement
                if (m_vel) {
                    *m_vel = project_onto_plane(m_vel->value, triangle.normal());
                }
                auto diff = new_ - intersection;
                auto rem_displc = project_onto_plane(diff, triangle.normal());

                auto rv =  triangle.closest_point(rem_displc + intersection)
                         - triangle.closest_point(intersection);
                return rv;
            }

            Variant<SegmentTransfer, Vector> pass_triangle_side
                (const Triangle &, const Triangle * to,
                 const Vector &, const Vector &) const final
            {
                // always transfer to the other surface
                if (!to) {
                    return std::make_tuple(false, Vector2{});
                    return Vector{};
                }
                // may I know the previous inversion value?
                return std::make_tuple(false, Vector2{});
            }

            bool cling_to_edge(const Triangle & triangle, TriangleSide side) const final {
                if (!m_vel) return true;
                auto [sa, sb] = triangle.side_points(side); {}
                *m_vel = project_onto(m_vel->value, sa - sb);
                return true;
            }

            Opt<Velocity> & m_vel;
        };
        Impl impl{vel};
        state = (*m_ppdriver)(state, impl);
    }, [](point_and_plane::State & state, PlayerControl & control, JumpVelocity & vel) {
        constexpr auto k_jump_vel = k_up*10;
        {
        auto * on_segment = std::get_if<point_and_plane::OnSurface>(&state);
        if (on_segment && control.is_starting_jump()) {
            auto & triangle = *on_segment->segment;
            auto dir = (on_segment->invert_normal ? -1 : 1)*triangle.normal()*0.1;
            vel = k_jump_vel;
            state = point_and_plane::InAir{triangle.point_at(on_segment->location) + dir, Vector{}};
        }
        }
        {
        auto * in_air = std::get_if<point_and_plane::InAir>(&state);
        if (in_air && control.is_ending_jump() && !are_very_close(vel.value, Vector{})) {
            vel = normalize(vel.value)*std::sqrt(magnitude(vel.value));
        }
        }
    })(scene());
#   if 1
    for (auto e : scene()) {
        auto * on_surf = get_if<PpOnSurface>(e.ptr<PpState>());
        if (!on_surf) continue;

    }
#   endif
    // this code here is a good candidate for a "Trigger" system
    m_player.get<PlayerControl>().frame_update();

    auto pos = location_of(m_player.get<PpState>()) + Vector{0, 3, 0};
    auto & cam = m_player.get<DragCamera>();
    if (magnitude(cam.position - pos) > cam.max_distance) {
        cam.position += normalize(pos - cam.position)*(magnitude(cam.position - pos) - cam.max_distance);
        assert(are_very_close( magnitude( cam.position - pos ), cam.max_distance ));
    }
    m_player.get<Camera>().target = location_of(m_player.get<PpState>());
    m_player.get<Camera>().position = cam.position;
    //m_camera_target = cam.position;

#   endif
    m_time_controller.frame_update();

}
#if 0
/* private */ BuiltinMapLoader::PlayerEntities
    GameDriverCompleteN::InitLoader::make_player() const
{
    auto [ren, phys] = make_sample_player();
    return PlayerEntities{phys, ren};
}
#endif
// ----------------------------------------------------------------------------

// I need elements to figure out how verticies are linked...
// these should probably be tested
// what if known verticies always froms a rectangle on the texture itself?
// (even if not so for the model?)
// project model verticies onto the texture?
//
// I need a map between an element and it's immediate neighbors
class NeighborMap final {
public:
    using Itr = std::vector<int>::const_iterator;
    using IdxView = cul::View<Itr>;

    NeighborMap() {}

    explicit NeighborMap(const std::vector<int> & elements)
        { setup(elements); }

    void setup(const std::vector<int> & elements) {
        setup_new(elements);
    }

    void setup_new(const std::vector<int> & elements) {
        assert(elements.size() % 3 == 0);
        int max_idx = *std::max_element(elements.begin(), elements.end());
        // verify...
        // all elements must be 0...n, with any number of gaps

        using IntPair = std::pair<int, int>;
        std::vector<IntPair> neighbor_pairs;
        for (auto itr = elements.begin(); itr != elements.end(); itr += 3) {
            // should I add every possible pair? rather than just two?
            // whatever makes this easier to write
            auto add = [&neighbor_pairs] (int a, int b)
                { neighbor_pairs.emplace_back(a, b); };
            add( itr[0], itr[1] );
            add( itr[0], itr[2] );
            add( itr[1], itr[0] );
            add( itr[1], itr[2] );
            add( itr[2], itr[0] );
            add( itr[2], itr[1] );
        }
        // is this strict weak ordering?
        std::sort(neighbor_pairs.begin(), neighbor_pairs.end(),
                  [](const IntPair & lhs, const IntPair & rhs)
        {
            if (lhs.first == rhs.first) return lhs.second < rhs.second;
            return lhs.first < rhs.first;
        });
        // must remove (now) sequential dupelicates
        neighbor_pairs.erase(
            std::unique(neighbor_pairs.begin(), neighbor_pairs.end()),
            neighbor_pairs.end());

        m_indicies.reserve(neighbor_pairs.size());
        for (auto & pair : neighbor_pairs) {
            m_indicies.push_back(pair.second);
        }
        assert(m_indicies.size() == neighbor_pairs.size());

        m_index_map.reserve(max_idx);
        auto push_empty_range = [this] () {
            const auto end = m_indicies.end();
            m_index_map.emplace_back( end, end );
        };
        int idx = 0;
        for (auto prs_itr = neighbor_pairs.begin(); prs_itr != neighbor_pairs.end(); ) {
            for (; idx < prs_itr->first; ++idx) {
                push_empty_range();
            }
            auto next = std::find_if(prs_itr, neighbor_pairs.end(),
                [prs_itr](const IntPair & pair)
                { return pair.first > prs_itr->first; });
            m_index_map.emplace_back(
                m_indicies.begin() + (prs_itr - neighbor_pairs.begin()),
                m_indicies.begin() + (next    - neighbor_pairs.begin()));
            ++idx;
            prs_itr = next;
        }
        for (; idx != max_idx + 1; ++idx) {
            push_empty_range();
        }
    }

    void setup_old(const std::vector<int> & elements) {
        assert(elements.size() % 3 == 0);
        // maybe I'll just do pairs and sort :/

        // in each set of three, each other element is a neighbor
        std::vector<std::size_t> idx_sizes;
        int max_idx = *std::max_element(elements.begin(), elements.end());
        idx_sizes.reserve(max_idx);
        // on "remove as you go" too complicated for a first implementation
        for (int idx = 0; idx != max_idx; ++idx) {
            for (auto itr = elements.begin(); itr != elements.end(); itr += 3) {
                // skip if none are possible neighbors of idx
                if (!std::any_of(itr, itr + 3, [idx](int n) { return n == idx; }))
                { continue; }

                for (auto jtr = itr; jtr != itr + 3; ++jtr) {
                    if (*jtr == idx) continue;
                    // add if not idx and a possible neighbor
                    m_indicies.push_back(*jtr);
                }
            }
            // finished with idx
            idx_sizes.push_back(m_indicies.size() - 1);
        }


        //auto itr = m_indicies.begin();
        std::size_t last_size = 0;
        for (auto size : idx_sizes) {
            assert(size < m_indicies.size());
            m_index_map.emplace_back( m_indicies.begin() + last_size, m_indicies.begin() + size );
            last_size = size;
            // make sure itr does not overrun m_indicies
            //assert((itr - m_indicies.begin()) + size < m_indicies.size());
            //itr += size;
        }
    }

    IdxView operator () (int idx) const
        { return m_index_map.at(idx); }

private:
    std::vector<int> m_indicies;
    std::vector<IdxView> m_index_map;
};

class UnknownTxNodes final {
public:
    UnknownTxNodes() {}

    explicit UnknownTxNodes(const std::vector<Vertex> & verticies)
        { setup(verticies); }

    void setup(const std::vector<Vertex> & verticies) {
        for (const auto & vertx : verticies) {
            if (!cul::is_real(vertx.texture_position)) continue;
            m_knowns.insert(std::make_pair(
                &vertx - &verticies.front(),
                Known{ int(m_knowns.size()), vertx.texture_position }
            ));
        }
        m_distances_for_unknowns.resize(
            (verticies.size() - m_knowns.size())*m_knowns.size());
        {
        auto dend = m_distances_for_unknowns.end();
        m_unknown_to_distances.resize(
            verticies.size(), InfoView{ dend, dend });
        }
        auto itr = m_distances_for_unknowns.begin();
        for (const auto & vertx : verticies) {
            if (cul::is_real(vertx.texture_position)) continue;
            m_unknown_to_distances[&vertx - &verticies.front()] =
                InfoView{itr, itr + m_knowns.size()};
            assert(  std::size_t(itr - m_distances_for_unknowns.begin())
                   < m_knowns.size() + m_distances_for_unknowns.size());
            itr += m_knowns.size();
        }
    }

    // for each unknown:
    // d_avg*v_known + ...
    void set_distance(int node, int known, Real dist)
        { get_ref_(node, known).distance = dist; }

    Real distance(int node, int known) const
        { return get_ref_(node, known).distance; }

    void set_count(int node, int known, int count)
        { get_ref_(node, known).count = count; }

    int count(int node, int known) const
        { return get_ref_(node, known).count; }

    // compute unknowns, computationally heavy
    // linearly interpolate texture positions for all unknowns, values for all
    // knowns retained
    void compute_all_unknowns(std::vector<Vertex> & verticies) const {
        using namespace cul::exceptions_abbr;
        if (m_unknown_to_distances.size() != verticies.size()) {
            throw InvArg("UnknownTxNodes::operator(): number of verticies do "
                         "not match between call and setup (are they the same "
                         "verticies?).");
        }
        for (auto & vtx : verticies) {
            if (cul::is_real(vtx.texture_position)) continue;
            auto distances = m_unknown_to_distances[&vtx - &verticies.front()];
            Real total_distance = 0;
            Vector2 tx;
            for (auto itr = m_knowns.begin(); itr != m_knowns.end(); ++itr) {
                auto dist = ( distances.begin() + itr->second.idx_in_group )->distance;
                tx += itr->second.value*dist;
                total_distance += dist;
            }
            vtx.texture_position = tx / total_distance;
        }
    }

    struct Known final {
        Known() {}
        Known(int idx_, Vector2 r_): idx_in_group(idx_), value(r_) {}
        int idx_in_group = 0;
        Vector2 value;
    };

    class KnownsItr final {
    public:
        explicit KnownsItr(std::map<int, Known>::const_iterator itr):
            m_back_itr(itr) {}

        int operator * () const { return m_back_itr->first; }

        KnownsItr & operator ++ (int) {
            ++m_back_itr;
            return *this;
        }

        KnownsItr operator ++ () {
            KnownsItr rv{m_back_itr};
            ++m_back_itr;
            return rv;
        }

        bool operator == (const KnownsItr & rhs) const noexcept
            { return rhs.m_back_itr == m_back_itr; }

        bool operator != (const KnownsItr & rhs) const noexcept
            { return rhs.m_back_itr != m_back_itr; }

    private:
        std::map<int, Known>::const_iterator m_back_itr;
    };

    auto knowns() const noexcept
        { return cul::View<KnownsItr>{ KnownsItr{m_knowns.begin()}, KnownsItr{m_knowns.end()} }; }

    bool is_known(int n) const noexcept
        { return m_knowns.find(n) != m_knowns.end(); }

private:
    struct UnknownKnownInfo final {
        Real distance = 0;
        int count = 0;
    };

    UnknownKnownInfo & get_ref_(int node, int known) const {
        using namespace cul::exceptions_abbr;
        auto itr = m_knowns.find(known);
        auto view = m_unknown_to_distances.at(node);
        if (itr == m_knowns.end()) {
            throw InvArg("UnknownTxNodes::add_distance: element "
                         + std::to_string(known) + " is not a known node.");
        }
        if (view.end() == view.begin()) {
            throw InvArg("UnknownTxNodes::add_distance: element "
                         + std::to_string(node) + " is not an unknown node.");
        }
        auto & known_info = itr->second;
        assert(view.end() - view.begin() == int(m_knowns.size()));
        assert(known_info.idx_in_group < int(m_knowns.size()));
        // averaging?!
        return *(view.begin() + known_info.idx_in_group);
    }

    std::map<int, Known> m_knowns;
    // #unknowns*#knowns
    std::vector<UnknownKnownInfo> m_distances_for_unknowns;
    using InfoView = cul::View<std::vector<UnknownKnownInfo>::iterator>;
    std::vector<InfoView> m_unknown_to_distances;
};

// build three things, frontier, interior unknowns, exterior unknowns
//
// turn frontier into knowns, then use that frontier as knowns in a recursive
// call, no exterior nodes as a base case
//
// this function should probably be tested as well...
std::vector<Vertex> deduce_texture_positions
    (std::vector<Vertex> & verticies, const std::vector<int> & elements)
{
    bool all_known = std::all_of(
        verticies.begin(), verticies.end(),
        [](const Vertex & vtx) { return cul::is_real(vtx.texture_position); });
    if (all_known) return std::move(verticies);

    // set of known and unknown texture positions
    // there must be at least three!
    // for the unknown compute distances to the three other verticies
    NeighborMap nmap{elements};
    UnknownTxNodes unknowns{verticies};
    auto knowns = unknowns.knowns();
    std::vector<int> exploring;
    for (int n : knowns) {
        for (int neighbor : nmap(n)) {
            if (unknowns.is_known(n)) continue;
            exploring.push_back(neighbor);
        }
    }
    auto distance_between = [&verticies] (int a, int b)
        { return magnitude( verticies[a].position - verticies[b].position ); };

    std::vector<int> next_exploring;
    do {
        // distance averages?
        for (int n : exploring) {
            // for each neighbor
            for (int known : knowns) {
                int count = 0;
                Real distance = 0;
                // for each known in each neighbor
                // is there a way to build an iterator to simplify this?
                for (int neighbor : nmap(n)) {
                    // if neighbor is *that* known
                    auto distance_for_neighbor = distance_between(n, neighbor);
                    if (neighbor == known) {
                        ++count;
                        distance += distance_for_neighbor;
                        continue;
                    }

                    // if count is 0, then it's "unprocessed"
                    if (unknowns.count(neighbor, known) == 0) {
                        next_exploring.push_back(neighbor);
                        continue;
                    }
                    distance += distance_for_neighbor + unknowns.distance(neighbor, known);
                    ++count;
                }
                assert(count);
                unknowns.set_count(n, known, count);
                unknowns.set_distance(n, known, distance / count);
            }
        }
        exploring.swap(next_exploring);
        next_exploring.clear();
    } while (!exploring.empty());
    // following, no "unknowns" should remain (though not "complete") how ought
    // I verify this?
    assert(([&verticies, &unknowns] {
        auto knowns = unknowns.knowns();
        for (int i = 0; i != int(verticies.size()); ++i) {
            if (unknowns.is_known(i)) continue;
            for (int n : knowns) {
                if (unknowns.count(i, n) == 0)
                    { return false; }
            }
        }
        return true;
    } ()));
    unknowns.compute_all_unknowns(verticies);
    return std::move(verticies);
}

void test_slope_rotations() {
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    using namespace cul::ts;
    using cul::is_real;
    using Tgg = TileGraphicGenerator;
    TestSuite suite;
    suite.start_series("slope rotation detection");
    mark(suite).test([] {
        return test(is_real(Tgg::rotation_between(
            Slopes{0, 0, 0, 1, 1},
            Slopes{0, 0, 1, 1, 0}
        )));
    });
    mark(suite).test([] {
        return test(is_real(Tgg::rotation_between(
            Slopes{0, 1, 0, 0, 1},
            Slopes{0, 0, 1, 1, 0}
        )));
    });
}

void test_neighbor_map() {
    using namespace cul::ts;
    TestSuite suite;
    suite.start_series("Neighbor Map");
    mark(suite).test([] {
        NeighborMap nmap({
            0, 1, 2,
            0, 2, 3
        });
        std::array zero_neighbors = { 1, 2, 3 };
        return test(std::equal(
            zero_neighbors.begin(), zero_neighbors.end(),
            nmap(0).begin(), nmap(0).end()));
    });
    // test something who doesn't have all others as neighbors
    mark(suite).test([] {
        NeighborMap nmap({
            0, 1, 2,
            0, 2, 3,
            2, 3, 4
        });
        std::array four_neighbors = { 2, 3 };
        auto gn = nmap(4);
        return test(std::equal(
            four_neighbors.begin(), four_neighbors.end(),
            gn.begin(), gn.end()));
    });

    mark(suite).test([] {
        NeighborMap nmap({
            0, 1, 2,
            0, 2, 3,
            2, 3, 4
        });

        auto gn = nmap(4);
        return test(std::none_of(
            gn.begin(), gn.end(), [](int n) { return n == 0; }));
    });
#   undef mark
}

} // end of <anonymous> namespace
