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

#include "WallTileFactory.hpp"
#include "tiled-map-loader.hpp"
#include "RenderModel.hpp"

#include <numeric>

#include <cstring>

namespace {

using namespace cul::exceptions_abbr;
using Triangle = TriangleSegment;
using TriangleAdder = WallTileFactory::TriangleAdder;

using SplitOpt = WallTileFactory::SplitOpt;

constexpr const auto k_flats_only          =
    static_cast<SplitOpt>(WallTileFactory::k_bottom_only | WallTileFactory::k_top_only);
constexpr const auto k_bottom_only         = WallTileFactory::k_bottom_only;
constexpr const auto k_top_only            = WallTileFactory::k_top_only;
constexpr const auto k_wall_only           = WallTileFactory::k_wall_only;
constexpr const auto k_both_flats_and_wall = WallTileFactory::k_both_flats_and_wall;

void east_west_split
    (Real north_east_y, Real north_west_y,
     Real south_east_y, Real south_west_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f);

// should handle division == 0, == 1
// everything around
// {-0.5, x,  0.5}, {0.5, x,  0.5}
// {-0.5, x, -0.5}, {0.5, x, -0.5}
void north_south_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f);

void south_north_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f);

void west_east_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_x, SplitOpt opt, const TriangleAdder & f);

void northwest_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void southwest_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void northeast_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

void southeast_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f);

} // end of <anonymous> namespace

void TranslatableTileFactory::setup
    (Vector2I, const TiXmlElement * properties, Platform::ForLoaders &)
{
    // eugh... having to run through elements at a time
    // not gonna worry about it this iteration
    if (const auto * val = find_property("translation", properties)) {
        auto list = { &m_translation.x, &m_translation.y, &m_translation.z };
        auto itr = list.begin();
        for (auto value_str : split_range(val, val + ::strlen(val),
                                          is_comma, make_trim_whitespace<const char *>()))
        {
            bool is_num = cul::string_to_number(value_str.begin(), value_str.end(), **itr);
            assert(is_num);
            ++itr;
        }
    }
}

/* protected */ Entity
    TranslatableTileFactory::make_entity
    (Platform::ForLoaders & platform, Vector2I tile_loc,
     SharedCPtr<RenderModel> model_ptr) const
{
    return TileFactory::make_entity(platform,
        m_translation + grid_position_to_v3(tile_loc), model_ptr);
}

// ----------------------------------------------------------------------------

/* static */ void WallTileFactoryBase::add_wall_triangles_to
    (CardinalDirection dir, Real nw, Real sw, Real se, Real ne, SplitOpt opt,
     Real div, const TriangleAdder & add_f)
{
    // how will I do texture coords?
    using Cd = CardinalDirection;

    const auto splitter_func = [dir] {
        switch (dir) {
        case Cd::n : return north_south_split     ;
        case Cd::s : return south_north_split     ;
        case Cd::e : return east_west_split       ;
        case Cd::w : return west_east_split       ;
        case Cd::nw: return northwest_corner_split;
        case Cd::sw: return southwest_corner_split;
        case Cd::se: return southeast_corner_split;
        case Cd::ne: return northeast_corner_split;
        default: throw InvArg{"WallTileFactory::add_wall_triangles_to: direction is not a valid value."};
        }
    } ();
    splitter_func(nw, ne, sw, se, div, opt, add_f);
#   if 0
    switch (dir) {
    case Cd::n : north_south_split     (nw, ne, sw, se, div, opt, add_f); return;
    case Cd::s : south_north_split     (nw, ne, sw, se, div, opt, add_f); return;
    case Cd::e : east_west_split       (nw, ne, sw, se, div, opt, add_f); return;
    case Cd::w : west_east_split       (nw, ne, sw, se, div, opt, add_f); return;
    case Cd::nw: northwest_corner_split(nw, ne, sw, se, div, opt, add_f); return;
    case Cd::sw: southwest_corner_split(nw, ne, sw, se, div, opt, add_f); return;
    case Cd::se: southeast_corner_split(nw, ne, sw, se, div, opt, add_f); return;
    case Cd::ne: northeast_corner_split(nw, ne, sw, se, div, opt, add_f); return;
    default: break;
    }
    throw InvArg{"WallTileFactory::add_wall_triangles_to: direction is not a valid value."};
#   endif
}

/* static */ int WallTileFactoryBase::corner_index(CardinalDirection dir) {
    using Cd = CardinalDirection;
    switch (dir) {
    case Cd::nw: return 0;
    case Cd::sw: return 1;
    case Cd::se: return 2;
    case Cd::ne: return 3;
    default: break;
    }
    throw InvArg{""};
}

/* static */ WallElevationAndDirection WallTileFactoryBase::elevations_and_direction
    (const NeighborInfo & ninfo, Real known_elevation,
     CardinalDirection dir, Vector2I tile_loc)
{
    WallElevationAndDirection rv;
    // I need a way to address each corner...
    using Cd = CardinalDirection;
    rv.direction = dir;
    rv.tileset_location = tile_loc;
    auto known_corners = make_known_corners(rv.direction);
    for (auto corner : { Cd::nw, Cd::sw, Cd::se, Cd::ne }) {
        // walls are only generated for dips on "unknown corners"
        // if a neighbor elevation is unknown, then no wall it created for that
        // corner (which can very easily mean no wall are generated on any "dip"
        // corner
        bool is_known = known_corners[corner_index(corner)];
        Real neighbor_elevation = ninfo.neighbor_elevation(corner);
        Real diff   = known_elevation - neighbor_elevation;
        bool is_dip =    cul::is_real(neighbor_elevation)
                      && known_elevation > neighbor_elevation
                      && !known_corners[corner_index(corner)];
        // must be finite for our purposes
        rv.dip_heights[corner_index(corner)] = is_dip ? diff : 0;
    }

    return rv;
}


/* private */ void WallTileFactoryBase::setup
    (Vector2I loc_in_ts, const tinyxml2::XMLElement * properties, Platform::ForLoaders & platform)
{
    TranslatableTileFactory::setup(loc_in_ts, properties, platform);
    m_dir = cardinal_direction_from(find_property("direction", properties));
    m_tileset_location = loc_in_ts;
}

/* private */ Slopes WallTileFactoryBase::tile_elevations() const {
    // it is possible that some elevations are indeterminent...
    Real y = translation().y + 1;
    // first implementation should fail the "three corners facing each other"
    // test
    using Cd = CardinalDirection;
    auto knowns = make_known_corners(m_dir);
    return Slopes{0,
        knowns[corner_index(Cd::ne)] ? y : k_inf,
        knowns[corner_index(Cd::nw)] ? y : k_inf,
        knowns[corner_index(Cd::sw)] ? y : k_inf,
        knowns[corner_index(Cd::se)] ? y : k_inf};
}

/* private */ void WallTileFactoryBase::make_tile
    (EntityAndTrianglesAdder & adder, const NeighborInfo & ninfo,
     Platform::ForLoaders & platform) const
{
    auto wed = elevations_and_direction(ninfo);
    if (m_render_model_cache) {
        auto itr = m_render_model_cache->find(wed);
        if (itr != m_render_model_cache->end()) {
            const auto & [render_model, triangles] = itr->second; {}
            return make_entities_and_triangles(adder, platform, ninfo, render_model, triangles);
        }
    }
    auto [render_model, triangles] = make_render_model_and_triangles(wed, platform); {}
    if (m_render_model_cache) {
        m_render_model_cache->insert(std::make_pair(
            wed,
            make_tuple(std::move(render_model), std::move(triangles))
        ));
        assert(m_render_model_cache->find(wed) != m_render_model_cache->end());
        return make_tile(adder, ninfo, platform);
    }
    // in case there's no cache...
    make_entities_and_triangles(adder, platform, ninfo, render_model, triangles);
}

/* private */ void WallTileFactoryBase::make_entities_and_triangles(
    EntityAndTrianglesAdder & adder,
    Platform::ForLoaders & platform,
    const NeighborInfo & ninfo,
    const SharedPtr<const RenderModel> & render_model,
    const std::vector<Triangle> & triangles) const
{
    auto spawn_offset = /*translation() +*/ Vector{0, 1, 0}
        + grid_position_to_v3(ninfo.tile_location());
    for (auto & triangle : triangles) {
#           if 0
        // | there's *got* to be a way to generate this stuff using a
        // v "shared" deletor

        auto triangle_vec = new std::vector<Triangle>{triangles};
        auto del = [triangle_vec] (Triangle *) { delete triangle_vec; };
        for (auto & triangle : *triangle_vec) {
            SharedPtr<const Triangle>{&triangle, del};
        }
        //SharedPtr
#           endif
        adder.add_triangle(triangle.move(spawn_offset));
    }
    auto e = platform.make_renderable_entity();

    e.add<SharedPtr<const RenderModel>, SharedPtr<const Texture>, Translation, Visible>() =
        make_tuple(render_model, common_texture(), Translation{spawn_offset}, true);
    adder.add_entity(e);
}

/* private static */ std::array<bool, 4> WallTileFactoryBase::make_known_corners(CardinalDirection dir) {
    using Cd = CardinalDirection;
    auto mk_rv = [](bool nw, bool sw, bool se, bool ne) {
        std::array<bool, 4> rv;
        const std::array k_corners = {
            make_tuple(Cd::nw, nw), make_tuple(Cd::ne, ne),
            make_tuple(Cd::sw, sw), make_tuple(Cd::se, se),
        };
        for (auto [corner, val] : k_corners)
            rv[corner_index(corner)] = val;
        return rv;
    };
    switch (dir) {
    // a north wall, all point on the south are known
    case Cd::n : return mk_rv(false, true , true , false);
    case Cd::s : return mk_rv(true , false, false, true );
    case Cd::e : return mk_rv(true , true , false, false);
    case Cd::w : return mk_rv(false, false, true , true );
    case Cd::nw: return mk_rv(false, false, true , false);
    case Cd::sw: return mk_rv(false, false, false, true );
    case Cd::se: return mk_rv(true , false, false, false);
    case Cd::ne: return mk_rv(false, true , false, false);
    default: break;
    }
    throw BadBranchException{__LINE__, __FILE__};
}

/* private */ WallElevationAndDirection WallTileFactoryBase::elevations_and_direction
    (const NeighborInfo & ninfo) const
{
    return WallTileFactory::elevations_and_direction
        (ninfo, translation().y + 1, m_dir, m_tileset_location);
}

/* private */ Tuple<SharedPtr<const RenderModel>, std::vector<Triangle>>
    WallTileFactoryBase::make_render_model_and_triangles
    (const WallElevationAndDirection & wed,
     Platform::ForLoaders & platform) const
{
    using Cd = CardinalDirection;

    // nothing here should move to their final position

    auto add_wall_triangles_to_ = [this, &wed] {
        // wall dips on cases where no dips occur, will be zero, and therefore
        // quite usable despite the use of subtraction here
        Real ne, nw, se, sw;
        const auto corner_pairs = {
            make_tuple(Cd::ne, &ne), make_tuple(Cd::nw, &nw),
            make_tuple(Cd::se, &se), make_tuple(Cd::sw, &sw)
        };
        auto offset = translation().y;
        for (auto [dir, ptr] : corner_pairs)
            { *ptr = offset - wed.dip_heights[corner_index(dir)]; }

        return [nw, sw, se, ne, this]
               (SplitOpt opt, Real div, const TriangleAdder & adder)
        {
            WallTileFactory::add_wall_triangles_to
                (m_dir, nw, sw, se, ne, opt, div, adder);
        };
    } ();

    // how will I do texture coords?
    std::vector<Triangle> triangles;
    add_wall_triangles_to_(k_both_flats_and_wall, k_physical_dip_thershold,
        TriangleAdder::make(
            [&triangles] (const Triangle & triangle)
            { triangles.push_back(triangle); }));

    std::vector<Vertex> verticies;
    std::vector<unsigned> elements;

    auto to_tx_coord = [this] {
        auto origin = common_texture_origin(m_tileset_location);
        auto scale  = common_texture_tile_size();
        return [origin, scale](Vector pos) {
            auto local_pos = Vector2{pos.x + 0.5, -pos.z + 0.5};
            Vector2 scaled_pos{local_pos.x*scale.width, local_pos.y*scale.height};
            return origin + scaled_pos;
        };
    } ();
    auto to_vtx = [&to_tx_coord](Vector pos)
        { return Vertex{pos, to_tx_coord(pos)}; };
    add_wall_triangles_to_(k_flats_only, k_visual_dip_thershold,
        TriangleAdder::make([&verticies, to_vtx]
        (const Triangle & triangle)
    {
        // triangle x-zs correspond pretty closely to texture
        verticies.push_back(to_vtx(triangle.point_a()));
        verticies.push_back(to_vtx(triangle.point_b()));
        verticies.push_back(to_vtx(triangle.point_c()));
    }));
    // x or z, it depends
    // v assumes is wall already (and co-planar)
    static auto is_x_axis_aligned = [](const Triangle & triangle) {
        return are_very_close(triangle.point_a().z, triangle.point_b().z);
    };
    auto to_wall_tx_coord = [] (const TileTexture & tile_texture) {
        return [&tile_texture] (Vector pos, bool use_x_axis) {
            // how do I want to restrict the texture rectangle?
            using std::fmod;
            // why isn't nextafter constexpr?!
            static const Real k_after_one = std::nextafter(Real(1), Real(2));
            Vector2 txr{fmod(magnitude((use_x_axis ? pos.x : pos.z) - Real(0.5)), k_after_one),
                        fmod(magnitude(pos.y), k_after_one)};
            auto x = tile_texture.sw.x*txr.x + tile_texture.ne.x*(1 - txr.x);
            auto y = tile_texture.sw.y*txr.y + tile_texture.ne.y*(1 - txr.y);
            return Vertex{pos, Vector2{x, y}};
        };
    } (*m_wall_texture_coords);
    // walls are a bit trickier
    add_wall_triangles_to_(k_wall_only, k_visual_dip_thershold,
        TriangleAdder::make([&verticies, to_wall_tx_coord]
        (const Triangle & triangle)
    {
        bool use_x_axis = is_x_axis_aligned(triangle);
        verticies.push_back(to_wall_tx_coord(triangle.point_a(), use_x_axis));
        verticies.push_back(to_wall_tx_coord(triangle.point_b(), use_x_axis));
        verticies.push_back(to_wall_tx_coord(triangle.point_c(), use_x_axis));
    }));
    elements.resize(verticies.size());
    std::iota(elements.begin(), elements.end(), 0);
    auto model = platform.make_render_model();
    model->load(verticies, elements);

    return make_tuple(model, std::move(triangles));
}

CardinalDirection cardinal_direction_from(const char * str) {
    auto seq = [str](const char * s) { return !::strcmp(str, s); };
    using Cd = CardinalDirection;
    if (seq("n" )) return Cd::n;
    if (seq("s" )) return Cd::s;
    if (seq("e" )) return Cd::e;
    if (seq("w" )) return Cd::w;
    if (seq("ne")) return Cd::ne;
    if (seq("nw")) return Cd::nw;
    if (seq("se")) return Cd::se;
    if (seq("sw")) return Cd::sw;
    throw InvArg{""};
}

// ------------------------------- <! messy !> --------------------------------

/* private static */ const TileSet::TileTexture WallTileFactoryBaseN::s_default_texture = [] {
    Size2 scale{1. / 9., 1. / 8.};
    Vector2 offset{2*scale.width, 6*scale.height};
    TileSet::TileTexture tx;
    tx.nw = offset;
    tx.ne = offset + Vector2{scale.width, 0};
    tx.sw = offset + Vector2{0, scale.height};
    tx.se = offset + Vector2{scale.width, scale.height};
    return tx;
} ();

namespace wall {

using TileTexture = TileSet::TileTexture;

static const Real k_after_one = std::nextafter(Real(1), Real(2));

bool is_x_axis_aligned(const Triangle & triangle) {
    return are_very_close(triangle.point_a().z, triangle.point_b().z);
}

Vector2 to_x_ways_texture_vertex(const Vector & r) {
    return Vector2{fmod(magnitude(r.x - Real(0.5)), k_after_one),
                   fmod(magnitude(r.y), k_after_one)};
}

Vector2 to_z_ways_texture_vertex(const Vector & r) {
    return Vector2{fmod(magnitude(r.z - Real(0.5)), k_after_one),
                   fmod(magnitude(r.y), k_after_one)};
}

std::array<Vertex, 3> to_verticies(const Triangle & triangle) {
    auto get_tx_x = [&triangle] {
        bool use_x_axis = is_x_axis_aligned(triangle);
        return [use_x_axis] (const Vector & r) {
            return use_x_axis ? r.x : r.y;
        };
    } ();

    auto to_tex = [&get_tx_x] (const Vector & r) {
        // even/odd is the way to go I think?
        // mmm least value of the triangle?
        return Vector2{fmod(magnitude(get_tx_x(r) - Real(0.5)), k_after_one),
                       fmod(magnitude(r.y), k_after_one)};
    };

    auto a = triangle.point_a();
    auto b = triangle.point_b();
    auto c = triangle.point_c();
    return std::array {
        Vertex{a, to_tex(a)},
        Vertex{b, to_tex(b)},
        Vertex{c, to_tex(c)}
    };
}

Vertex map_to_texture(const Vertex & vtx, const TileTexture & txt) {
    auto tx = vtx.texture_position;
    return Vertex{vtx.position, Vector2{
        tx.x*txt.se.x + txt.nw.x*(1 - tx.x),
        tx.y*txt.se.y + txt.nw.y*(1 - tx.y)
    }};
}

std::array<Vertex, 3> map_to_texture(std::array<Vertex, 3> arr, const TileTexture & txt) {
    auto tf = [&txt](const Vertex & vtx)
        { return map_to_texture(vtx, txt); };
    std::transform(arr.begin(), arr.end(), arr.begin(), tf);
    return arr;
}

}

WallTileFactoryBaseN::TileTexture WallTileFactoryBaseN::floor_texture() const {
    static constexpr Size2 k_scale{1. / 9., 1. / 8.};
    TileTexture rv;
    rv.nw = Vector2(m_tileset_location.x*k_scale.width,
                    m_tileset_location.y*k_scale.height);
    rv.se = rv.nw + Vector2{k_scale.width, k_scale.height};
    rv.ne = rv.nw + Vector2{k_scale.width, 0};
    rv.sw = rv.nw + Vector2{0, k_scale.height};
    return rv;
}

/* private static */ TwoWayWallTileFactory::GraphicMap TwoWayWallTileFactory::s_cache;
/* private static */ TwoWayWallTileFactory::GraphicMap TwoWayWallTileFactory::s_bottom_cache;

SharedPtr<const RenderModel>
    TwoWayWallTileFactory::make_top_model(Platform::ForLoaders & platform) const
{
    return make_model_graphics(
        tile_elevations(), k_top_only,
        make_triangle_to_floor_verticies(), platform);
}

SharedPtr<const RenderModel>
    TwoWayWallTileFactory::make_wall_graphics
    (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const
{
    const auto elvs = computed_tile_elevations(neighborhood);
    auto to_verticies = make_triangle_to_verticies([this] (const Triangle & triangle) {
        auto vtxs = wall::to_verticies(triangle.move(Vector{0, -translation().y, 0}));
        return wall::map_to_texture(vtxs, wall_texture());
    });
    return make_model_graphics(elvs, k_wall_only, to_verticies, platform);
}

/* private */ SharedPtr<const RenderModel>
    TwoWayWallTileFactory::make_bottom_graphics
    (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const
{
    return make_model_graphics(
        computed_tile_elevations(neighborhood), k_bottom_only,
        make_triangle_to_floor_verticies(), platform);
}

void TwoWayWallTileFactory::make_physical_triangles
    (const NeighborInfo & neighborhood, EntityAndTrianglesAdder & adder) const
{
    auto elvs = computed_tile_elevations(neighborhood);
    auto offset = grid_position_to_v3(neighborhood.tile_location());
    make_triangles(elvs, k_physical_dip_thershold,
                   k_both_flats_and_wall,
                   TriangleAdder::make([&adder, offset] (const Triangle & triangle)
    { adder.add_triangle(triangle.move(offset)); }));
}

/* private */ SharedPtr<const RenderModel>
    TwoWayWallTileFactory::make_model_graphics
    (const Slopes & elvs, SplitOpt split_opt,
     const TriangleToVerticies & to_verticies, Platform::ForLoaders & platform) const
{
    auto mod_ptr = platform.make_render_model();
    std::vector<Vertex> verticies;
    make_triangles(elvs, k_visual_dip_thershold,
                   split_opt,
                   TriangleAdder::make([&verticies, &to_verticies] (const Triangle & triangle)
    {
        auto vtxs = to_verticies(triangle);
        verticies.insert(verticies.end(), vtxs.begin(), vtxs.end());
    }));
    std::vector<unsigned> elements;
    elements.resize(verticies.size());
    std::iota(elements.begin(), elements.end(), 0);
    mod_ptr->load(verticies, elements);
    return mod_ptr;
}

/* private */ void TwoWayWallTileFactory::make_triangles
    (const Slopes & elvs, Real thershold, SplitOpt split_opt,
     const TriangleAdder & add_triangle) const
{
    auto make_triangles = [this] {
        using Cd = CardinalDirection;
        switch (direction()) {
        case Cd::n : return north_south_split;
        case Cd::s : return south_north_split;
        case Cd::e : return east_west_split  ;
        case Cd::w : return west_east_split  ;
        default: break;
        }
        throw BadBranchException{__LINE__, __FILE__};
    } ();

    make_triangles(elvs.nw, elvs.ne, elvs.sw, elvs.se, thershold,
                   split_opt, add_triangle);
}

/* private static */ OutWallTileFactory::GraphicMap OutWallTileFactory::s_cache;
/* private static */ OutWallTileFactory::GraphicMap OutWallTileFactory::s_bottom_cache;

SharedPtr<const RenderModel>
    OutWallTileFactory::make_top_model(Platform::ForLoaders & platform) const
{
    return make_model_graphics(
        tile_elevations(), k_top_only,
        make_triangle_to_floor_verticies(), platform);
}

SharedPtr<const RenderModel>
    OutWallTileFactory::make_wall_graphics
    (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const
{
    const auto elvs = computed_tile_elevations(neighborhood);
    auto to_verticies = make_triangle_to_verticies([this] (const Triangle & triangle) {
        auto vtxs = wall::to_verticies(triangle.move(Vector{0, -translation().y, 0}));
        return wall::map_to_texture(vtxs, wall_texture());
    });
    return make_model_graphics(elvs, k_wall_only, to_verticies, platform);
}

/* private */ SharedPtr<const RenderModel>
    OutWallTileFactory::make_bottom_graphics
    (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const
{
    return make_model_graphics(
        computed_tile_elevations(neighborhood), k_bottom_only,
        make_triangle_to_floor_verticies(), platform);
}

void OutWallTileFactory::make_physical_triangles
    (const NeighborInfo & neighborhood, EntityAndTrianglesAdder & adder) const
{
    auto elvs = computed_tile_elevations(neighborhood);
    auto offset = grid_position_to_v3(neighborhood.tile_location());
    make_triangles(elvs, k_physical_dip_thershold,
                   k_both_flats_and_wall,
                   TriangleAdder::make([&adder, offset] (const Triangle & triangle)
    { adder.add_triangle(triangle.move(offset)); }));
}

/* private */ SharedPtr<const RenderModel>
    OutWallTileFactory::make_model_graphics
    (const Slopes & elvs, SplitOpt split_opt,
     const TriangleToVerticies & to_verticies, Platform::ForLoaders & platform) const
{
    auto mod_ptr = platform.make_render_model();
    std::vector<Vertex> verticies;
    make_triangles(elvs, k_visual_dip_thershold,
                   split_opt,
                   TriangleAdder::make([&verticies, &to_verticies] (const Triangle & triangle)
    {
        auto vtxs = to_verticies(triangle);
        verticies.insert(verticies.end(), vtxs.begin(), vtxs.end());
    }));
    std::vector<unsigned> elements;
    elements.resize(verticies.size());
    std::iota(elements.begin(), elements.end(), 0);
    mod_ptr->load(verticies, elements);
    return mod_ptr;
}

/* private */ void OutWallTileFactory::make_triangles
    (const Slopes & elvs, Real thershold, SplitOpt split_opt,
     const TriangleAdder & add_triangle) const
{
    auto make_triangles = [this] {
        using Cd = CardinalDirection;
        switch (direction()) {
        case Cd::ne: return northeast_corner_split;
        case Cd::nw: return northwest_corner_split;
        case Cd::se: return southeast_corner_split;
        case Cd::sw: return southwest_corner_split;
        default: break;
        }
        throw BadBranchException{__LINE__, __FILE__};
    } ();

    make_triangles(elvs.nw, elvs.ne, elvs.sw, elvs.se, thershold,
                   split_opt, add_triangle);
}

namespace { // ----------------------------------------------------------------

template <typename Func>
void make_linear_triangle_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     Real step, Func && f);

template <typename TransformingFunc, typename PassOnFunc>
/* <! auto breaks BFS ordering !> */ auto make_triangle_transformer
    (TransformingFunc && tf, const PassOnFunc & pf)
{
    return WallTileFactory::TriangleAdder::make([tf = std::move(tf), &pf](const Triangle & tri)
        { pf(Triangle{tf(tri.point_a()), tf(tri.point_b()), tf(tri.point_c())}); });
}

void east_west_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_x, SplitOpt opt, const TriangleAdder & f)
{
    // simply switch roles
    // east <-> north
    // west <-> south
    auto xz_swap_roles = [] (const Vector & r)
        { return Vector{r.z, r.y, r.x}; };
    north_south_split
        (south_east_y, north_east_y, south_west_y, north_west_y,
         division_x, opt,
         make_triangle_transformer(xz_swap_roles, f));
}

// this is a bit different, presently it makes no assumption on which is the
// floor, and which is the top
// I think I may make this function more specialized
// north is bottom, south is top
void north_south_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f)
{
    // late division, less top space, early division... more
    // in all cases, southerns must be reals
    // in all but top case, northerns must also be reals
    using cul::is_real;
    if (!is_real(south_west_y) || !is_real(south_east_y)) {
        throw InvArg{"north_south_split: Southern elevations must be real numbers in all cases"};
    }
    if (   (opt & ~k_top_only)
        && (!is_real(north_west_y) || !is_real(north_east_y)))
    {
        throw InvArg{"north_south_split: Northern elevations must be real numbers in top cases"};
    }
    if (division_z < -0.5 || division_z > 0.5) {
        throw InvArg{"north_south_split: division must be in [0.5 0.5]"};
    }
    if (   (opt & ~k_top_only)
        && (south_west_y < north_west_y || south_east_y < north_east_y))
    {
        throw InvArg{"north_south_split: method was designed assuming south is "
                     "the top"};
    }

    const Vector div_nw{-0.5, north_west_y, -division_z};
    const Vector div_sw{-0.5, south_west_y, -division_z};

    const Vector div_ne{ 0.5, north_east_y, -division_z};
    const Vector div_se{ 0.5, south_east_y, -division_z};

    if (opt & k_bottom_only) {
        Vector nw{-0.5, north_west_y, 0.5};
        Vector ne{ 0.5, north_east_y, 0.5};
        make_linear_triangle_strip(nw, div_nw, ne, div_ne, 1, f);
    }

    if (opt & k_top_only) {
        Vector sw{-0.5, south_west_y, -0.5};
        Vector se{ 0.5, south_east_y, -0.5};
        make_linear_triangle_strip(div_sw, sw, div_se, se, 1, f);
    }

    // We should only skip triangles along the wall if
    // there's no elevation difference to cover
    if (opt & k_wall_only) {
        // both sets of y values' directions must be the same
        assert((north_east_y - north_west_y)*(south_east_y - south_west_y) >= 0);
        make_linear_triangle_strip(div_nw, div_sw, div_ne, div_se, 1, f);
    }
}

void south_north_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_z, SplitOpt opt, const TriangleAdder & f)
{
    auto invert_z = [] (const Vector & r)
        { return Vector{r.x, r.y, -r.z}; };
    north_south_split
        (south_west_y, south_east_y, north_west_y, north_east_y, division_z, opt,
         make_triangle_transformer(invert_z, f));
}

void west_east_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_x, SplitOpt opt, const TriangleAdder & f)
{
    auto invert_x = [] (const Vector & r)
        { return Vector{-r.x, r.y, r.z}; };
    east_west_split
        (north_east_y, north_west_y, south_east_y, south_west_y, division_x, opt,
         make_triangle_transformer(invert_x, f));
}

void northwest_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    // se as only Real...
    using cul::is_real;
    bool can_only_do_top =
           !is_real(north_east_y) && !is_real(north_west_y)
        && !is_real(south_west_y) &&  is_real(south_east_y);
    bool are_all_real =
           is_real(north_east_y) && is_real(north_west_y)
        && is_real(south_west_y) && is_real(south_east_y);
    if (   (opt & ~k_top_only)
        && can_only_do_top)
    {
        throw InvArg{"northwest_corner_split: "};
    }
    if (   are_all_real
        && (   south_east_y < north_west_y || south_east_y < north_east_y
            || south_east_y < south_west_y))
    {
        throw InvArg{"northwest_corner_split: south_east_y is assumed to be "
                     "the top's elevation, method not explicitly written to "
                     "handle south east *not* being the top"};
    }
    // late division, less top space, early division... more
    // the top "flat's" depth/width remain equal regardless where the division
    // is placed
    // the "control" points are:
    // nw corner, floor, top
    Vector nw_corner{       -0.5, north_west_y,          0.5};
    Vector nw_floor {division_xz, north_west_y, -division_xz};
    Vector nw_top   {division_xz, south_east_y, -division_xz};
    // se
    Vector se       {        0.5, south_east_y,         -0.5};
    // ne corner, floor, top
    Vector ne_corner{        0.5, north_east_y,          0.5};
    Vector ne_floor {        0.5, north_east_y, -division_xz};
    Vector ne_top   {        0.5, south_east_y, -division_xz};
    // sw corner, floor, top
    Vector sw_corner{       -0.5, south_west_y,         -0.5};
    Vector sw_floor {division_xz, south_west_y,         -0.5};
    Vector sw_top   {division_xz, south_east_y,         -0.5};

    // both top triangles should come together or not at all
    // there is only one condition where no tops are generated:
    // - division is ~0.5
    if (opt & k_top_only && !are_very_close(division_xz, 0.5)) {
        f(Triangle{nw_top, ne_top, se});
        f(Triangle{nw_top, sw_top, se});
    }

    // four triangles for the bottom,
    // all triangles should come together, or not at all
    if (opt & k_bottom_only && !are_very_close(division_xz, -0.5)) {
        f(Triangle{nw_corner, ne_corner, ne_floor});
        // this constructor should be throwing when div ~= 0.5
        if (!are_very_close(division_xz, 0.5)) {
            f(Triangle{nw_corner, nw_floor , ne_floor});

            f(Triangle{nw_corner, nw_floor , sw_floor});
        }
        f(Triangle{nw_corner, sw_corner, sw_floor});
    }

    if (opt & k_wall_only) {
        make_linear_triangle_strip(nw_top, nw_floor, ne_top, ne_floor, 1, f);
        make_linear_triangle_strip(nw_top, nw_floor, sw_top, sw_floor, 1, f);
    }
}

// can just exploit symmetry to implement the rest
void southwest_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    auto invert_z = [](const Vector & r)
        { return Vector{r.x, r.y, -r.z}; };
    northwest_corner_split
        (south_west_y, south_east_y, north_west_y, north_east_y, division_xz,
         opt, make_triangle_transformer(invert_z, f));
}

void northeast_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    auto invert_x = [](const Vector & r)
        { return Vector{-r.x, r.y, r.z}; };
    northwest_corner_split
        (north_east_y, north_west_y, south_east_y, south_west_y, division_xz,
         opt, make_triangle_transformer(invert_x, f));
}

void southeast_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, SplitOpt opt, const TriangleAdder & f)
{
    auto invert_xz = [](const Vector & r)
        { return Vector{-r.x, r.y, -r.z}; };
    northwest_corner_split
        (south_east_y, south_west_y, north_east_y, north_west_y, division_xz,
         opt, make_triangle_transformer(invert_xz, f));
}

// ----------------------------------------------------------------------------

/* <! auto breaks BFS ordering !> */ auto
    make_get_next_for_dir_split_v
    (Vector end, Vector step)
{
    return [end, step] (Vector east_itr) {
        auto cand_next = east_itr + step;
        if (are_very_close(cand_next, end)) return cand_next;

        if (are_very_close(normalize(end - east_itr ),
                           normalize(end - cand_next)))
        { return cand_next; }
        return end;
    };
}

/* <! auto breaks BFS ordering !> */ auto make_step_factory(Real step) {
    return [step](const Vector & start, const Vector & last) {
        auto diff = last - start;
        if (are_very_close(diff, Vector{})) return Vector{};
        return step*normalize(diff);
    };
}

template <typename Func>
void make_linear_triangle_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     Real step, Func && f)
{
    if (   are_very_close(a_start, a_last)
        && are_very_close(b_start, b_last))
    { return; }

    const auto make_step = make_step_factory(step);

    auto itr_a = a_start;
    const auto next_a = make_get_next_for_dir_split_v(
        a_last, make_step(a_start, a_last));

    auto itr_b = b_start;
    const auto next_b = make_get_next_for_dir_split_v(
        b_last, make_step(b_start, b_last));

    while (   !are_very_close(itr_a, a_last)
           && !are_very_close(itr_b, b_last))
    {
        const auto new_a = next_a(itr_a);
        const auto new_b = next_b(itr_b);
        if (!are_very_close(itr_a, itr_b))
            f(Triangle{itr_a, itr_b, new_a});
        if (!are_very_close(new_a, new_b))
            f(Triangle{itr_b, new_a, new_b});
        itr_a = new_a;
        itr_b = new_b;
    }

    // at this point we are going to generate at most one triangle
    if (are_very_close(b_last, a_last)) {
        // here we're down to three points
        // there is only one possible triangle
        if (   are_very_close(itr_a, a_last)
            || are_very_close(itr_a, itr_b))
        {
            // take either being true:
            // in the best case: a line, so nothing
            return;
        }

        f(Triangle{itr_a, itr_b, a_last});
        return;
    } else {
        // a reminder from above
        assert(   are_very_close(itr_a, a_last)
               || are_very_close(itr_b, b_last));

        // here we still haven't ruled any points out
        if (   are_very_close(itr_a, itr_b)
            || (   are_very_close(itr_a, a_last)
                && are_very_close(itr_b, b_last)))
        {
            // either are okay, as they are "the same" pt
            return;
        } else if (!are_very_close(itr_a, a_last)) {
            // must exclude itr_b
            f(Triangle{itr_a, b_last, a_last});
            return;
        } else if (!are_very_close(itr_b, b_last)) {
            // must exclude itr_a
            f(Triangle{itr_b, a_last, b_last});
            return;
        }
    }
    throw BadBranchException{__LINE__, __FILE__};
}

} // end of <anonymous> namespace
