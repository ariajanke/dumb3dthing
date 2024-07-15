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

#include "RenderModel.hpp"
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

/* static */ SharedPtr<const RenderModel> RenderModel::make_cube
    (PlatformAssetsStrategy & platform)
{
    static WeakPtr<RenderModel> s_memoized_cube;
    if (!s_memoized_cube.expired()) {
        return s_memoized_cube.lock();
    }

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

    auto rm = platform.make_render_model();
    rm->load
        (&verticies.front(), &verticies.front() + verticies.size(),
         &elements .front(), &elements .front() + elements.size());
    s_memoized_cube = rm;
    return rm;
}

/* static */ SharedPtr<const RenderModel> RenderModel::make_cone
    (PlatformAssetsStrategy & platform)
{
    static WeakPtr<RenderModel> s_memoized_cone;
    if (!s_memoized_cone.expired()) {
        return s_memoized_cone.lock();
    }

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
    auto rm = platform.make_render_model();
    rm->load
        (&verticies.front(), &verticies.front() + verticies.size(),
         &elements.front(), &elements.front() + elements.size());
    s_memoized_cone = rm;
    return rm;
}

/* static */ SharedPtr<const RenderModel>
    RenderModel::make_vaguely_tree_like_thing
    (PlatformAssetsStrategy & platform)
{
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
    auto model_data =
        make_bezier_model_geometry(t1, t2, 12, Vector2{0, 0}, 1. / 3.);
    model_data =
        make_bezier_model_geometry(t2, t3, 12, Vector2{0, 0}, 1. / 3., std::move(model_data));
    model_data =
        make_bezier_model_geometry(t3, t1, 12, Vector2{0, 0}, 1. / 3., std::move(model_data));

    auto mod = platform.make_render_model();
    mod->load(model_data);
    return mod;
}

void RenderModel::load(const RenderModelData & model_data)
    { load(model_data.vertices, model_data.elements); }

void RenderModel::load
    (const std::vector<Vertex> & vertices, const std::vector<unsigned> & elements)
{
    load(&vertices.front(), &vertices.front() + vertices.size(),
         &elements.front(), &elements.front() + elements.size());
}

void RenderModel::load
    (const Vertex   * vertex_beg  , const Vertex   * vertex_end,
     const unsigned * elements_beg, const unsigned * elements_end)
{ load_(vertex_beg, vertex_end, elements_beg, elements_end); }
