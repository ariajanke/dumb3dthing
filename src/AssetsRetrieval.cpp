/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "AssetsRetrieval.hpp"
#include "platform.hpp"

#include <ariajanke/cul/BezierCurves.hpp>

namespace {

template <typename Vec, typename ... Types>
std::enable_if_t<cul::detail::k_are_vector_types<Vec, Types...>, RenderModelData>
    make_bezier_model_geometry
        (const Tuple<Vec, Types...> & lhs,
         const Tuple<Vec, Types...> & rhs,
         int resolution,
         Vector2 texture_offset,
         Real texture_scale,
         RenderModelData && model_data = RenderModelData{})
{
    std::vector<Vertex> & verticies = model_data.vertices;
    std::vector<unsigned> & elements = model_data.elements;
    unsigned el = elements.size();
    for (auto [a, b, c] : cul::make_bezier_strip(lhs, rhs, resolution).details_view()) {
        auto a_pt = a.point();
        auto b_pt = b.point();
        auto c_pt = c.point();
        if (are_very_close(a_pt, b_pt) || are_very_close(c_pt, a_pt) || are_very_close(b_pt, c_pt))
            { continue; }
        verticies.emplace_back(a_pt, texture_offset + Vector2{1.f*a.on_right(), a.position()}*texture_scale);
        verticies.emplace_back(b_pt, texture_offset + Vector2{1.f*b.on_right(), b.position()}*texture_scale);
        verticies.emplace_back(c_pt, texture_offset + Vector2{1.f*c.on_right(), c.position()}*texture_scale);

        elements.emplace_back(el++);
        elements.emplace_back(el++);
        elements.emplace_back(el++);
    }
    return std::move(model_data);
}

} // end of <anonymous> namespace


class NonSavingAssetsRetrieval final : public AssetsRetrieval {
public:
    NonSavingAssetsRetrieval(PlatformAssetsStrategy & platform):
        m_platform(platform)
    {}

    SharedPtr<const RenderModel> for_model(const RenderModelData & model_data) {
        auto mod = m_platform.make_render_model();
        mod->load(model_data);
        return mod;
    }

    SharedPtr<const Texture> for_texture(const char * filename) {
        auto tx = m_platform.make_texture();
        tx->load_from_file(filename);
        return tx;
    }

    SharedPtr<const RenderModel> make_cube_model() final;

    SharedPtr<const RenderModel> make_cone_model() final;

    SharedPtr<const RenderModel> make_vaguely_tree_like_model() final;

    SharedPtr<const RenderModel> make_grass_model() final;

    SharedPtr<const Texture> make_ground_texture() final;

private:
    PlatformAssetsStrategy & m_platform;
};

class SavingAssetsRetrieval final : public AssetsRetrieval {
public:
    SavingAssetsRetrieval(PlatformAssetsStrategy & platform):
        m_retrieval(platform)
    {}

    SharedPtr<const RenderModel> make_cube_model() final;

    SharedPtr<const RenderModel> make_cone_model() final;

    SharedPtr<const RenderModel> make_vaguely_tree_like_model() final;

    SharedPtr<const RenderModel> make_grass_model() final;

    SharedPtr<const Texture> make_ground_texture() final;

private:
    // more weaknessess of C++ :/
    // which could potentially be fixed using templates, but using templates
    // as a crutch and to cover for C++'s awful ""design"" is even more of a
    // miss. Especially for a so-called "strongly typed" language

    // omg...
    template <std::size_t kt_i, typename Func>
    SharedPtr<const RenderModel> wrap_check_for_saved_model(Func && f) {
        auto & saved_model = m_saved_models[kt_i];
        if (!saved_model.expired()) {
            return saved_model.lock();
        }
        SharedPtr<const RenderModel> t = f();
        saved_model = t;
        return t;
    }

    std::array<WeakPtr<const RenderModel>, 4> m_saved_models;
    std::array<WeakPtr<const Texture>, 1> m_saved_textures;
    NonSavingAssetsRetrieval m_retrieval;
};

SharedPtr<const RenderModel> NonSavingAssetsRetrieval::make_cube_model() {
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

    auto rm = m_platform.make_render_model();
    rm->load
        (&verticies.front(), &verticies.front() + verticies.size(),
         &elements .front(), &elements .front() + elements.size());
    return rm;
}

SharedPtr<const RenderModel> NonSavingAssetsRetrieval::make_cone_model() {
    constexpr const int k_faces = 10;
    constexpr const Vector tip = k_up*0.5;
    std::array<Vertex, k_faces + 1> verticies;
    verticies[0] = Vertex{tip, Vector2{}};
    auto pt_at = [] (Real t) {
        return -k_up*0.5 + k_east*0.5*std::sin(t) + k_north*0.5*std::cos(t);
    };
    for (int i = 0; i != k_faces; ++i) {
        Real t = Real(i) / k_pi*2.;
        auto current_pt = pt_at(t);
        verticies[i + 1] = Vertex{current_pt, Vector2{}};
    }

    std::array<unsigned, (k_faces - 1)*3> elements;
    for (unsigned i = 1; i != k_faces; ++i) {
        auto j = (i - 1)*3;
        assert(j + 2 < elements.size());
        elements[j    ] = 0;
        elements[j + 1] = i;
        elements[j + 2] = i + 1 == k_faces ? 1 : i + 1;
    }
    auto rm = m_platform.make_render_model();
    rm->load
        (&verticies.front(), &verticies.front() + verticies.size(),
         &elements.front(), &elements.front() + elements.size());
    return rm;
}

SharedPtr<const RenderModel> NonSavingAssetsRetrieval::make_vaguely_tree_like_model() {
    auto t1 = make_tuple
        (k_up*3,
         k_up*2.5 + k_east + k_north*0.3,
         k_up*1 + k_east*0.3 + k_north*0.3,
         k_east*0.25 + k_north*0.3);
    auto t2 = make_tuple
        (k_up*3,
         k_up*2.5 + k_east - k_north*0.3,
         k_up*1 + k_east*0.3 - k_north*0.3,
         k_east*0.25 - k_north*0.3);
    auto t3 = make_tuple
        (k_up*3,
         k_up*2.6 + k_east*0.4,
         k_up*1.2,
         -k_east*0.2);
    constexpr const int res = 20;
    auto model_data =
        make_bezier_model_geometry(t1, t2, res, Vector2{0, 0}, 1. / 3.);
    model_data =
        make_bezier_model_geometry(t2, t3, res*3/2, Vector2{0, 0}, 1. / 3., std::move(model_data));
    model_data =
        make_bezier_model_geometry(t3, t1, res, Vector2{0, 0}, 1. / 3., std::move(model_data));

    auto mod = m_platform.make_render_model();
    mod->load(model_data);
    return mod;
}

SharedPtr<const RenderModel> NonSavingAssetsRetrieval::make_grass_model() {
    static constexpr Vector2 tx_offset{1. / 3., 2. / 3.};
    static constexpr Real    tx_scale = 1. / 3.;
    auto adjust_tx = [] (Real x, Real y) {
        return tx_offset + Vector2{x, y}*tx_scale;
    };
    std::array verticies = {
        Vertex{ k_east*0.5  + k_up*0.5, adjust_tx(0., 1. / 4.)},
        Vertex{-k_east*0.5  + k_up*0.5, adjust_tx(1., 1. / 4.)},
        Vertex{-k_east*0.5            , adjust_tx(1., 2. / 4.)},
        Vertex{ k_east*0.5            , adjust_tx(0., 2. / 4.)},

        Vertex{ k_north*0.5 + k_up*0.5, adjust_tx(0., 3. / 4.)},
        Vertex{-k_north*0.5 + k_up*0.5, adjust_tx(1., 3. / 4.)},
        Vertex{-k_north*0.5           , adjust_tx(1., 2. / 4.)},
        Vertex{ k_north*0.5           , adjust_tx(0., 2. / 4.)}
    };
    std::array elements = {
        0u, 1u, 2u,
        2u, 3u, 0u,
        4u, 5u, 6u,
        6u, 7u, 4u
    };
    auto mod = m_platform.make_render_model();
    mod->load
        (&verticies.front(), &verticies.front() + verticies.size(),
         &elements.front(), &elements.front() + elements.size());
    return mod;
}

SharedPtr<const Texture> NonSavingAssetsRetrieval::make_ground_texture() {
    auto t = m_platform.make_texture();
    t->load_from_file("ground.png");
    return t;
}

SharedPtr<const RenderModel> SavingAssetsRetrieval::make_cube_model() {
    return wrap_check_for_saved_model<0>([this] {
        return m_retrieval.make_cube_model();
    });
}

SharedPtr<const RenderModel> SavingAssetsRetrieval::make_cone_model() {
    return wrap_check_for_saved_model<1>([this] {
        return m_retrieval.make_cone_model();
    });
}

SharedPtr<const RenderModel> SavingAssetsRetrieval::make_vaguely_tree_like_model() {
    return wrap_check_for_saved_model<2>([this] {
        return m_retrieval.make_vaguely_tree_like_model();
    });
}

SharedPtr<const RenderModel> SavingAssetsRetrieval::make_grass_model() {
    return wrap_check_for_saved_model<3>([this] {
        return m_retrieval.make_grass_model();
    });
}

SharedPtr<const Texture> SavingAssetsRetrieval::make_ground_texture() {
    auto & saved_texture = m_saved_textures[0];
    if (!saved_texture.expired()) {
        return saved_texture.lock();
    }
    auto t = m_retrieval.make_ground_texture();
    saved_texture = t;
    return t;
}

/* static */ SharedPtr<AssetsRetrieval> AssetsRetrieval::make_non_saving_instance
    (PlatformAssetsStrategy & platform)
{ return make_shared<NonSavingAssetsRetrieval>(platform); }

/* static */ SharedPtr<AssetsRetrieval> AssetsRetrieval::make_saving_instance
    (PlatformAssetsStrategy & platform)
{
    // I don't think this is actually thread safe :/
    static std::atomic<PlatformAssetsStrategy *> s_strat_ptr;
    static WeakPtr<AssetsRetrieval> s_memoized;
    if (s_strat_ptr == &platform && !s_memoized.expired()) {
        return s_memoized.lock();
    }

    auto rv = make_shared<SavingAssetsRetrieval>(platform);
    s_memoized = rv;
    s_strat_ptr = &platform;
    return rv;
}
