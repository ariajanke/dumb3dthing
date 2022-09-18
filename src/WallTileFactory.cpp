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

#include <cstring>

namespace {

using namespace cul::exceptions_abbr;
using Triangle = TriangleSegment;

enum SplitOpt {
    k_flats_only = 1 << 0,
    k_wall_only  = 1 << 1,
    k_both_flats_and_wall = k_flats_only | k_wall_only
};

template <SplitOpt k_opt, typename Func>
void east_west_split
    (Real north_east_y, Real north_west_y,
     Real south_east_y, Real south_west_y,
     Real division_z, Func && f);

// should handle division == 0, == 1
// everything around
// {-0.5, x,  0.5}, {0.5, x,  0.5}
// {-0.5, x, -0.5}, {0.5, x, -0.5}
template <SplitOpt k_opt, typename Func>
void north_south_split
    (Real north_east_y, Real north_west_y,
     Real south_east_y, Real south_west_y,
     Real division_z, Func && f);

template <SplitOpt k_opt, typename Func>
void south_north_split
    (Real south_west_y, Real south_east_y,
     Real north_west_y, Real north_east_y,
     Real division_z, Func && f);

template <SplitOpt k_opt, typename Func>
void west_east_split
    (Real west_south_y, Real west_north_y,
     Real east_south_y, Real east_north_y,
     Real division_x, Func && f);

template <SplitOpt k_opt, typename Func>
void nw_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, Func && f);

} // end of <anonymous> namespace

void TranslatableTileFactory::setup
    (Vector2I, const tinyxml2::XMLElement * properties, Platform::ForLoaders &)
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

/* static */ WallElevationAndDirection WallTileFactory::elevations_and_direction
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

/* static */ int WallTileFactory::corner_index(CardinalDirection dir) {
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

/* private */ void WallTileFactory::setup
    (Vector2I loc_in_ts, const tinyxml2::XMLElement * properties, Platform::ForLoaders & platform)
{
    TranslatableTileFactory::setup(loc_in_ts, properties, platform);
    m_dir = cardinal_direction_from(find_property("direction", properties));
    m_tileset_location = loc_in_ts;
}

/* private */ Slopes WallTileFactory::tile_elevations() const {
    // it is possible that some elevations are indeterminent...
    Real y = translation().y + 1;
    return Slopes{0, y, y, y, y};
}

/* private */ void WallTileFactory::make_tile
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
    auto [render_model, triangles] = make_render_model_and_triangles(wed, ninfo, platform); {}
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

/* private */ void WallTileFactory::make_entities_and_triangles(
    EntityAndTrianglesAdder & adder,
    Platform::ForLoaders & platform,
    const NeighborInfo & ninfo,
    const SharedPtr<const RenderModel> & render_model,
    const std::vector<Triangle> & triangles) const
{
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
        adder.add_triangle(Triangle{triangle.move(translation())});
    }
    // mmm
    // make_entity(platform, ninfo.tile_location(), render_model);
}

/* private static */ std::array<bool, 4> WallTileFactory::make_known_corners(CardinalDirection dir) {
    using Cd = CardinalDirection;
    auto mk_rv = [](bool nw, bool sw, bool se, bool ne) {
        std::array<bool, 4> rv;
        const std::array k_corners = {
            make_tuple(Cd::nw, nw), make_tuple(Cd::ne, ne),
            make_tuple(Cd::sw, sw), make_tuple(Cd::se, se),
        };
        for (auto [corner, val] : k_corners)
            rv[corner_index(corner)] = !val;
        return rv;
    };
    switch (dir) {
    // a north wall, all point on the north are known
    case Cd::n : return mk_rv(true , false, false, true );
    case Cd::s : return mk_rv(false, true , true , false);
    case Cd::e : return mk_rv(false, false, true , true );
    case Cd::w : return mk_rv(true , true , false, false);
    case Cd::nw: return mk_rv(true , false, false, false);
    case Cd::sw: return mk_rv(false, true , false, false);
    case Cd::se: return mk_rv(false, false, true , false);
    case Cd::ne: return mk_rv(false, false, false, true );
    default: break;
    }
    throw BadBranchException{__LINE__, __FILE__};
}

/* private */ WallElevationAndDirection WallTileFactory::elevations_and_direction
    (const NeighborInfo & ninfo) const
{
    return WallTileFactory::elevations_and_direction
        (ninfo, translation().y + 1, m_dir, m_tileset_location);
}

/* private */ Tuple<SharedPtr<const RenderModel>, std::vector<Triangle>>
    WallTileFactory::make_render_model_and_triangles(
    const WallElevationAndDirection & wed,
    const NeighborInfo & ninfo,
    Platform::ForLoaders &) const
{
    auto offset = grid_position_to_v3(ninfo.tile_location())
        + translation() + Vector{0, 1, 0};

    std::vector<Triangle> triangles;
    auto add_triangle = [&triangles] (const Triangle & triangle)
        { triangles.push_back(triangle); };

    // we have cases depending on the number of dips
    // okay, I should like to handle only two (adjacent) and three dips
    using Cd = CardinalDirection;
    static constexpr const auto k_adjusted_thershold =
        k_physical_dip_thershold - 0.5;

    // wall dips on cases where no dips occur, will be zero, and therefore
    // quite usable despite the use of subtraction here
    Real ne, nw, se, sw;
    const auto corner_pairs = {
        make_tuple(Cd::ne, &ne), make_tuple(Cd::nw, &nw),
        make_tuple(Cd::se, &se), make_tuple(Cd::sw, &sw)
    };
    for (auto [dir, ptr] : corner_pairs)
        { *ptr = offset.y - wed.dip_heights[corner_index(dir)]; }

    // nvm, direction describes which case
    switch (m_dir) {
    // a north wall, all point on the north are known
    case Cd::n:
        north_south_split<k_both_flats_and_wall>(
            ne, nw, se, sw, k_adjusted_thershold, add_triangle);
        break;
    case Cd::s:
        south_north_split<k_both_flats_and_wall>(
            sw, se, nw, ne, k_adjusted_thershold, add_triangle);
        break;
    case Cd::e:
        east_west_split<k_both_flats_and_wall>(
            ne, se, nw, sw, k_adjusted_thershold, add_triangle);
        break;
    case Cd::w:
        west_east_split<k_both_flats_and_wall>(
            sw, nw, se, ne, k_adjusted_thershold, add_triangle);
        break;
    // maybe have seperate divider functions for these cases?
    case Cd::nw:
        // corners are pretty interesting, and they're already half way
        // solved
        //
        // north west corner
        // we can still frame this with four corners
        break;
    case Cd::sw: ;
    case Cd::se: ;
    case Cd::ne: ;
        throw RtError{"unimplemented"};
    default: break;
    }
    return make_tuple(nullptr, std::move(triangles));
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

namespace { // ----------------------------------------------------------------

template <typename Func>
void make_linear_triangle_strip
    (const Vector & a_start, const Vector & a_last,
     const Vector & b_start, const Vector & b_last,
     Real step, Func && f);

template <SplitOpt k_opt, typename Func>
void east_west_split
    (Real east_north_y, Real east_south_y,
     Real west_north_y, Real west_south_y,
     Real division_x, Func && f)
{
    // simply switch roles
    // east <-> north
    // west <-> south
    auto remap_vector = [] (const Vector & r) { return Vector{r.z, r.y, r.x}; };
    north_south_split<k_opt>(
        east_north_y, east_south_y, west_north_y, west_south_y,
        division_x,
        [f = std::move(f), remap_vector] (const Triangle & tri)
    {
        f(Triangle{
            remap_vector(tri.point_a()),
            remap_vector(tri.point_b()),
            remap_vector(tri.point_c())
        });
    });
}

template <SplitOpt k_opt, typename Func>
void north_south_split
    (Real north_east_y, Real north_west_y,
     Real south_east_y, Real south_west_y,
     Real division_z, Func && f)
{
    // division z must make sense
    assert(division_z >= -0.5 && division_z <= 0.5);

    // both sets of y values' directions must be the same
    assert((north_east_y - north_west_y)*(south_east_y - south_west_y) >= 0);

    const Vector div_nw{-0.5, north_west_y, division_z};
    const Vector div_ne{ 0.5, north_east_y, division_z};

    const Vector div_sw{-0.5, south_west_y, division_z};
    const Vector div_se{ 0.5, south_east_y, division_z};
    // We must handle division_z being 0.5
    if constexpr (k_opt & k_flats_only) {
        Vector nw{-0.5, north_west_y, 0.5};
        Vector ne{ 0.5, north_east_y, 0.5};
        make_linear_triangle_strip(nw, div_nw, ne, div_ne, 1, f);
        Vector sw{-0.5, south_west_y, -0.5};
        Vector se{ 0.5, south_east_y, -0.5};
        make_linear_triangle_strip(div_sw, sw, div_se, se, 1, f);
    }
    // We should only skip triangles along the wall if
    // there's no elevation difference to cover
    if constexpr (k_opt & k_wall_only) {
        make_linear_triangle_strip(div_nw, div_sw, div_ne, div_se, 1, f);
    }
}

template <SplitOpt k_opt, typename Func>
void south_north_split
    (Real south_west_y, Real south_east_y,
     Real north_west_y, Real north_east_y,
     Real division_z, Func && f)
{
    auto z = 1 - (division_z + 0.5);
    north_south_split<k_opt>(
        north_east_y, north_west_y, south_east_y, south_west_y, z,
        std::move(f));
}

template <SplitOpt k_opt, typename Func>
void west_east_split
    (Real west_south_y, Real west_north_y,
     Real east_south_y, Real east_north_y,
     Real division_x, Func && f)
{
    auto x = 1 - (division_x + 0.5);
    north_south_split<k_opt>(
        east_north_y, east_south_y, west_north_y, west_south_y, x,
        std::move(f));
}

template <SplitOpt k_opt, typename Func>
void northwest_corner_split
    (Real north_west_y, Real north_east_y,
     Real south_west_y, Real south_east_y,
     Real division_xz, Func && f);

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
        f(Triangle{itr_a, itr_b, new_a});
        f(Triangle{itr_a, new_a, new_b});
        itr_a = new_a;
        itr_b = new_b;
    }

    if (!are_very_close(itr_a, a_last)) {
        f(Triangle{itr_a, b_last, a_last});
    } else if (!are_very_close(itr_b, b_last)) {
        f(Triangle{itr_b, itr_a, b_last});
    }
}

} // end of <anonymous> namespace
