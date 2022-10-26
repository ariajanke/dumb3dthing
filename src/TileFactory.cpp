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

#include "TileFactory.hpp"
#include "WallTileFactory.hpp"
#include "RenderModel.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

using Triangle = TriangleSegment;

} // end of <anonymous> namespace

TileFactory::NeighborInfo::NeighborInfo
    (const SlopesGridInterface & slopesintf, Vector2I tilelocmap,
     Vector2I spawner_offset):
    m_grid(&slopesintf),
    m_loc(tilelocmap),
    m_offset(spawner_offset)
{}

/* static */ TileFactory::NeighborInfo
    TileFactory::NeighborInfo::make_no_neighbor()
{
    return NeighborInfo{
        SlopesGridInterface::null_instance(),
        Vector2I{}, Vector2I{}};
}

Real TileFactory::NeighborInfo::neighbor_elevation(CardinalDirection dir) const {
    using Cd = CardinalDirection;

    using VecCdTup = Tuple<Vector2I, Cd>;
    auto select_el = [this] (const std::array<VecCdTup, 2> & arr) {
        // I miss map, transform sucks :c
        std::array<Real, 2> vals;
        std::transform(arr.begin(), arr.end(), vals.begin(), [this] (const VecCdTup & tup) {
            auto [r, d] = tup; {}
            return neighbor_elevation(r, d);
        });
        auto itr = std::find_if(vals.begin(), vals.end(),
            [] (Real x) { return cul::is_real(x); });
        return itr == vals.end() ? k_inf : *itr;
    };

    switch (dir) {
    case Cd::n: case Cd::s: case Cd::e: case Cd::w:
        throw InvArg{"Not a corner"};
    case Cd::nw:
        return select_el(std::array{
            make_tuple(Vector2I{ 0, -1}, Cd::sw),
            make_tuple(Vector2I{-1 , 0}, Cd::ne)
        });
    case Cd::sw:
        return select_el(std::array{
            make_tuple(Vector2I{-1,  0}, Cd::se),
            make_tuple(Vector2I{ 0,  1}, Cd::nw)
        });
    case Cd::se:
        return select_el(std::array{
            make_tuple(Vector2I{ 1,  0}, Cd::sw),
            make_tuple(Vector2I{ 0 , 1}, Cd::ne)
        });
    case Cd::ne:
        return select_el(std::array{
            make_tuple(Vector2I{ 1,  0}, Cd::nw),
            make_tuple(Vector2I{ 0, -1}, Cd::se)
        });
    default: break;
    }
    throw BadBranchException{__LINE__, __FILE__};
}

/* private */ Real TileFactory::NeighborInfo::neighbor_elevation
    (const Vector2I & r, CardinalDirection dir) const
{
    using Cd = CardinalDirection;
    auto get_slopes = [this, r] { return (*m_grid)(m_loc + r); };
    switch (dir) {
    case Cd::n: case Cd::s: case Cd::e: case Cd::w:
        throw InvArg{"Not a corner"};
    case Cd::nw: return get_slopes().nw;
    case Cd::sw: return get_slopes().sw;
    case Cd::se: return get_slopes().se;
    case Cd::ne: return get_slopes().ne;
    default: break;
    }
    throw BadBranchException{__LINE__, __FILE__};
}

void TileFactory::set_shared_texture_information
    (const SharedCPtr<Texture> & texture_ptr_, const Size2 & texture_size_,
     const Size2 & tile_size_)
{
    m_texture_ptr = texture_ptr_;
    m_texture_size = texture_size_;
    m_tile_size = tile_size_;
}

/* protected static */ void TileFactory::add_triangles_based_on_model_details
    (Vector2I gridloc, Vector translation, const Slopes & slopes,
     EntityAndTrianglesAdder & adder)
{
    const auto & els = TileGraphicGenerator::get_common_elements();
    const auto pos = TileGraphicGenerator::get_points_for(slopes);
    auto offset = grid_position_to_v3(gridloc) + translation;
    adder.add_triangle(TriangleSegment{
        pos[els[0]] + offset, pos[els[1]] + offset, pos[els[2]] + offset});
    adder.add_triangle(TriangleSegment{
        pos[els[3]] + offset, pos[els[4]] + offset, pos[els[5]] + offset});
}

/* protected static */ const char * TileFactory::find_property
    (const char * name_, const tinyxml2::XMLElement * properties)
{
    for (auto itr = properties; itr; itr = itr->NextSiblingElement("property")) {
        auto name = itr->Attribute("name");
        auto val = itr->Attribute("value");
        if (!val || !name) continue;
        if (strcmp(name, name_)) continue;
        return val;
    }
    return nullptr;
}

/* protected */ Size2 TileFactory::common_texture_tile_size() const {
    return Size2{m_tile_size.width  / m_texture_size.width ,
                 m_tile_size.height / m_texture_size.height};
}

/* protected */ Vector2 TileFactory::common_texture_origin(Vector2I ts_r) const {
    const auto scale = common_texture_tile_size();
    return Vector2{ts_r.x*scale.width, ts_r.y*scale.height};
}

/* protected */ TileTextureN TileFactory::floor_texture_at(Vector2I r) const {
    using cul::convert_to;
    Size2 scale{m_tile_size.width  / m_texture_size.width ,
                m_tile_size.height / m_texture_size.height};
    Vector2 offset{r.x*scale.width, r.y*scale.height};
    return TileTextureN{offset, offset + convert_to<Vector2>(scale)};
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

/* protected */ SharedCPtr<RenderModel>
    TileFactory::make_render_model_with_common_texture_positions
    (Platform::ForLoaders & platform, const Slopes & slopes,
     Vector2I loc_in_ts) const
{
    const auto & pos = TileGraphicGenerator::get_points_for(slopes);
    auto txpos = common_texture_positions_from(loc_in_ts);

    std::vector<Vertex> verticies;
    assert(txpos.size() == pos.size());
    for (int i = 0; i != int(pos.size()); ++i) {
        verticies.emplace_back(pos[i], txpos[i]);
    }

    auto render_model = platform.make_render_model(); // need platform
    const auto & els = TileGraphicGenerator::get_common_elements();
    render_model->load(verticies, els);
    return render_model;
}

/* protected */ Entity TileFactory::make_entity
    (Platform::ForLoaders & platform, Vector translation,
     const SharedCPtr<RenderModel> & model_ptr) const
{
    assert(model_ptr);
    auto ent = platform.make_renderable_entity();
    ent.add<SharedCPtr<RenderModel>, SharedCPtr<Texture>, Translation, Visible>() =
            make_tuple(model_ptr, common_texture(), Translation{translation}, true);
    return ent;
}
