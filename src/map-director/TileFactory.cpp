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

#include "TileFactory.hpp"
#include "TileSetPropertiesGrid.hpp"
#include "ProducableGrid.hpp"

#include "../RenderModel.hpp"
#include "../Components.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

using Triangle = TriangleSegment;

static const constexpr std::array k_flat_points = {
    Vector{-.5, 0, 0.5}, // nw
    Vector{-.5, 0, -.5}, // sw
    Vector{0.5, 0, -.5}, // se
    Vector{0.5, 0, 0.5}  // ne
};

} // end of <anonymous> namespace

void TileFactory::set_shared_texture_information
    (const SharedPtr<const Texture> & texture_ptr_, const Size2 & texture_size_,
     const Size2 & tile_size_)
{
    m_texture_ptr = texture_ptr_;
    m_texture_size = texture_size_;
    m_tile_size = tile_size_;
}

void TileFactory::setup
    (const TileSetXmlGrid & xml_grid, Platform & platform,
     const Vector2I & location_on_tileset)
{
    m_texture_ptr = xml_grid.texture();
    m_texture_size = xml_grid.texture_size();
    m_tile_size = xml_grid.tile_size();
    setup_(location_on_tileset, xml_grid(location_on_tileset), platform);
}

/* protected static */ void TileFactory::add_triangles_based_on_model_details
    (Vector2I gridloc, Vector translation, const Slopes & slopes,
     ProducableTileCallbacks & callbacks)
{
    auto offset = grid_position_to_v3(gridloc) + translation;
#   if 0
    const auto & els = get_common_elements();
    const auto pos = get_points_for(slopes);
    callbacks.add(TriangleSegment{
        pos[els[0]] + offset, pos[els[1]] + offset, pos[els[2]] + offset});
    callbacks.add(TriangleSegment{
        pos[els[3]] + offset, pos[els[4]] + offset, pos[els[5]] + offset});
#   else
    auto el_pt_of = [&slopes] {
        const auto & els = get_common_elements();
        const auto pos = get_points_for(slopes);
        return [=] (int n) { return pos[els[n]]; };
    } ();
    TriangleSegment first {el_pt_of(0), el_pt_of(1), el_pt_of(2)};
    TriangleSegment second{el_pt_of(3), el_pt_of(4), el_pt_of(5)};
    callbacks.add_collidable(first .move(offset));
    callbacks.add_collidable(second.move(offset));
#   endif
}

/* protected static */ std::array<Vector, 4>
    TileFactory::get_points_for(const Slopes & slopes)
{
    return std::array<Vector, 4>
        {k_flat_points[0] + Vector{0, slopes.nw, 0},
         k_flat_points[1] + Vector{0, slopes.sw, 0},
         k_flat_points[2] + Vector{0, slopes.se, 0},
         k_flat_points[3] + Vector{0, slopes.ne, 0}};
}

/* protected static */ const std::vector<unsigned> &
    TileFactory::get_common_elements()
{
    static constexpr const std::array<unsigned, 6> arr =
        {0, 1, 2, 0, 2, 3};
    static const std::vector<unsigned> s_rv{arr.begin(), arr.end()};
    return s_rv;
}

/* protected */ Size2 TileFactory::common_texture_tile_size() const {
    return Size2{m_tile_size.width  / m_texture_size.width ,
                 m_tile_size.height / m_texture_size.height};
}

/* protected */ Vector2 TileFactory::common_texture_origin(Vector2I ts_r) const {
    const auto scale = common_texture_tile_size();
    return Vector2{ts_r.x*scale.width, ts_r.y*scale.height};
}

/* protected */ TileTexture TileFactory::floor_texture_at(Vector2I r) const {
    using cul::convert_to;
    Size2 scale{m_tile_size.width  / m_texture_size.width ,
                m_tile_size.height / m_texture_size.height};
    Vector2 offset{r.x*scale.width, r.y*scale.height};
    return TileTexture{offset, offset + convert_to<Vector2>(scale)};
}

/* protected */ std::array<Vector2, 4> TileFactory::common_texture_positions_from
    (Vector2I ts_r) const
{
    const auto scale = common_texture_tile_size();
    const std::array<Vector2, 4> k_base = {
        // for textures not physical locations
        Vector2{ 0*scale.width, 0*scale.height }, // nw
        Vector2{ 0*scale.width, 1*scale.height }, // sw
        Vector2{ 1*scale.width, 1*scale.height }, // se
        Vector2{ 1*scale.width, 0*scale.height }  // ne
    };
    auto origin = common_texture_origin(ts_r);
    auto rv = k_base;
    for (auto & r : rv) r += origin;
    return rv;
}

/* protected */ SharedPtr<const RenderModel>
    TileFactory::make_render_model_with_common_texture_positions
    (Platform & platform, const Slopes & slopes, Vector2I loc_in_ts) const
{
    const auto & pos = get_points_for(slopes);
    auto txpos = common_texture_positions_from(loc_in_ts);

    std::vector<Vertex> verticies;
    assert(txpos.size() == pos.size());
    for (int i = 0; i != int(pos.size()); ++i) {
        verticies.emplace_back(pos[i], txpos[i]);
    }

    auto render_model = platform.make_render_model(); // need platform
    const auto & els = get_common_elements();
    render_model->load(verticies, els);
    return render_model;
}

#if 0
/* protected */ Entity TileFactory::make_entity
    (Platform & platform, Vector translation,
     const SharedPtr<const RenderModel> & model_ptr) const
{
    assert(model_ptr);
    auto ent = platform.make_renderable_entity();
    ent.add
        <SharedPtr<const RenderModel>, SharedPtr<const Texture>,
         Translation, Visible>
        () = make_tuple
        (model_ptr, common_texture(), Translation{translation}, true);
    return ent;
}
#endif
