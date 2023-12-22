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

#include "GameDriver.hpp"
#include "Components.hpp"
#include "RenderModel.hpp"
#include "TasksController.hpp"
#include "Texture.hpp"
#include "Systems.hpp"
#include "Configuration.hpp"

#include "map-director.hpp"
#include <ariajanke/cul/BezierCurves.hpp>
#include <ariajanke/cul/TestSuite.hpp>

#include <iostream>

namespace {

class TimeControl final {
public:
    void press(KeyControl ky) {
        switch (ky) {
        case Kc::advance: m_advance_frame = true; break;
        case Kc::pause: m_paused = !m_paused; break;
        default: return;
        }
    }

    void frame_update()
        { m_advance_frame = false; }

    bool runs_this_frame() const
        { return m_paused ? m_advance_frame : true; }

private:
    using Kc = KeyControl;
    bool m_paused = false, m_advance_frame = false;
};

class FpsCounter final {
public:
    Optional<int> update(Real seconds) {
        m_frame_count += 1;
        m_accumulated_seconds += seconds;
        if (m_accumulated_seconds < 1)
            { return {}; }

        auto rem_secs = std::fmod(m_accumulated_seconds, 1);
        auto n_secs = m_accumulated_seconds - rem_secs;
        auto frames = m_frame_count*(n_secs / m_accumulated_seconds);
        auto rem_frames = m_frame_count - frames;

        m_frame_count = rem_frames;
        m_accumulated_seconds = rem_secs;
        return static_cast<int>(std::round(frames));
    }

private:
    Real m_frame_count = 0;
    Real m_accumulated_seconds = 0;
};

class GameDriverComplete final : public GameDriver {
public:
    void press_key(KeyControl) final;

    void release_key(KeyControl) final;

    void setup(Platform &) final;

    void update(Real seconds, Platform &) final;

private:
    struct PlayerEntities final {
        PlayerEntities() {}
        PlayerEntities(Entity physical_, Entity renderable_):
            physical(physical_), renderable(renderable_) {}

        Entity physical, renderable;
    };

    void initial_load(TaskCallbacks &);

    void update_(Real seconds);

    UniquePtr<point_and_plane::Driver> m_ppdriver = point_and_plane::Driver::make_driver();
    TimeControl m_time_controller;
    Scene m_scene;
    PlayerEntities m_player_entities;
    TasksController m_tasks_controller;
#   if 0
    FpsCounter m_frame_counter;
#   endif
};

} // end of <anonymous> namespace

/* static */ UniquePtr<GameDriver> GameDriver::make_instance()
    { return make_unique<GameDriverComplete>(); }

namespace {

// ------------------------------ <Messy Space> -------------------------------

template <typename Vec, typename ... Types>
std::enable_if_t<cul::detail::k_are_vector_types<Vec, Types...>, Entity>
//  :eyes:
    make_bezier_strip_model
    (const Tuple<Vec, Types...> & lhs, const Tuple<Vec, Types...> & rhs,
     Platform & platform, SharedPtr<Texture> texture, int resolution,
     Vector2 texture_offset, Real texture_scale)
{
    std::vector<TriangleSegment> triangles;
    std::vector<Vertex> verticies;
    std::vector<int> elements;
    int el = 0;
    // uh oh, I need implementation knowledge here :c
    // so I'm trying to map texture positions correctly
    for (auto [a, b, c] : cul::make_bezier_strip(lhs, rhs, resolution).details_view()) {
        verticies.emplace_back(a.point(), texture_offset + Vector2{1.f*a.on_right(), a.position()}*texture_scale);
        verticies.emplace_back(b.point(), texture_offset + Vector2{1.f*b.on_right(), b.position()}*texture_scale);
        verticies.emplace_back(c.point(), texture_offset + Vector2{1.f*c.on_right(), c.position()}*texture_scale);

        triangles.emplace_back(TriangleSegment{a.point(), b.point(), c.point()});

        elements.emplace_back(el++);
        elements.emplace_back(el++);
        elements.emplace_back(el++);
    }

    auto mod = platform.make_render_model();
    mod->load<int>(verticies, elements);

    auto ent = platform.make_renderable_entity();
    ent.add
        <SharedPtr<const RenderModel>, SharedPtr<const Texture>, VisibilityChain>
        () = make_tuple
        (std::move(mod), texture, VisibilityChain{});
    return ent;
}

template <typename T>
Entity make_bezier_yring_model();

Entity make_sample_bezier_model
    (Platform & callbacks, SharedPtr<Texture> texture, int resolution)
{
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
    rv.add<ModelTranslation>() = Vector{ 4, 0, -3 };
    return rv;
}

class CircleLine final {
public:
    CircleLine(const Tuple<Vector, Vector, Vector> & pts_):
        m_points(pts_) {}

    Vector operator () (Real) const
        { throw RuntimeError{"unimplemented"}; }

private:
    const Tuple<Vector, Vector, Vector> & m_points;
};

Entity make_loop(Platform & callbacks,
                 const Tuple<Vector, Vector, Vector> & tup);

Entity make_sample_loop
    (Platform & callbacks, SharedPtr<Texture> texture, int resolution)
{
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
    rv.add<ModelTranslation, YRotation>() = make_tuple(Vector{4, 0, 0}, k_pi*0.5);
    return rv;
}

// model entity, physical entity
Tuple<Entity, Entity>
    make_sample_player
    (Platform & platform)
{
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

    static const auto mk_v = [](Real x, Real y, Real z, int vtidx) {
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
        mk_v(-1,  1, -1, k_bl), // 4: bsw
        mk_v( 1,  1, -1, k_br), // 5: bse
        mk_v( 1, -1, -1, k_tl), // 6: bne
        mk_v(-1, -1, -1, k_tr)  // 7: bnw
    };

    std::array<unsigned, 3*2*6> elements = {
        0, 1, 2, /**/ 0, 2, 3, // top    faces
        0, 1, 7, /**/ 0, 6, 7, // north  faces
        2, 3, 4, /**/ 3, 4, 5, // south  faces
        1, 2, 7, /**/ 2, 7, 4, // west   faces
        0, 3, 6, /**/ 3, 5, 6, // east   faces
        4, 6, 7, /**/ 4, 5, 6  // bottom faces
    };

    auto model = platform.make_render_model();
    model->load(&verticies.front(), &verticies.front() + verticies.size(),
                &elements .front(), &elements .front() + elements.size());

    auto physics_ent = Entity::make_sceneless_entity();
    auto model_ent   = platform.make_renderable_entity();

    auto tx = platform.make_texture();
    tx->load_from_file("ground.png");
    model_ent.add
        <SharedPtr<const Texture>, SharedPtr<const RenderModel>, ModelTranslation,
         TranslationFromParent>
        () = make_tuple
        (tx, model, ModelTranslation{},
         TranslationFromParent{EntityRef{physics_ent}, Vector{0, 0.5, 0}});

    physics_ent.add<JumpVelocity, DragCamera, Camera, PlayerControl>();

    return make_tuple(model_ent, physics_ent);
}

// ----------------------------------------------------------------------------

void GameDriverComplete::press_key(KeyControl ky) {
    m_player_entities.physical.get<PlayerControl>().press(ky);
    m_time_controller.press(ky);
    if (ky == KeyControl::restart) {
        auto & physical = m_player_entities.physical;
        auto recovery_point = physical.get<PlayerRecovery>();
        physical.get<PpState>() = PpInAir{recovery_point.value, Vector{}};
    }
}

void GameDriverComplete::release_key(KeyControl ky) {
    m_player_entities.physical.get<PlayerControl>().release(ky);
}

void GameDriverComplete::setup(Platform & platform_) {
    m_tasks_controller.assign_platform(platform_);
    m_tasks_controller.assign_point_and_plane_driver(*m_ppdriver);
    initial_load(m_tasks_controller);
    m_tasks_controller.add_entities_to(m_scene);
}

void GameDriverComplete::update(Real seconds, Platform & platform) {
    update_(seconds);
    m_tasks_controller.assign_platform(platform);    
    m_tasks_controller.run_tasks(seconds);
    m_tasks_controller.add_entities_to(m_scene);
    platform.render_scene(m_scene);
}

void GameDriverComplete::initial_load(TaskCallbacks & callbacks) {
    auto [renderable, physical] =
        make_sample_player(callbacks.platform());
    callbacks.add
        (MapDirector_::begin_initial_map_loading
            (physical, k_testmap_filename, callbacks.platform(), *m_ppdriver));
    callbacks.platform().set_camera_entity(EntityRef{physical});
    callbacks.add(physical);
    callbacks.add(renderable);

    m_player_entities.physical   = physical;
    m_player_entities.renderable = renderable;
#   if 0
    // let's... head north... starting 0.5+ ues north
    auto west = make_tuple(
        Vector{  3,  3, -20      },
        Vector{  3,  3,  0.5     },
        Vector{  3,  3,  0.5 - 19},
        Vector{2.5, -55,  0.5 - 19},
        Vector{2.5, -55,  0.5 + 19},
        Vector{  6,  3,  0.5 + 19},
        Vector{  6,  3,  0.5     },
        Vector{  6,  3,  20      });

    auto east = make_tuple(
        Vector{  6,  3, -20      },
        Vector{  6,  3,  0.5     },
        Vector{  6,  3,  0.5 - 19},
        Vector{8.5, -55,  0.5 - 19},
        Vector{8.5, -55,  0.5 + 19},
        Vector{  9,  3,  0.5 + 19},
        Vector{  9,  3,  0.5     },
        Vector{  9,  3,  20      });

    auto texture = callbacks.platform().make_texture();
    texture->load_from_file("ground.png");
    auto bezent = make_bezier_strip_model(
        west, east, callbacks.platform(), texture, 64, Vector2{0, 0}, 1. / 3.);
    m_ppdriver->add_triangles(bezent.get<std::vector<TriangleLinks>>());
    bezent.remove<std::vector<TriangleLinks>>();
    callbacks.add_to_scene(bezent);
#   endif
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
    [](TranslationFromParent & trans_from_parent, ModelTranslation & trans) {
        auto pent = Entity{trans_from_parent.parent};
        if (!pent.has<PpState>())
            { return; }
        Real s = 1;
        auto & state = pent.get<PpState>();
        if (auto * on_surf = get_if<PpOnSegment>(&state)) {
            s *= on_surf->invert_normal ? -1 : 1;
            s *= (angle_between(on_surf->segment->normal(), k_up) > k_pi*0.5) ? -1 : 1;
        }
        trans = location_of(state) + s*trans_from_parent.translation;
    },
    PlayerControlToVelocity{seconds},
    AccelerateVelocities{seconds},
    VelocitiesToDisplacement{seconds},
    UpdatePpState{*m_ppdriver},
    CheckJump{},
    [ppstate = m_player_entities.physical.ptr<PpState>(),
     plyvel  = m_player_entities.physical.ptr<Velocity>()]
        (ModelTranslation & trans, EcsOpt<ModelVisibility> vis)
    {
        using point_and_plane::location_of;
        if (!vis || !ppstate) return;
        Vector vel = plyvel ? plyvel->value*0.4 : Vector{};
        auto dist = magnitude(location_of(*ppstate) + vel - trans.value);
        *vis = dist < 12;
    })(m_scene);

    auto & player = m_player_entities.physical;
    player.get<PlayerControl>().frame_update();

    if (auto * ppstate = player.ptr<PpState>()) {
        auto pos = location_of(player.get<PpState>()) + Vector{0, 3, 0};
        auto & cam = player.get<DragCamera>();
        if (magnitude(cam.position - pos) > cam.max_distance) {
            cam.position += normalize(pos - cam.position)*(magnitude(cam.position - pos) - cam.max_distance);
            assert(are_very_close( magnitude( cam.position - pos ), cam.max_distance ));
        }

        player.get<Camera>().target = location_of(player.get<PpState>());
        player.get<Camera>().position = cam.position;
    }

    m_time_controller.frame_update();
}

} // end of <anonymous> namespace
