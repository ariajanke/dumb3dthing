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

#include <common/BezierCurves.hpp>

#include <iostream>

namespace {

using std::get_if, std::make_tuple;

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

class GameDriverComplete final : public Driver {
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

} // end of <anonymous> namespace

/* static */ UniquePtr<Driver> Driver::make_instance()
    { return std::make_unique<GameDriverComplete>(); }

void Driver::update(Real seconds, Platform::Callbacks & callbacks) {
    update_(seconds);
    for (auto & single : m_singles) {
        (*single)(m_scene);
    }

    UniquePtr<Loader> loader;
    for (auto & trigger : m_triggers) {
        auto uptr = (*trigger)(m_scene);
        if (!uptr) continue;
        if (loader) {
            // emit warning
        } else {
            loader = std::move(uptr);
        }
    }
    if (loader) {
        if (loader->reset_dynamic_systems()) {
            m_singles.clear();
            m_triggers.clear();
        }

        // wipe entities requesting deletion from triggers before loads
        m_scene.update_entities();
        handle_loader_tuple((*loader)(m_player_entities, callbacks));
    }
    m_scene.update_entities();
    callbacks.render_scene(m_scene);
}

/* private */ void Driver::handle_loader_tuple(Loader::LoaderTuple && tup) {
    using std::get;
    auto player_ents = get<PlayerEntities>(tup);
    auto & ents = get<EntityVec>(tup);
    auto & singles = get<SingleSysVec>(tup);
    auto & triggers = get<TriggerSysVec>(tup);
    auto optionally_replace = [this](Entity & e, Entity new_) {
        if (!new_) return;
        if (e) e.request_deletion();
        e = new_;

        m_scene.add_entity(e);
    };
    optionally_replace(m_player_entities.physical, player_ents.physical);
    optionally_replace(m_player_entities.renderable, player_ents.renderable);

    m_scene.add_entities(ents);
    for (auto & single : singles) {
        m_singles.emplace_back(std::move(single));
    }
    for (auto & trigger : triggers) {
        m_triggers.emplace_back(std::move(trigger));
    }
}

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
    constexpr const Real k_max_acc = 10; // u/s^2
    constexpr const Real k_min_acc = 2;
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

template <typename Vec, typename ... Types>
std::enable_if_t<cul::detail::k_are_vector_types<Vec, Types...>, Entity>
//  :eyes:
    make_bezier_strip_model
    (const Tuple<Vec, Types...> & lhs, const Tuple<Vec, Types...> & rhs,
     Platform::Callbacks & callbacks,
     SharedPtr<Texture> texture, int resolution,
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
    auto mod = callbacks.make_render_model();
    mod->load<int>(verticies, elements);

    auto ent = callbacks.make_renderable_entity();
    ent.add<
        SharedCPtr<RenderModel>, SharedPtr<Texture>, VisibilityChain
    >() = make_tuple(
        std::move(mod), texture, VisibilityChain{}
    );
    return ent;
}

template <typename T>
Entity make_bezier_yring_model();

Entity make_sample_bezier_model(Platform::Callbacks & callbacks, SharedPtr<Texture> texture, int resolution) {
    static constexpr const auto k_hump_side = make_tuple(
        Vector{ -.5, 0,  1},
        Vector{ -.5, 0,  1} + Vector{-0.5, 1, 0.5},
                //Vector{0, -1, 0},
        Vector{-1, 1, -1} + Vector{   0, 3,   0},
        Vector{-1, 1, -1} + Vector{0, 0, 0.2});
    static constexpr const auto k_low_side = make_tuple(
        Vector{0.5, 0,  1},
        Vector{0.5, 0,  1} + Vector{0.5, 1, -0.5},
                //Vector{0, -2, 0},
        Vector{1, 1, -1} + Vector{  0, 1,    0},
        Vector{1, 1, -1} + Vector{0, 0, 0.2});

    constexpr const Vector2 k_offset{5./16, 1./16};
    constexpr const Real k_scale = 1. / 16;

    // very airy
    Entity rv = make_bezier_strip_model(k_hump_side, k_low_side, callbacks, texture, resolution, k_offset, k_scale);
    rv.add<Translation>() = Vector{ 4, 0, -3 };
    return rv;
}

Entity make_sample_loop(Platform::Callbacks & callbacks, SharedPtr<Texture> texture, int resolution) {

    static constexpr const Real k_y = 10;

    static constexpr const Real k_s = 5;
    static constexpr const auto k_neg_side = make_tuple(
/*    */Vector{-1, 0,  0},
        Vector{-.5, 0, k_s}, Vector{-.5, k_y, k_s},
        Vector{-.5, k_y, -k_s}, Vector{-.5, 0, -k_s},
        Vector{0, 0, 0});
    static constexpr const auto k_pos_side = make_tuple(
/*    */Vector{0, 0,  0},
        Vector{.5, 0, k_s}, Vector{.5, k_y, k_s},
        Vector{.5, k_y, -k_s}, Vector{.5, 0, -k_s},
        Vector{1, 0, 0});
    constexpr const Vector2 k_offset{5./16, 1./16};
    constexpr const Real k_scale = 1. / 16;
    auto rv = make_bezier_strip_model(k_neg_side, k_pos_side, callbacks, texture, resolution, k_offset, k_scale);
    rv.add<Translation, YRotation>() = make_tuple(Vector{4, 0, 0}, k_pi*0.5);
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

    auto model = callbacks.make_render_model();
    model->load(&verticies.front(), &verticies.front() + verticies.size(),
                &elements .front(), &elements .front() + elements.size());

    auto physics_ent = Entity::make_sceneless_entity();
    auto model_ent   = callbacks.make_renderable_entity();
#   if 1
    auto tx = callbacks.make_texture();
    tx->load_from_file("ground.png");
    model_ent.add<
        SharedCPtr<Texture>, SharedCPtr<RenderModel>, Translation,
        TranslationFromParent
    >() = make_tuple(
        tx, model, Translation{},
        TranslationFromParent{EntityRef{physics_ent}, Vector{0, 0.5, 0}}
    );

    physics_ent.add<PpState>(PpInAir{k_player_start, Vector{}});
    physics_ent.add<Velocity, JumpVelocity, DragCamera, Camera, PlayerControl>();
#   endif
    return make_tuple(model_ent, physics_ent);
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

// ----------------------------------------------------------------------------

void GameDriverComplete::press_key(KeyControl ky) {
    m_player.get<PlayerControl>().press(ky);
    m_time_controller.press(ky);
    if (ky == KeyControl::restart) {
        //setup();
    }
}

void GameDriverComplete::release_key(KeyControl ky) {
    m_player.get<PlayerControl>().release(ky);
    m_time_controller.release(ky);
}

Loader::LoaderTuple GameDriverComplete::initial_load
    (Platform::ForLoaders & callbacks)
{
    std::vector<Entity> entities;
    std::vector<SharedPtr<TriangleSegment>> triangles;

    auto tgg_ptr = TileGraphicGenerator{entities, triangles, callbacks};
    tgg_ptr.setup();
    auto [tlinks, ents] = load_map_graphics(tgg_ptr, load_map_cell(k_layout4, CharToCell::default_instance())); {}
    auto [renderable, physical] = make_sample_player(callbacks); {}
    callbacks.set_camera_entity(EntityRef{physical});

    // v kinda icky v
    m_player = physical;
    m_ppdriver->clear_all_triangles();
    m_ppdriver->add_triangles(tlinks);

    return make_tuple(PlayerEntities{physical, renderable}, ents, SingleSysVec{}, TriggerSysVec{});
}

void GameDriverComplete::update_(Real seconds) {
    if (!m_time_controller.runs_this_frame()) {
        m_time_controller.frame_update();
        return;
    }

    m_ppdriver->update();

    ecs::make_singles_system<Entity>([seconds](VisibilityChain & vis) {
        if (!vis.visible || !vis.next) return;
        if ((vis.time_spent += seconds) > VisibilityChain::k_to_next) {
            vis.time_spent = 0;
            vis.visible = false;
            Entity{vis.next}.get<VisibilityChain>().visible = true;
        }
    },
    [](TranslationFromParent & trans_from_parent, Translation & trans) {
        auto pent = Entity{trans_from_parent.parent};
        Real s = 1;
        auto & state = pent.get<PpState>();
        if (auto * on_surf = get_if<PpOnSurface>(&state)) {
            s *= on_surf->invert_normal ? -1 : 1;
            s *= (angle_between(on_surf->segment->normal(), k_up) > k_pi*0.5) ? -1 : 1;
        }
        trans = location_of(state) + s*trans_from_parent.translation;
    },
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
        auto willed_dir = normalize(control.heading().y*forward - control.heading().x*left);
        velocity = get_new_velocity(velocity, willed_dir, seconds);
    }, [seconds](PpState & state, Velocity & velocity, Opt<JumpVelocity> jumpvel) {
        if (auto * in_air = get_if<PpInAir>(&state)) {
            velocity = velocity.value + k_gravity*seconds;
            if (jumpvel) {
                *jumpvel = jumpvel->value + k_gravity*seconds;
            }
        } else if(auto * on_segment = get_if<PpOnSurface>(&state)) {
#           if 0
            auto & triangle = *on_segment->segment;
            velocity = velocity.value + cul::project_onto_plane(k_gravity*seconds, triangle.normal());
#           endif
        }
    }, [seconds](PpState & state, Velocity & velocity, Opt<JumpVelocity> jumpvel) {
        auto displacement = velocity.value*seconds + (1. / 2)*k_gravity*seconds*seconds;
        if (auto * in_air = get_if<PpInAir>(&state)) {
            in_air->displacement = displacement + (jumpvel ? jumpvel->value*seconds : Vector{});
        } else if(auto * on_segment = get_if<PpOnSurface>(&state)) {
            auto & triangle = *on_segment->segment;
            displacement = cul::project_onto_plane(displacement, triangle.normal());
            auto pt_v3 = triangle.point_at(on_segment->location);
            auto new_pos = triangle.closest_point(pt_v3 + displacement);
            on_segment->displacement = new_pos - on_segment->location;
        }
    }, [this](PpState & state, Opt<Velocity> vel) {

        class Impl final : public point_and_plane::EventHandler {
        public:
            Impl(Opt<Velocity> & vel): m_vel(vel) {}
            Variant<Vector2, Vector> displacement_after_triangle_hit
                (const TriangleSegment & triangle, const Vector & /*location*/,
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
                (const TriangleSegment &, const TriangleSegment * to,
                 const Vector &, const Vector &) const final
            {
                // always transfer to the other surface
                if (!to) {
                    return make_tuple(false, Vector2{});
                }
                // may I know the previous inversion value?
                return make_tuple(false, Vector2{});
            }

            bool cling_to_edge(const TriangleSegment & triangle, TriangleSide side) const final {
                if (!m_vel) return true;
                auto [sa, sb] = triangle.side_points(side); {}
                *m_vel = project_onto(m_vel->value, sa - sb);
                return true;
            }

            Opt<Velocity> & m_vel;
        };
        // on catching flip-flop bug: I want to watch displacements in 3 space between flops
        Vector dis;
        if (auto * onsurf = get_if<PpOnSurface>(&state)) {
            dis = onsurf->segment->point_at(onsurf->displacement);
        }

        Impl impl{vel};
        state = (*m_ppdriver)(state, impl);
    }, [](PpState & state, PlayerControl & control, JumpVelocity & vel) {
        constexpr auto k_jump_vel = k_up*10;
        {
        auto * on_segment = get_if<PpOnSurface>(&state);
        if (on_segment && control.is_starting_jump()) {
            auto & triangle = *on_segment->segment;
            auto dir = (on_segment->invert_normal ? -1 : 1)*triangle.normal()*0.1;
            vel = k_jump_vel;
            state = PpInAir{triangle.point_at(on_segment->location) + dir, Vector{}};
        }
        }
        {
        auto * in_air = get_if<PpInAir>(&state);
        if (in_air && control.is_ending_jump() && !are_very_close(vel.value, Vector{})) {
            vel = normalize(vel.value)*std::sqrt(magnitude(vel.value));
        }
        }
    })(scene());

    for (auto e : scene()) {
        auto * on_surf = get_if<PpOnSurface>(e.ptr<PpState>());
        if (!on_surf) continue;

    }

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

    m_time_controller.frame_update();
}

} // end of <anonymous> namespace
