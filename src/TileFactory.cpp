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

#pragma once

#include "TileFactory.hpp"
#include "WallTileFactory.hpp"
#include "RenderModel.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

using Triangle = TriangleSegment;

// ----------------------------------------------------------------------------

class SlopesBasedModelTile : public TranslatableTileFactory {
public:

    void operator ()
        (EntityAndTrianglesAdder & adder, const NeighborInfo & ninfo,
         Platform::ForLoaders & platform) const final;

protected:
    void add_triangles_based_on_model_details(Vector2I gridloc,
                                              EntityAndTrianglesAdder & adder) const;

    Entity make_entity(Platform::ForLoaders & platform, Vector2I r) const
        { return TranslatableTileFactory::make_entity(platform, r, m_render_model); }

    virtual Slopes model_tile_elevations() const = 0;

    void setup(Vector2I loc_in_ts, const tinyxml2::XMLElement * properties, Platform::ForLoaders & platform) override;

    Slopes tile_elevations() const final
        { return translate_y(model_tile_elevations(), translation().y); }

private:
    SharedCPtr<RenderModel> m_render_model;
};

class Ramp : public SlopesBasedModelTile {
public:
    template <typename T>
    static Tuple<const std::vector<Vector> &, const std::vector<unsigned> &>
        get_model_positions_and_elements_(const Slopes & slopes);

protected:
    virtual void set_direction(const char *) = 0;

    void setup(Vector2I loc_in_ts, const tinyxml2::XMLElement * properties,
               Platform::ForLoaders & platform) final;
};

class CornerRamp : public Ramp {
protected:
    CornerRamp() {}


    Slopes model_tile_elevations() const final
        { return m_slopes; }

    virtual Slopes non_rotated_slopes() const = 0;

    void set_direction(const char * dir) final;

private:
    Slopes m_slopes;
};

class InRampTileFactory final : public CornerRamp {
    Slopes non_rotated_slopes() const final
        { return Slopes{0, 1, 1, 1, 0}; }
};

class OutRampTileFactory final : public CornerRamp {
    Slopes non_rotated_slopes() const final
        { return Slopes{0, 0, 0, 0, 1}; }
};

class TwoRampTileFactory final : public Ramp {
    Slopes model_tile_elevations() const final
        { return m_slopes; }

    void set_direction(const char * dir) final;

    Slopes m_slopes;
};

class FlatTileFactory final : public SlopesBasedModelTile {
    Slopes model_tile_elevations() const final
        { return Slopes{0, 0, 0, 0, 0}; }
};

} // end of <anonymous> namespace

TileFactory::NeighborInfo::NeighborInfo(
    const SharedPtr<const TileSet> & ts, const Grid<int> & layer,
    Vector2I tilelocmap, Vector2I spawner_offset):
    NeighborInfo(*ts, layer, tilelocmap, spawner_offset)
{}

TileFactory::NeighborInfo::NeighborInfo
    (const TileSet & ts, const Grid<int> & layer,
     Vector2I tilelocmap, Vector2I spawner_offset):
    m_tileset(ts), m_layer(layer), m_loc(tilelocmap),
    m_offset(spawner_offset) {}

/* static */ TileFactory::NeighborInfo
    TileFactory::NeighborInfo::make_no_neighbor()
{
    static Grid<int> s_layer;
    static TileSet s_tileset;
    return NeighborInfo{s_tileset, s_layer, Vector2I{}, Vector2I{}};
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
    if (!m_layer.has_position(r + m_loc)) return k_inf;
    const auto * ts = m_tileset(m_layer(r + m_loc));
    switch (dir) {
    case Cd::n: case Cd::s: case Cd::e: case Cd::w:
        throw InvArg{"Not a corner"};
    case Cd::nw: return ts ? ts->tile_elevations().nw : k_inf;
    case Cd::sw: return ts ? ts->tile_elevations().sw : k_inf;
    case Cd::se: return ts ? ts->tile_elevations().se : k_inf;
    case Cd::ne: return ts ? ts->tile_elevations().ne : k_inf;
    default: break;
    }
    throw BadBranchException{__LINE__, __FILE__};
}

// ----------------------------------------------------------------------------

/* static */ UniquePtr<TileFactory> TileFactory::make_tileset_factory
    (const char * type)
{
    using TlPtr = UniquePtr<TileFactory>;
    using FnPtr = UniquePtr<TileFactory>(*)();
    using std::make_pair;
    static const std::map<std::string, FnPtr> k_factory_map {
        make_pair("flat"    , [] { return TlPtr{make_unique<FlatTileFactory    >()}; }),
        make_pair("ramp"    , [] { return TlPtr{make_unique<TwoRampTileFactory >()}; }),
        make_pair("in-ramp" , [] { return TlPtr{make_unique<InRampTileFactory  >()}; }),
        make_pair("out-ramp", [] { return TlPtr{make_unique<OutRampTileFactory >()}; }),
        make_pair("wall"    , [] { return TlPtr{make_unique<WallTileFactory    >()}; })
    };
    auto itr = k_factory_map.find(type);
    if (itr == k_factory_map.end()) return nullptr;
    return (*itr->second)();
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

/* protected */ std::array<Vector2, 4> TileFactory::common_texture_positions_from
    (Vector2I ts_r) const
{
    const Real x_scale = m_tile_size.width  / m_texture_size.width ;
    const Real y_scale = m_tile_size.height / m_texture_size.height;
    const std::array<Vector2, 4> k_base = {
        // for textures not physical locations
        Vector2{ 0*x_scale, 0*y_scale }, // nw
        Vector2{ 0*x_scale, 1*y_scale }, // sw
        Vector2{ 1*x_scale, 1*y_scale }, // se
        Vector2{ 1*x_scale, 0*y_scale }  // ne
    };
    Vector2 origin{ts_r.x*x_scale, ts_r.y*y_scale};
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

namespace { // ----------------------------------------------------------------

void SlopesBasedModelTile::operator ()
    (EntityAndTrianglesAdder & adder, const NeighborInfo & ninfo,
     Platform::ForLoaders & platform) const
{
    auto r = ninfo.tile_location();
    add_triangles_based_on_model_details(r, adder);
    adder.add_entity(make_entity(platform, r));
}

/* protected */ void SlopesBasedModelTile::add_triangles_based_on_model_details
    (Vector2I gridloc, EntityAndTrianglesAdder & adder) const
{
    TileFactory::add_triangles_based_on_model_details
        (gridloc, translation(), model_tile_elevations(), adder);
}

/* protected */ void SlopesBasedModelTile::setup
    (Vector2I loc_in_ts, const tinyxml2::XMLElement * properties,
     Platform::ForLoaders & platform)
{
    TranslatableTileFactory::setup(loc_in_ts, properties, platform);
    m_render_model = make_render_model_with_common_texture_positions(
        platform, model_tile_elevations(), loc_in_ts);
}

// --------------------------- <anonymous> namespace --------------------------

template <typename T>
/* static */ Tuple<const std::vector<Vector> &, const std::vector<unsigned> &>
    Ramp::get_model_positions_and_elements_(const Slopes & slopes)
{
    // force unique per class
    static std::vector<Vector> s_positions;
    if (!s_positions.empty()) {
        return Tuple<const std::vector<Vector> &, const std::vector<unsigned> &>
            {s_positions, TileGraphicGenerator::get_common_elements()};
    }
    auto pts = TileGraphicGenerator::get_points_for(slopes);
    s_positions = std::vector<Vector>{pts.begin(), pts.end()};
    return get_model_positions_and_elements_<T>(slopes);
}

/* protected */ void Ramp::setup
    (Vector2I loc_in_ts, const tinyxml2::XMLElement * properties,
     Platform::ForLoaders & platform)
{
    if (const auto * val = find_property("direction", properties)) {
        set_direction(val);
    }
    SlopesBasedModelTile::setup(loc_in_ts, properties, platform);
}

// --------------------------- <anonymous> namespace --------------------------

/* protected */ void CornerRamp::set_direction(const char * dir) {
    using Cd = CardinalDirection;
    int n = [dir] {
        switch (cardinal_direction_from(dir)) {
        case Cd::nw: return 0;
        case Cd::sw: return 1;
        case Cd::se: return 2;
        case Cd::ne: return 3;
        default: throw InvArg{""};
        }
    } ();
    m_slopes = half_pi_rotations(non_rotated_slopes(), n);
}

// --------------------------- <anonymous> namespace --------------------------

/* private */ void TwoRampTileFactory::set_direction(const char * dir) {
    static const Slopes k_non_rotated_slopes{0, 1, 1, 0, 0};
    using Cd = CardinalDirection;
    int n = [dir] {
        switch (cardinal_direction_from(dir)) {
        case Cd::n: return 0;
        case Cd::w: return 1;
        case Cd::s: return 2;
        case Cd::e: return 3;
        default: throw InvArg{""};
        }
    } ();
    m_slopes = half_pi_rotations(k_non_rotated_slopes, n);
}

} // end of <anonymous> namespace
