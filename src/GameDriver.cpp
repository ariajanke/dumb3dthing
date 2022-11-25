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
#include "Components.hpp"
#include "RenderModel.hpp"
#include "map-loader/map-loader.hpp"
#include "Texture.hpp"
#include "Systems.hpp"
#include "map-loader/tiled-map-loader.hpp"

#include <ariajanke/cul/BezierCurves.hpp>
#include <ariajanke/cul/TestSuite.hpp>

#include <iostream>

namespace {

using namespace cul::exceptions_abbr;

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

class GameDriverComplete final : public Driver {
public:
    void press_key(KeyControl) final;

    void release_key(KeyControl) final;

private:
    void initial_load(LoaderCallbacks &) final;

    void update_(Real seconds) final;

    point_and_plane::Driver & ppdriver() final
        { return *m_ppdriver; }

    Vector m_camera_target;
    UniquePtr<point_and_plane::Driver> m_ppdriver = point_and_plane::Driver::make_driver();
    TimeControl m_time_controller;
};

template <typename T>
auto make_sole_owner_pred() {
    return [](const SharedPtr<T> & ptr) { return ptr.use_count() == 1; };
}

} // end of <anonymous> namespace

/* static */ UniquePtr<Driver> Driver::make_instance()
    { return make_unique<GameDriverComplete>(); }

void Driver::update(Real seconds, Platform & callbacks) {
    update_(seconds);

    std::vector<Entity> entities;
    auto callbacks_ = get_callbacks(callbacks, entities);
    {
    auto enditr = m_every_frame_tasks.begin();
    m_every_frame_tasks.erase
        (std::remove_if
            (m_every_frame_tasks.begin(), enditr,
             make_sole_owner_pred<EveryFrameTask>()),
         enditr);
    }
    for (auto & task : m_every_frame_tasks) {
        task->on_every_frame(callbacks_, seconds);
    }
#   if 0
    for (auto & task : m_occasional_tasks) {
        // v same bug?
        m_scene.update_entities();
        task->on_occasion(callbacks_);
    }
#   endif
    for (auto & task : m_loader_tasks) {
        (*task)(callbacks_);
    }

    for (auto & task : m_background_tasks) {
        // v same bug?
        m_scene.update_entities();

        if ((*task)(callbacks_) == BackgroundCompletion::finished)
            task = nullptr;
    }
    m_background_tasks.erase
        (std::remove(m_background_tasks.begin(), m_background_tasks.end(), nullptr),
         m_background_tasks.end());
#   if 0
    m_occasional_tasks.clear();
#   endif
    m_loader_tasks.clear();

    on_entities_changed(entities);
    m_scene.add_entities(entities);
    if (!entities.empty()) {
        std::cout << "There are now " << m_scene.count() << " entities." << std::endl;
    }

    m_scene.update_entities();
    callbacks.render_scene(m_scene);
}

/* protected */ void Driver::on_entities_changed(std::vector<Entity> & new_entities) {
    if (new_entities.empty()) return;
    // I need to peak at new entities?
    for (auto & ent : new_entities) {
        if (auto * every_frame = ent.ptr<SharedPtr<EveryFrameTask>>()) {
            m_every_frame_tasks.push_back(*every_frame);
        }
    }

    int new_triangles = 0;
    for (auto & ent : new_entities) {
        auto * vec = ent.ptr<TriangleLinks>();
        if (!vec) continue;
        ppdriver().add_triangles(*vec);
        new_triangles += vec->size();
        // ecs has a bug/design flaw...
        // when adding entities directly, requesting deletion can cause a
        // confusing error to be thrown
        // this is because the entities are not yet in order, and so binary
        // searching for it inside the scene fails
    }
    std::cout << "Added " << new_triangles << " new triangles." << std::endl;

}

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
#   if 0
    std::vector<SharedPtr<TriangleLink>> links;
    if (triangles.size() > 1) {
        links.emplace_back( triangles.front() );
        for (auto itr = triangles.begin() + 1; itr != triangles.end(); ++itr) {

            links.back()->attempt_attachment_to( *itr );
            links.emplace_back( *itr );
            links.back()->attempt_attachment_to( *(itr - 1) );
        }
    }
#   endif
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
    rv.add<Translation>() = Vector{ 4, 0, -3 };
    return rv;
}

class CircleLine final {
public:
    CircleLine(const Tuple<Vector, Vector, Vector> & pts_):
        m_points(pts_) {}

    Vector operator () (Real) const
        { throw RtError{"unimplemented"}; }

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
    rv.add<Translation, YRotation>() = make_tuple(Vector{4, 0, 0}, k_pi*0.5);
    return rv;
}

static constexpr const Vector k_player_start{2, 5.1, -2};

// model entity, physical entity
Tuple<Entity, Entity> make_sample_player(TaskCallbacks & callbacks, point_and_plane::Driver & ppdriver) {
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

    auto model = callbacks.platform().make_render_model();
    model->load(&verticies.front(), &verticies.front() + verticies.size(),
                &elements .front(), &elements .front() + elements.size());

    auto physics_ent = Entity::make_sceneless_entity();
    auto model_ent   = callbacks.platform().make_renderable_entity();

    auto tx = callbacks.platform().make_texture();
    tx->load_from_file("ground.png");
    model_ent.add
        <SharedPtr<const Texture>, SharedPtr<const RenderModel>, Translation,
         TranslationFromParent>
        () = make_tuple
        (tx, model, Translation{k_player_start},
         TranslationFromParent{EntityRef{physics_ent}, Vector{0, 0.5, 0}});

    physics_ent.add<PpState>(PpInAir{k_player_start, Vector{}});
    physics_ent.add<JumpVelocity, DragCamera, Camera, PlayerControl>();

    // This sort of is "temporary" code.
    // So not subjecting to testing.
    {



    static constexpr const auto k_testmap_filename = "demo-map.tmx";
#   if 0
    using TeardownTask = MapLoader::TeardownTask;
    std::map<std::string, MapLoader> map_loaders;
    using MpTuple = Tuple<Vector2I, MapLoader *, SharedPtr<TeardownTask>>;
    std::vector<MpTuple> loaded_maps;
    static constexpr const auto k_load_limit = 3;

    static auto check_fall_below = [](Entity & ent) {
        auto * ppair = get_if<PpInAir>(&ent.get<PpState>());
        if (!ppair) return;
        auto & loc = ppair->location;
        if (loc.y < -10) {
            loc = Vector{loc.x, 4, loc.z};
            ent.get<Velocity>() = Velocity{};
        }
    };

    physics_ent.add<SharedPtr<EveryFrameTask>>() =
        EveryFrameTask::make(
        [loaded_maps = std::move(loaded_maps),
         map_loaders = std::move(map_loaders),
         physics_ent]
        (TaskCallbacks & callbacks, Real) mutable
    {
        check_fall_below(physics_ent);

        static constexpr const auto k_base_map_size = 20;
        // if there's no teardown task... then it's pending
        for (auto & [gpos, loader, teardown] : loaded_maps) {
            if (teardown) continue;

            auto [task_ptr, teardown_ptr] = (*loader)(gpos*k_base_map_size); {}
            if (!task_ptr) continue;
            callbacks.add(task_ptr);
            teardown = teardown_ptr;
            physics_ent.ensure<Velocity>();
        }

        using namespace point_and_plane;
        auto player_loc = location_of(physics_ent.get<PpState>());
        auto gposv3 =
            (Vector{0.5, 0, -0.5} + player_loc)*
            ( 1. / k_base_map_size );
        Vector2I gpos{ int(std::floor(gposv3.x)), int(std::floor(-gposv3.z)) };

        bool current_posisition_loaded = std::any_of(
            loaded_maps.begin(), loaded_maps.end(),
            [gpos](const MpTuple & tup) { return std::get<Vector2I>(tup) == gpos; });
        if (current_posisition_loaded) return;
        auto itr = map_loaders.find(k_testmap_filename);
        if (itr == map_loaders.end()) {
            itr = map_loaders.insert(std::make_pair(k_testmap_filename, MapLoader{callbacks.platform()})).first;
            itr->second.start_preparing(k_testmap_filename);
        }
        loaded_maps.emplace_back( gpos, &itr->second, nullptr);
        int n_too_many = std::max(int(loaded_maps.size()) - k_load_limit, 0);
        for (auto itr = loaded_maps.begin(); itr != loaded_maps.begin() + n_too_many; ++itr) {
            auto & teardown_task = std::get<SharedPtr<TeardownTask>>(*itr);
            callbacks.add(teardown_task);
        }
        loaded_maps.erase(loaded_maps.begin(), loaded_maps.begin() + n_too_many);
    });
#   else
    PlayerUpdateTask
        {MapLoadingDirector{&ppdriver, cul::Size2<int>{10, 10}},
         EntityRef{physics_ent}};
    auto player_update_task = make_shared<PlayerUpdateTask>
        (MapLoadingDirector{&ppdriver, cul::Size2<int>{10, 10}},
         EntityRef{physics_ent});
    player_update_task->load_initial_map(callbacks, k_testmap_filename, callbacks.platform());
    SharedPtr<EveryFrameTask> & ptrref = physics_ent.add<SharedPtr<EveryFrameTask>>();
    ptrref = SharedPtr<EveryFrameTask>{player_update_task};
#   endif


    }

    return make_tuple(model_ent, physics_ent);
}

// ----------------------------------------------------------------------------

void GameDriverComplete::press_key(KeyControl ky) {
    player_entities().physical.get<PlayerControl>().press(ky);
    m_time_controller.press(ky);
    if (ky == KeyControl::restart) {
        player_entities().physical.get<PpState>() = PpInAir{k_player_start, Vector{}};
    }
}

void GameDriverComplete::release_key(KeyControl ky) {
    player_entities().physical.get<PlayerControl>().release(ky);
}

void GameDriverComplete::initial_load(LoaderCallbacks & callbacks) {
    auto [renderable, physical] = make_sample_player(callbacks, ppdriver()); {}
    callbacks.platform().set_camera_entity(EntityRef{physical});
    callbacks.set_player_entities(PlayerEntities{physical, renderable});
    callbacks.add(physical);
    callbacks.add(renderable);
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
    [](TranslationFromParent & trans_from_parent, Translation & trans) {
        auto pent = Entity{trans_from_parent.parent};
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
    [ppstate = player_entities().physical.get<PpState>(),
     plyvel  = player_entities().physical.ptr<Velocity>()]
        (Translation & trans, Opt<Visible> vis)
    {
        using point_and_plane::location_of;
        if (!vis) return;
        Vector vel = plyvel ? plyvel->value*0.4 : Vector{};
        auto dist = magnitude(location_of(ppstate) + vel - trans.value);
        *vis = dist < 12;
    })(scene());

    // this code here is a good candidate for a "Trigger" system
    auto player = player_entities().physical;
    player.get<PlayerControl>().frame_update();

    auto pos = location_of(player.get<PpState>()) + Vector{0, 3, 0};
    auto & cam = player.get<DragCamera>();
    if (magnitude(cam.position - pos) > cam.max_distance) {
        cam.position += normalize(pos - cam.position)*(magnitude(cam.position - pos) - cam.max_distance);
        assert(are_very_close( magnitude( cam.position - pos ), cam.max_distance ));
    }
    player.get<Camera>().target = location_of(player.get<PpState>());
    player.get<Camera>().position = cam.position;

    m_time_controller.frame_update();
}

} // end of <anonymous> namespace
