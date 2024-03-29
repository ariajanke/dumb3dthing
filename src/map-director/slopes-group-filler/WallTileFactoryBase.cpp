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

#include "WallTileFactoryBase.hpp"

#include "../ProducableGrid.hpp"
#include "../TilesetPropertiesGrid.hpp"

#include <numeric>

namespace {

using VertexArray = TriangleToFloorVerticies::VertexArray;
using Triangle = TriangleSegment;

Optional<Vector> parse_vector(const std::string & val) {
    Vector rv;
    auto list = { &rv.x, &rv.y, &rv.z };
    auto itr = list.begin();
    for (auto value_str : split_range(val.begin(), val.end(),
                                      is_comma, make_trim_whitespace<std::string::const_iterator>()))
    {
        bool is_num = cul::string_to_number(value_str.begin(), value_str.end(), **itr);
        if (!is_num) return {};
        ++itr;
    }
    return rv;
}

} // end of <anonymous> namespace

namespace wall {

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

VertexArray to_verticies(const Triangle & triangle) {
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
    return Vertex{vtx.position, txt.texture_position_for(vtx.texture_position)};
}

VertexArray map_to_texture(VertexArray arr, const TileTexture & txt) {
    auto tf = [&txt](const Vertex & vtx)
        { return map_to_texture(vtx, txt); };
    std::transform(arr.begin(), arr.end(), arr.begin(), tf);
    return arr;
}

} // end of wall namespace

/* protected */ void TranslatableTileFactory::setup_
    (const Vector2I &,
     const TileProperties & properties,
     PlatformAssetsStrategy &)
{
    // eugh... having to run through elements at a time
    // not gonna worry about it this iteration
    m_translation = *properties.for_value
        ("translation", Optional<Vector>{}, parse_vector);
}

// ----------------------------------------------------------------------------

/* private static */ Real WallTileGraphicKey::difference_between
    (const CornersArray<Real> & lhs, const CornersArray<Real> & rhs)
{
    using Cd = CardinalDirection;
    for (auto corner : { Cd::nw, Cd::ne, Cd::se, Cd::sw }) {
        auto diff = lhs[corner] - rhs[corner];
        if (!are_very_close(diff, 0))
            return diff;
    }
    return 0;
}

/* private */ int WallTileGraphicKey::compare
    (const WallTileGraphicKey & rhs) const noexcept
{
    auto diff = static_cast<int>(direction) - static_cast<int>(rhs.direction);
    if (diff) return diff;

    auto slopes_diff = difference_between(dip_heights, rhs.dip_heights);
    if (are_very_close(slopes_diff, 0))
        return 0;

    // do not truncate to zero
    return slopes_diff < 0 ? -1 : 1;
}

// ----------------------------------------------------------------------------

VertexArray TriangleToFloorVerticies::operator ()
    (const Triangle & triangle) const
{
    auto to_vtx = [this] (const Vector & r) {
        auto tx = m_ttex.texture_position_for(Vector2{ r.x + 0.5, -r.z + 0.5 });
        return Vertex{r, tx};
    };
    const auto tri_ = triangle.move(Vector{0, m_ytrans, 0});
    return VertexArray{
        to_vtx(tri_.point_a()),
        to_vtx(tri_.point_b()),
        to_vtx(tri_.point_c()),
    };
}

// ----------------------------------------------------------------------------

/* private static */ const TileTexture WallTileFactoryBase::s_default_texture =
    TileTexture{Vector2{}, Vector2{1, 1}};
/* private static */ WallTileFactoryBase::GraphicMap WallTileFactoryBase::s_wall_graphics_cache;
/* private static */ WallTileFactoryBase::GraphicMap WallTileFactoryBase::s_bottom_graphics_cache;

void WallTileFactoryBase::operator ()
    (const SlopeGroupNeighborhood & ninfo,
     ProducableTileCallbacks & callbacks) const
{
    // physical triangles
    make_physical_triangles(ninfo, callbacks);

    // top
    SharedPtr<const RenderModel> ptr{m_top_model};
    add_visual_entity_with(callbacks, std::move(ptr)).
        get<ModelTranslation>() += translation();

    // wall graphics
    add_visual_entity_with
        (callbacks, ensure_wall_graphics(ninfo, callbacks)).
        get<ModelTranslation>() += translation();

    // bottom
    add_visual_entity_with
        (callbacks, ensure_bottom_model(ninfo, callbacks)).
        get<ModelTranslation>() += translation();
}

Slopes WallTileFactoryBase::computed_tile_elevations
    (const SlopeGroupNeighborhood & ninfo) const
{
    using Cd = CardinalDirection;
    auto slopes = tile_elevations();
    auto update_corner = [&ninfo, this] (Real & x, Cd dir) {
        if (cul::is_real(x)) return;
        x = ninfo.neighbor_elevation(dir);
        if (cul::is_real(x)) return;
        // falls back to known elevation
        x = known_elevation();
    };
    update_corner(slopes.nw, Cd::nw);
    update_corner(slopes.ne, Cd::ne);
    update_corner(slopes.se, Cd::se);
    update_corner(slopes.sw, Cd::sw);
    return slopes;
}

void WallTileFactoryBase::make_physical_triangles
    (const SlopeGroupNeighborhood & neighborhood,
     ProducableTileCallbacks & callbacks) const
{
    auto elvs = computed_tile_elevations(neighborhood);
    make_triangles(
        elvs, k_physical_dip_thershold, k_both_flats_and_wall,
        TriangleAdder::make([&callbacks](const Triangle & triangle)
    { callbacks.add_collidable(triangle); }));
}

Slopes WallTileFactoryBase::tile_elevations() const {
    using Cd = CardinalDirection;
    auto is_known_corner = [this] {
        auto knowns = make_known_corners();
        return [knowns] (Cd dir) { return knowns[dir]; };
    } ();

    auto elevation_for_corner = [this, &is_known_corner] {
        Real y = known_elevation();
        return [y, &is_known_corner] (Cd dir) {
            return is_known_corner(dir) ? y : k_inf;
        };
    } ();

    return Slopes
        {elevation_for_corner(Cd::ne), elevation_for_corner(Cd::nw),
         elevation_for_corner(Cd::sw), elevation_for_corner(Cd::se)};
}

/* protected */ CardinalDirection WallTileFactoryBase::direction() const noexcept
    { return m_dir; }

/* protected */ SharedPtr<const RenderModel>
    WallTileFactoryBase::ensure_bottom_model
    (const SlopeGroupNeighborhood & neighborhood,
     ProducableTileCallbacks & callbacks) const
{
    return ensure_model
        (neighborhood, s_bottom_graphics_cache,
         [&] { return make_bottom_graphics(neighborhood, callbacks); });
}

template <typename MakerFunc>
/* protected */ SharedPtr<const RenderModel> WallTileFactoryBase::ensure_model
    (const SlopeGroupNeighborhood & neighborhood, GraphicMap & graphic_map,
     MakerFunc && make_model) const
{
    auto key = graphic_key(neighborhood);
    auto itr = graphic_map.find(key);
    if (itr == graphic_map.end()) {
        auto new_pair = std::make_pair(key, WeakPtr<const RenderModel>{});
        itr = std::get<0>(graphic_map.insert(new_pair));
    }

    if (auto rv = itr->second.lock())
        return rv;

    SharedPtr<const RenderModel> rv = make_model();
    itr->second = rv;
    return rv;
}

/* protected */ SharedPtr<const RenderModel>
    WallTileFactoryBase::ensure_wall_graphics
    (const SlopeGroupNeighborhood & neighborhood,
     ProducableTileCallbacks & callbacks) const
{
    return ensure_model
        (neighborhood, s_wall_graphics_cache,
         [&] { return make_wall_graphics(neighborhood, callbacks); });
}

/* protected */ TileTexture WallTileFactoryBase::floor_texture() const {
    return floor_texture_at(m_tileset_location);
}

/* protected */ WallTileGraphicKey WallTileFactoryBase::graphic_key
    (const SlopeGroupNeighborhood & ninfo) const
{
    WallTileGraphicKey key;
    key.direction = m_dir;

    const auto known_elevation = translation().y + 1;
    for (auto [known, corner] : make_known_corners_with_preposition()) {
        // walls are only generated for dips on "unknown corners"
        // if a neighbor elevation is unknown, then no wall it created for that
        // corner (which can very easily mean no wall are generated on any "dip"
        // corner
        Real neighbor_elevation = ninfo.neighbor_elevation(corner);
        Real diff   = known_elevation - neighbor_elevation;
        bool is_dip =    cul::is_real(neighbor_elevation)
                      && known_elevation > neighbor_elevation
                      && !known;
        // must be finite for our purposes
        key.dip_heights[corner] = is_dip ? diff : 0;
    }
    return key;
}

/* protected */ Real WallTileFactoryBase::known_elevation() const
    { return translation().y + 1; }

/* protected */ SharedPtr<const RenderModel>
    WallTileFactoryBase::make_bottom_graphics
    (const SlopeGroupNeighborhood & neighborhood,
     ProducableTileCallbacks & callbacks) const
{
    return make_model_graphics(
        computed_tile_elevations(neighborhood), k_bottom_only,
        make_triangle_to_floor_verticies(), callbacks);
}

/* protected */ std::array<Tuple<bool, CardinalDirection>, 4>
    WallTileFactoryBase::make_known_corners_with_preposition() const
{
    using Cd = CardinalDirection;
    auto knowns = make_known_corners();
    return {
        make_tuple(knowns[Cd::ne], Cd::ne),
        make_tuple(knowns[Cd::nw], Cd::nw),
        make_tuple(knowns[Cd::sw], Cd::sw),
        make_tuple(knowns[Cd::se], Cd::se),
    };
}

/* protected */ SharedPtr<const RenderModel>
    WallTileFactoryBase::make_model_graphics
    (const Slopes & elvs, SplitOpt split_opt,
     const TriangleToVerticies & to_verticies,
     ProducableTileCallbacks & callbacks) const
{
    return make_model_graphics
        (elvs, split_opt, to_verticies, callbacks.make_render_model());
}

/* protected */ SharedPtr<const RenderModel>
    WallTileFactoryBase::make_model_graphics
    (const Slopes & elevations,
     SplitOpt split_opt,
     const TriangleToVerticies & to_verticies,
     SharedPtr<RenderModel> && model_to_use) const
{
    std::vector<Vertex> verticies;
    make_triangles(elevations, k_visual_dip_thershold,
                   split_opt,
                   TriangleAdder::make([&verticies, &to_verticies] (const Triangle & triangle)
    {
        auto vtxs = to_verticies(triangle);
        verticies.insert(verticies.end(), vtxs.begin(), vtxs.end());
    }));
    std::vector<unsigned> elements;
    elements.resize(verticies.size());
    std::iota(elements.begin(), elements.end(), 0);
    model_to_use->load(verticies, elements);
    return model_to_use;
}

/* protected */ SharedPtr<const RenderModel>
    WallTileFactoryBase::make_top_model
    (PlatformAssetsStrategy & platform) const
{
    return make_model_graphics(
        tile_elevations(), k_top_only,
        make_triangle_to_floor_verticies(), platform.make_render_model());
}

/* protected */ SharedPtr<const RenderModel>
WallTileFactoryBase::make_wall_graphics
    (const SlopeGroupNeighborhood & neighborhood,
     ProducableTileCallbacks & callbacks) const
{
    const auto elvs = computed_tile_elevations(neighborhood);
    auto to_verticies = make_triangle_to_verticies([this] (const Triangle & triangle) {
        auto vtxs = wall::to_verticies(triangle.move(Vector{0, -translation().y, 0}));
        return wall::map_to_texture(vtxs, wall_texture());
    });
    return make_model_graphics(elvs, k_wall_only, to_verticies, callbacks);
}

/* private */ void WallTileFactoryBase::setup_
    (const TileProperties & properties,
     PlatformAssetsStrategy & platform,
     const SlopeFillerExtra & specials,
     const Vector2I & location_on_tileset)
{
    TranslatableTileFactory::setup_(location_on_tileset, properties, platform);

    properties.for_value("direction", [this] (const std::string & str) {
        m_dir = verify_okay_wall_direction(cardinal_direction_from(str));
    });
    specials.for_texture("wall", [this] (TileTexture tx) {
        m_wall_texture_coords = tx;
    });

    m_tileset_location = location_on_tileset;
    m_top_model = make_top_model(platform);
}

/* protected */ TileTexture WallTileFactoryBase::wall_texture() const
    { return m_wall_texture_coords; }
