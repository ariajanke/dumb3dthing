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

#include "map-loader.hpp"
#include "RenderModel.hpp"
#include "Texture.hpp"
#include "Components.hpp"

#include <ariajanke/cul/TestSuite.hpp>

namespace {

using MaybeCell = CharToCell::MaybeCell;
using Triangle = TriangleSegment;
using namespace cul::exceptions_abbr;

static const constexpr std::array k_flat_points = {
    Vector{-.5, 0, 0.5}, // nw
    Vector{-.5, 0, -.5}, // sw
    Vector{0.5, 0, -.5}, // se
    Vector{0.5, 0, 0.5}  // ne
};

Vector grid_location_to_v3(Vector2I loc, Real elevation)
    { return Vector{Real(loc.x), elevation, Real(-loc.y)}; }

Tuple<Triangle, Triangle>
    make_flat_segments(Vector2I loc, Real elevation);

TileGraphicGenerator::WallDips get_dips(CellSubGrid, Vector2I);

} // end of <anonymous> namespace

// ----------------------------------------------------------------------------

TileGraphicGenerator::TileGraphicGenerator
    (LoaderCallbacks & callbacks):
    m_callbacks(callbacks)
{}

void TileGraphicGenerator::setup() {
    get_slope_model_(Slopes{0, 1, 1, 0, 0}, Vector{});
    get_slope_model_(Slopes{0, 1, 0, 0, 0}, Vector{});
}

void TileGraphicGenerator::create_slope(TrianglesAdder & adder, Vector2I loc, const Slopes & slopes) {
    auto e = m_callbacks.platform().make_renderable_entity();
    m_callbacks.add(e);
    Translation translation{grid_location_to_v3(loc, 0)};
    auto [key_slopes, model, segment_a, segment_b] =
        get_slope_model_(sub_minimum_value(slopes), translation.value); {}
    assert(*model);
    auto rot_ = rotation_between(slopes, key_slopes);
    int near_zeros = [] (const Slopes & key_slopes) {
        std::array l = { key_slopes.ne, key_slopes.nw, key_slopes.se, key_slopes.sw };
        return std::count_if(l.begin(), l.end(), [](float x) { return are_very_close(x, 0.); });
    } (key_slopes);
    assert(cul::is_real(rot_));
    static const Vector2 tx_start{ 80. / 256., 0 };
    static constexpr const Real tx_tile_len = 16. / 256.;
    Vector2 txr = [near_zeros, rot_] {
        if (near_zeros == 2) {
            // arrows on texture are: e, w, s, n
            // I... don't know...
            if (are_very_close(rot_, 0.))
                { return tx_start + Vector2{tx_tile_len*3, tx_tile_len}; }
            if (are_very_close(rot_, -k_pi*0.5))
                { return tx_start + Vector2{tx_tile_len, tx_tile_len}; }
            if (are_very_close(rot_, -k_pi))
                { return tx_start + Vector2{tx_tile_len*2., tx_tile_len}; }
            if (are_very_close(rot_, -k_pi*1.5))
                { return tx_start + Vector2{0, tx_tile_len}; }
        } else { // assume 3
            // corners are: se, sw, ne, nw
            if (are_very_close(rot_, 0.))
                { return tx_start + Vector2{tx_tile_len*2, 0}; }
            if (are_very_close(rot_, -k_pi*0.5))
                { return tx_start + Vector2{tx_tile_len*3, 0}; }
            if (are_very_close(rot_, -k_pi))
                { return tx_start + Vector2{tx_tile_len, 0}; }
            if (are_very_close(rot_, -k_pi*1.5))
                { return tx_start + Vector2{0, 0}; }
        }
        assert(!"you've met a terrible fate, haven't you?");
    } ();
    e.add<
        SharedCPtr<RenderModel>, Translation,
        YRotation, SharedCPtr<Texture>,
        TextureTranslation>()
    = make_tuple(
        model, translation,
        YRotation{rot_}, ensure_texture(m_tileset4, "tileset4.png"),
        TextureTranslation{txr});
    adder.add_triangle(segment_a);
    adder.add_triangle(segment_b);
}

void TileGraphicGenerator::create_flat
    (TrianglesAdder & adder, Vector2I loc, const Flat & flat, const WallDips & dips)
{
    if (m_flat_model) {
        using std::get, std::tuple_cat;
        assert(m_wall_model);
        auto e = m_callbacks.platform().make_renderable_entity();// m_platform.make_renderable_entity();
        m_callbacks.add(e);// m_entities_out.push_back(e);
        Translation flat_transl{grid_location_to_v3(loc, flat.y)};
        auto ground_tex = ensure_texture(m_ground_texture, "ground.png");
        e.add<SharedCPtr<RenderModel>, Translation, SharedCPtr<Texture>>() =
            make_tuple(m_flat_model, flat_transl, ground_tex);

        using TransTup = Tuple<Vector, YRotation>;
        // +x is east, +z is north
        std::array transf = { TransTup{
            Vector{0, -.5, 0.5}, YRotation{}
        }, TransTup{
            Vector{-.5, -.5, 0}, YRotation{k_pi*0.5}
        }, TransTup{
            Vector{0, -.5, -.5}, YRotation{}
        }, TransTup{
            Vector{0.5, -.5, 0}, YRotation{k_pi*0.5}
        } };
        auto add_wall = [this, ground_tex]
            (Entity e, Vector translation, YRotation rot)
        {
            auto fpart = make_tuple(m_wall_model, Translation{translation}, ground_tex);
            if (rot.value == 0.f) {
                e.add<SharedCPtr<RenderModel>, Translation, SharedCPtr<Texture>>() = fpart;
            } else {
                e.add<SharedCPtr<RenderModel>, Translation, SharedCPtr<Texture>, YRotation>() =
                        tuple_cat(fpart, make_tuple(rot));
            }
            return e;
        };
        auto titr = transf.begin();
        for (const auto & val : dips) {
            assert(titr != transf.end());
            if (val != 0.f) {
                m_callbacks.add(add_wall(Entity::make_sceneless_entity(),
                    flat_transl.value + get<Vector>(*titr),
                    get<YRotation>(*titr)));
            }
            titr++;
        }
        {
        auto [tria, trib] = make_flat_segments(loc, flat.y); {}
        adder.add_triangle(tria);
        adder.add_triangle(trib);
        }
        return;
    }
    m_flat_model = m_callbacks.platform().make_render_model();
    m_flat_model->load(
    { // +x is east, +z is north
        Vertex{k_flat_points[0], Vector2{ 0      , 0       }},
        Vertex{k_flat_points[1], Vector2{ 1. / 3., 0       }},
        Vertex{k_flat_points[2], Vector2{ 1. / 3., 1. / 3. }},
        Vertex{k_flat_points[3], Vector2{ 0      , 1. / 3. }},
    }, get_common_elements());
    m_wall_model = m_callbacks.platform().make_render_model();
    m_wall_model->load(
    { // +x is east, +z is north
      // wall runs east to west
        Vertex{Vector{-.5, 0.5, 0}, Vector2{0      , 2. / 3.}},
        Vertex{Vector{0.5, 0.5, 0}, Vector2{1. / 3., 2. / 3.}},
        Vertex{Vector{0.5, -.5, 0}, Vector2{1. / 3., 3. / 3.}},
        Vertex{Vector{-.5, -.5, 0}, Vector2{0      , 3. / 3.}},
    }, get_common_elements());
    assert(*m_flat_model);
    assert(*m_wall_model);
    return create_flat(adder, loc, flat, dips);
}

/* static */ Real TileGraphicGenerator::rotation_between
    (const Slopes & rhs, const Slopes & lhs)
{
    auto side_of = [] (const Slopes & slopes)
        { return std::array{ &slopes.ne, &slopes.nw, &slopes.sw, &slopes.se }; };
    auto right = side_of(rhs);
    auto left  = side_of(lhs);
    using Itr = decltype(right.begin());
    static_assert(std::is_same_v<Itr, decltype(left.begin())>);
    auto left_matches_at = [&left, &right](Itr ritr) {
        auto next = [&right] (Itr itr) {
            return (itr + 1) == right.end() ? right.begin() : itr + 1;
        };
        for (auto val : left) {
            // since we may have done subtraction... we want to use
            // "are_very_close" rather than straight equality (these are
            // floating points after all)

            if (!are_very_close(**ritr, *val)) return false;
            ritr = next(ritr);
        }
        return true;
    };
    for (auto ritr = right.begin(); ritr != right.end(); ++ritr) {
        if (left_matches_at(ritr)) {
            return -(ritr - right.begin())*k_pi*0.5;
        }
    }
    return k_inf;
}

/* static */ Slopes TileGraphicGenerator::sub_minimum_value
    (const Slopes & slopes)
{
    auto min_val = std::min({ slopes.ne, slopes.nw, slopes.se, slopes.sw });
    return Slopes{slopes.id, slopes.ne - min_val, slopes.nw - min_val,
                  slopes.sw - min_val, slopes.se - min_val};
}

/* static */ std::array<Vector, 4> TileGraphicGenerator::get_points_for
    (const Slopes & slopes)
{
    return std::array<Vector, 4> {
        k_flat_points[0] + Vector{0, slopes.nw, 0},
        k_flat_points[1] + Vector{0, slopes.sw, 0},
        k_flat_points[2] + Vector{0, slopes.se, 0},
        k_flat_points[3] + Vector{0, slopes.ne, 0}
    };
}

/* static */ const std::vector<unsigned> &
    TileGraphicGenerator::get_common_elements()
{
    static constexpr const std::array<unsigned, 6> arr = {
        0, 1, 2,
        0, 2, 3
    };
    static const std::vector<unsigned> s_rv{arr.begin(), arr.end()};
    return s_rv;
}

/* private */ SharedPtr<Texture> & TileGraphicGenerator::ensure_texture
    (SharedPtr<Texture> & sptr, const char * filename)
{
    if (!sptr) {
        sptr = m_callbacks.platform().make_texture();// m_platform.make_texture();
        sptr->load_from_file(filename);
    }
    return sptr;
}

/* private */ Tuple<Slopes, SharedPtr<RenderModel>,
      TriangleSegment, TriangleSegment>
    TileGraphicGenerator::get_slope_model_
    (const Slopes & slopes, const Vector & translation)
{
    static constexpr const Real tx_tile_len = 16. / 256.;
    const auto k_points = get_points_for(slopes);

    auto itr = m_slopes_map.find(slopes);
    if (itr != m_slopes_map.end()) {
        const auto & els = get_common_elements();
        auto triangle_segments = make_tuple(
            TriangleSegment{
                k_points[els[0]] + translation,
                k_points[els[1]] + translation,
                k_points[els[2]] + translation},
            TriangleSegment{
                k_points[els[3]] + translation,
                k_points[els[4]] + translation,
                k_points[els[5]] + translation});
        assert(*itr->second);
        auto pair = *itr;
        return std::tuple_cat(std::move(pair), triangle_segments);
    }

    // it's just a dumb quad (two triangles)
    auto rmptr = m_callbacks.platform().make_render_model();// m_platform.make_render_model();
    rmptr->load(
    { // +x is east, +z is north
        Vertex{k_points[0], Vector2{ tx_tile_len, 0 }},
        Vertex{k_points[1], Vector2{ 0, 0 }},
        Vertex{k_points[2], Vector2{ 0, tx_tile_len }},
        Vertex{k_points[3], Vector2{ tx_tile_len, tx_tile_len }},
    }, get_common_elements());
    m_slopes_map[slopes] = rmptr;
    assert(*m_slopes_map[slopes]);
    auto rmptr2 = *m_slopes_map.find(slopes);
    return get_slope_model_(slopes, translation);
}

// ----------------------------------------------------------------------------

MaybeCell CharToCell::operator () (char c) const {
    if (c == '\n') return EndOfRow{};
    return to_maybe_cell(to_cell(c));
}

/* static */ Cell CharToCell::to_cell(const MaybeCell & mcell) {
    if (auto * p = get_if<Flat>(&mcell)) return Cell{*p};
    if (auto * p = get_if<Slopes>(&mcell)) return Cell{*p};
    if (auto * p = get_if<Pit>(&mcell)) return Cell{*p};
    if (auto * p = get_if<VoidSpace>(&mcell)) return Cell{*p};
    throw InvArg{""};
}

/* static */ MaybeCell CharToCell::to_maybe_cell(const Cell & mcell) {
    if (auto * p = get_if<Flat>(&mcell)) return MaybeCell{*p};
    if (auto * p = get_if<Slopes>(&mcell)) return MaybeCell{*p};
    if (auto * p = get_if<Pit>(&mcell)) return MaybeCell{*p};
    if (auto * p = get_if<VoidSpace>(&mcell)) return MaybeCell{*p};
    throw InvArg{""};
}

/* static */ const CharToCell & CharToCell::default_instance() {
    class Impl final : public CharToCell {
        Cell to_cell(char c) const final {
            switch (c) {
            case 'x': return VoidSpace{};
            // s, n, w, e
            case 'v': return Slopes{0, 0.f, 0.f, 1.f, 1.f};
            case '^': return Slopes{0, 1.f, 1.f, 0.f, 0.f};
            case '<': return Slopes{0, 0.f, 1.f, 1.f, 0.f};
            case '>': return Slopes{0, 1.f, 0.f, 0.f, 1.f};
            // nw, ne, se, sw
            case 'a': return Slopes{0, 0.f, 0.f, 0.f, 1.f};
            case 'b': return Slopes{0, 0.f, 0.f, 1.f, 0.f};
            case 'c': return Slopes{0, 1.f, 0.f, 0.f, 0.f};
            case 'd': return Slopes{0, 0.f, 1.f, 0.f, 0.f};
            case ' ': return Flat{0, 0.f};
            case '1': return Flat{0, 1.f};
            default: throw InvArg{"no"};
            }
        }
    };
    static Impl inst;
    return inst;
}

// ----------------------------------------------------------------------------

TriangleLinks
    load_map_graphics
    (TileGraphicGenerator & tileset, CellSubGrid grid)
{
    using std::get;
    auto links = add_triangles_and_link_(grid.width(), grid.height(),
        [&tileset, &grid](Vector2I r, TrianglesAdder & adder) {
        if (get_if<Slopes>(&grid(r))) {
            tileset.create_slope(adder, r, get<Slopes>(grid(r)));
        } else if (get_if<Flat>(&grid(r))) {
            tileset.create_flat(adder, r, get<Flat>(grid(r)), get_dips(grid, r));
        }
    });
    return std::get<0>(links);
}

Grid<Cell> load_map_cell(const char * layout, const CharToCell & char_to_cell) {
    std::vector<CharToCell::MaybeCell> maybes;
    for (auto c = layout; *c; ++c) maybes.push_back(char_to_cell(*c));
    int width = 0;
    int height = 0;
    int counted_width = 0;
    for (const auto & mcell : maybes) {
        if (get_if<EndOfRow>(&mcell)) {
            width = std::max(width, counted_width);
            ++height;
            counted_width = 0;
            continue;
        }
        ++width;
    }
    if (!maybes.empty()) {
        if (!get_if<EndOfRow>(&maybes.back())) {
            ++height;
        }
    }
    Grid<Cell> rv;
    rv.set_size(width, height, Cell{VoidSpace{}});
    Vector2I r;
    for (const auto & mcell : maybes) {
        // what I'm doing is stupid, I've written bs like this 100 times,
        // why don't you saved functions or just pull something instead of
        // wasting time here?
        if (get_if<EndOfRow>(&mcell)) {
            ++r.y;
            r.x = 0;
            continue;
        }
        rv(r) = CharToCell::to_cell(mcell);
        ++r.x;
    }
    return rv;
}

namespace {

Tuple<Triangle, Triangle>
       make_flat_segments(Vector2I loc, Real elevation)
{
   auto translation = grid_location_to_v3(loc, elevation);
   return make_tuple(
       TriangleSegment{
           k_flat_points[0] + translation,  // nw
           k_flat_points[1] + translation,  // sw
           k_flat_points[2] + translation}, // se
       TriangleSegment{
           k_flat_points[0] + translation,   // nw
           k_flat_points[2] + translation,   // se
           k_flat_points[3] + translation}); // ne
}

TileGraphicGenerator::WallDips get_dips(CellSubGrid grid, Vector2I r) {
    using WallDips = TileGraphicGenerator::WallDips;
    WallDips rv = { 0.f, 0.f, 0.f, 0.f };
    using NTup = Tuple<Vector2I, float *>;
    using std::get;
    std::array neighbors = {
        NTup{Vector2I{ 0, -1}, &rv[0]}, // n
        NTup{Vector2I{-1,  0}, &rv[1]}, // w
        NTup{Vector2I{ 0,  1}, &rv[2]}, // s
        NTup{Vector2I{ 1,  1}, &rv[3]}  // e
    };
    if (!get_if<Flat>(&grid(r))) {
        throw InvArg{""};
    }
    const auto & flat = get<Flat>(grid(r));
    for (auto [nr, dip] : neighbors) {
        if (!grid.has_position(nr + r)) continue;

        const auto * nflat = get_if<Flat>(&grid(nr + r));
        if (!nflat) continue;
        if (nflat->y >= flat.y) continue;
        *dip = flat.y - nflat->y;
    }
    return rv;
}

} // end of <anonymous> namespace
