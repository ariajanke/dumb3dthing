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

#include "Defs.hpp"
#include "RenderModel.hpp"
#include "Texture.hpp"
#include "PointAndPlaneDriver.hpp"
#include <common/SubGrid.hpp>
#include <common/Vector2.hpp>
#include <common/Grid.hpp>
#include <common/TestSuite.hpp>
#include <memory>
#include <utility>
#include <iostream>
#include <variant>

namespace {

using MaybeCell = CharToCell::MaybeCell;
using Triangle = TriangleSegment;

static const constexpr std::array k_flat_points = {
    Vector{-.5, 0, 0.5}, // nw
    Vector{-.5, 0, -.5}, // sw
    Vector{0.5, 0, -.5}, // se
    Vector{0.5, 0, 0.5}  // ne
};

Vector grid_location_to_v3(Vector2I loc, Real elevation)
    { return Vector{Real(loc.x), elevation, Real(-loc.y)}; }

Tuple<SharedPtr<Triangle>, SharedPtr<Triangle>>
    make_flat_segments(Vector2I loc, Real elevation);

TileGraphicGenerator::WallDips get_dips(CellSubGrid, Vector2I);

class TestSubEnv final : public point_and_plane::EventHandler {
    Variant<Vector2, Vector> displacement_after_triangle_hit
        (const Triangle &, const Vector &,
         const Vector &, const Vector &) const final
    {
        // always land
        return Vector2{};
    }

    Variant<SegmentTransfer, Vector> pass_triangle_side
        (const Triangle &, const Triangle * to,
         const Vector &, const Vector &) const final
    {
        // always fall off
        if (!to) return Vector{};
        return std::make_tuple(false, Vector2{});
    }

    bool cling_to_edge(const Triangle &, TriangleSide) const final
        { return false; }
};

} // end of <anonymous> namespace

// ----------------------------------------------------------------------------

TileGraphicGenerator::TileGraphicGenerator
    (EntityVec & ents, TriangleVec & tris, Platform::ForLoaders & platform):
    m_entities_out(ents),
    m_triangles_out(tris),
    m_platform(platform)
{}

void TileGraphicGenerator::setup() {
    get_slope_model_(Slopes{0, 1, 1, 0, 0}, Vector{});
    get_slope_model_(Slopes{0, 1, 0, 0, 0}, Vector{});
}

void TileGraphicGenerator::create_slope(Vector2I loc, const Slopes & slopes) {
    auto e = m_platform.make_renderable_entity();
    m_entities_out.push_back(e);
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
    = std::make_tuple(
        model, translation,
        YRotation{rot_}, ensure_texture(m_tileset4, "tileset4.png"),
        TextureTranslation{txr});
    m_triangles_out.push_back(segment_a);
    m_triangles_out.push_back(segment_b);
}

void TileGraphicGenerator::create_flat
    (Vector2I loc, const Flat & flat, const WallDips & dips)
{
    if (m_flat_model) {
        using std::get, std::make_tuple, std::tuple_cat;
        assert(m_wall_model);
        auto e = m_platform.make_renderable_entity();
        m_entities_out.push_back(e);
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
                m_entities_out.push_back(add_wall(Entity::make_sceneless_entity(),
                    flat_transl.value + get<Vector>(*titr),
                    get<YRotation>(*titr)));
            }
            titr++;
        }
        {
        auto [tria, trib] = make_flat_segments(loc, flat.y); {}
        m_triangles_out.push_back(tria);
        m_triangles_out.push_back(trib);
        }
        return;
    }
    m_flat_model = m_platform.make_render_model();
    m_flat_model->load(
    { // +x is east, +z is north
        Vertex{k_flat_points[0], Vector2{ 0      , 0       }},
        Vertex{k_flat_points[1], Vector2{ 1. / 3., 0       }},
        Vertex{k_flat_points[2], Vector2{ 1. / 3., 1. / 3. }},
        Vertex{k_flat_points[3], Vector2{ 0      , 1. / 3. }},
    }, {
        0, 1, 2,
        0, 2, 3
    });
    m_wall_model = m_platform.make_render_model();
    m_wall_model->load(
    { // +x is east, +z is north
      // wall runs east to west
        Vertex{Vector{-.5, 0.5, 0}, Vector2{0      , 2. / 3.}},
        Vertex{Vector{0.5, 0.5, 0}, Vector2{1. / 3., 2. / 3.}},
        Vertex{Vector{0.5, -.5, 0}, Vector2{1. / 3., 3. / 3.}},
        Vertex{Vector{-.5, -.5, 0}, Vector2{0      , 3. / 3.}},
    }, {
        0, 1, 2,
        0, 2, 3
    });
    assert(*m_flat_model);
    assert(*m_wall_model);
    return create_flat(loc, flat, dips);
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
#   if 0
    auto print_out = [](const Slopes & slps) {
        std::cout << "ne " << slps.ne << " nw " << slps.nw << " sw " << slps.sw << " se " << slps.se << std::endl;
    };
    std::cout << "lhs ";
    print_out(lhs);
    std::cout << "rhs ";
    print_out(rhs);
#   endif
    return k_inf;
}

/* static */ Slopes TileGraphicGenerator::sub_minimum_value
    (const Slopes & slopes)
{
    auto min_val = std::min({ slopes.ne, slopes.nw, slopes.se, slopes.sw });
    return Slopes{slopes.id, slopes.ne - min_val, slopes.nw - min_val,
                  slopes.sw - min_val, slopes.se - min_val};
}

/* private */ SharedPtr<Texture> & TileGraphicGenerator::ensure_texture
    (SharedPtr<Texture> & sptr, const char * filename)
{
    if (!sptr) {
        sptr = m_platform.make_texture();
        sptr->load_from_file(filename);
    }
    return sptr;
}

/* private */ Tuple<Slopes, SharedPtr<RenderModel>,
      SharedPtr<TriangleSegment>, SharedPtr<TriangleSegment>>
    TileGraphicGenerator::get_slope_model_
    (const Slopes & slopes, const Vector & translation)
{
    static constexpr const Real tx_tile_len = 16. / 256.;
    const std::array<Vector, 4> k_points = {
        k_flat_points[0] + Vector{0, slopes.nw, 0},
        k_flat_points[1] + Vector{0, slopes.sw, 0},
        k_flat_points[2] + Vector{0, slopes.se, 0},
        k_flat_points[3] + Vector{0, slopes.ne, 0}
    };

    auto itr = m_slopes_map.find(slopes);
    if (itr != m_slopes_map.end()) {
        auto triangle_segments = std::make_tuple(
            std::make_shared<TriangleSegment>(
                k_points[0] + translation,
                k_points[1] + translation,
                k_points[2] + translation),
            std::make_shared<TriangleSegment>(
                k_points[0] + translation,
                k_points[2] + translation,
                k_points[3] + translation));
        assert(*itr->second);
        auto pair = *itr;
        return std::tuple_cat(std::move(pair), triangle_segments);
    }

    // it's just a dumb quad (two triangles)
    auto rmptr = m_platform.make_render_model();
    rmptr->load(
    { // +x is east, +z is north
        Vertex{k_points[0], Vector2{ tx_tile_len, 0 }},
        Vertex{k_points[1], Vector2{ 0, 0 }},
        Vertex{k_points[2], Vector2{ 0, tx_tile_len }},
        Vertex{k_points[3], Vector2{ tx_tile_len, tx_tile_len }},
    }, {
        0, 1, 2,
        0, 2, 3
    });
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
    using std::get_if;
    if (auto * p = get_if<Flat>(&mcell)) return Cell{*p};
    if (auto * p = get_if<Slopes>(&mcell)) return Cell{*p};
    if (auto * p = get_if<Pit>(&mcell)) return Cell{*p};
    if (auto * p = get_if<VoidSpace>(&mcell)) return Cell{*p};
    throw std::invalid_argument("");
}

/* static */ MaybeCell CharToCell::to_maybe_cell(const Cell & mcell) {
    using std::get_if;
    if (auto * p = get_if<Flat>(&mcell)) return MaybeCell{*p};
    if (auto * p = get_if<Slopes>(&mcell)) return MaybeCell{*p};
    if (auto * p = get_if<Pit>(&mcell)) return MaybeCell{*p};
    if (auto * p = get_if<VoidSpace>(&mcell)) return MaybeCell{*p};
    throw std::invalid_argument("");
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
            default: throw std::invalid_argument("no");
            }
        }
    };
    static Impl inst;
    return inst;
}

// ----------------------------------------------------------------------------

Tuple<std::vector<TriangleLinks>,
      std::vector<Entity>       >
    load_map_graphics
    (TileGraphicGenerator & tileset, CellSubGrid grid)
{
    using std::get, std::get_if;
    Grid<std::pair<std::size_t, std::size_t>> links_grid;
    links_grid.set_size(grid.width(), grid.height());
    for (Vector2I r; r != grid.end_position(); r = grid.next(r)) {
        links_grid(r).first = tileset.triangle_count();
        if (get_if<Slopes>(&grid(r))) {
            tileset.create_slope(r, get<Slopes>(grid(r)));
        } else if (get_if<Flat>(&grid(r))) {
            tileset.create_flat(r, get<Flat>(grid(r)), get_dips(grid, r));
        }
        links_grid(r).second = tileset.triangle_count();
    }

    using TrisItr = TileGraphicGenerator::TriangleVec::const_iterator;
    using TrisView = cul::View<TrisItr>;
    Grid<TrisView> triangles_grid;
    triangles_grid.set_size(links_grid.width(), links_grid.height(),
                            TrisView{tileset.triangles_view().end(), tileset.triangles_view().end()});
    {
    auto beg = tileset.triangles_view().begin();
    for (Vector2I r; r != links_grid.end_position(); r = links_grid.next(r)) {
        triangles_grid(r) = TrisView{beg + links_grid(r).first, beg + links_grid(r).second};
    }
    }
    using Links = point_and_plane::TriangleLinks;
    std::vector<Links> links;
    // now link them together
    for (Vector2I r; r != triangles_grid.end_position(); r = triangles_grid.next(r)) {
    for (auto this_tri : triangles_grid(r)) {
        if (!this_tri) continue;
        links.emplace_back(this_tri);
        for (Vector2I v : { r, Vector2I{1, 0} + r, Vector2I{-1,  0} + r,
/*                          */ Vector2I{0, 1} + r, Vector2I{ 0, -1} + r}) {
        if (!links_grid.has_position(v)) continue;
        for (auto other_tri : triangles_grid(v)) {
            if (!other_tri) continue;
            if (this_tri == other_tri) continue;
            auto & link = links.back();
#           if 0
            std::cout << link.hash() << " to " << std::hash<const TriangleSegment *>{}( other_tri.get() ) << std::endl;
#           endif
            link.attempt_attachment_to(other_tri);
        }}
#       if 0
        print_links(std::cout, links.back());
#       endif
    }}
    return std::make_tuple(links, tileset.give_entities());
}

Grid<Cell> load_map_cell(const char * layout, const CharToCell & char_to_cell) {
    using std::get_if;
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
        if (!std::get_if<EndOfRow>(&maybes.back())) {
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

void run_map_loader_tests() {
    // test movements in each cardinal direction
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    using namespace cul::ts;
    TestSuite suite;
    suite.start_series("Map Loader");
    static auto load_test_layout = [] {
        static constexpr auto k_layout =
            "xxx\n"
            "xx \n"
            "x  \n";
        std::vector<Entity> entities;
        std::vector<SharedPtr<TriangleSegment>> triangles;

        TileGraphicGenerator tgg{entities, triangles, Platform::null_callbacks()};
        tgg.setup();

        auto grid = load_map_cell(k_layout, CharToCell::default_instance());
        auto gv = load_map_graphics(tgg, grid);
        return std::get<0>(gv);
    };

    using std::any_of, std::get_if, std::get;

    static auto any_point_arrangement_of = []
        (const Triangle & tri, const std::array<Vector, 3> & pts)
    {
        auto make_pred = [](const Vector & tri_pt) {
            return [tri_pt](const Vector & r)
                { return are_very_close(tri_pt, r); };
        };
        return    any_of(pts.begin(), pts.end(), make_pred(tri.point_a()))
               && any_of(pts.begin(), pts.end(), make_pred(tri.point_b()))
               && any_of(pts.begin(), pts.end(), make_pred(tri.point_c()));
    };

    static auto make_driver_for_test_layout = [] {
        auto links = load_test_layout();
        auto ppdriver = point_and_plane::Driver::make_driver();
        ppdriver->add_triangles(links);
        ppdriver->update();
        return ppdriver;
    };

    static auto run_intersegment_transfer = []
        (point_and_plane::Driver & driver, const Vector & start, const Vector & displacement)
    {
        PpState state = PpInAir{start + Vector{0, 0.1, 0}, Vector{0, -0.2, 0}};
        state = driver(state, TestSubEnv{});

        auto & tri = *get<PpOnSurface>(state).segment;
        get<PpOnSurface>(state).displacement =
            tri.closest_point(displacement + start) - tri.closest_point(start);

        return driver(state, TestSubEnv{});
    };

    // triangle locations sanity
    mark(suite).test([] {
        auto links = load_test_layout();
        return test(any_of(links.begin(), links.end(), [](const TriangleLinks & link) {
            const auto & tri = link.segment();
            auto pt_list = { tri.point_a(), tri.point_b(), tri.point_c() };
            return any_of(pt_list.begin(), pt_list.end(), [](Vector r)
                { return are_very_close(r.x, 2.5) && are_very_close(r.z, -2.5); });
        }));
    });

    mark(suite).test([] {
        auto ppdriver = make_driver_for_test_layout();
        PpState state = PpInAir{ Vector{2, 0.1, -1.9}, Vector{ 0, -0.2, 0 } };
        state = (*ppdriver)(state, TestSubEnv{});
        return test(get_if<PpOnSurface>(&state));
    });
    // can cross eastbound
    mark(suite).test([] {
        constexpr const Vector k_start{1.4, 0, -2};
        constexpr const Vector k_displacement{0.2, 0, 0};
        constexpr const std::array k_expect_triangle =
            { Vector{1.5, 0, -2.5}, Vector{1.5, 0, -1.5}, Vector{2.5, 0, -2.5} };

        auto ppdriver = make_driver_for_test_layout();
        auto state = run_intersegment_transfer(*ppdriver, k_start, k_displacement);

        auto & res_tri = *get<PpOnSurface>(state).segment;
        return test(any_point_arrangement_of(res_tri, k_expect_triangle));
    });
    // can cross westbound
    mark(suite).test([] {
        constexpr const Vector k_start{1.6, 0, -2};
        constexpr const Vector k_displacement{-0.2, 0, 0};
        constexpr const std::array k_expect_triangle =
            { Vector{1.5, 0, -2.5}, Vector{1.5, 0, -1.5}, Vector{0.5, 0, -1.5} };

        auto ppdriver = make_driver_for_test_layout();
        auto state = run_intersegment_transfer(*ppdriver, k_start, k_displacement);

        return test(any_point_arrangement_of(
            *get<PpOnSurface>(state).segment, k_expect_triangle));
    });
    // land on...??? which triangle in the middle?
#   undef mark
}

namespace {

Tuple<SharedPtr<Triangle>, SharedPtr<Triangle>>
       make_flat_segments(Vector2I loc, Real elevation)
{
   using std::make_tuple, std::make_shared;
   auto translation = grid_location_to_v3(loc, elevation);
   return make_tuple(
       make_shared<TriangleSegment>(
           k_flat_points[0] + translation,  // nw
           k_flat_points[1] + translation,  // sw
           k_flat_points[2] + translation), // se
       make_shared<TriangleSegment>(
           k_flat_points[0] + translation,   // nw
           k_flat_points[2] + translation,   // se
           k_flat_points[3] + translation)); // ne
}

TileGraphicGenerator::WallDips get_dips(CellSubGrid grid, Vector2I r) {
    using WallDips = TileGraphicGenerator::WallDips;
    WallDips rv = { 0.f, 0.f, 0.f, 0.f };
    using NTup = Tuple<Vector2I, float *>;
    using std::get, std::get_if;
    std::array neighbors = {
        NTup{Vector2I{ 0, -1}, &rv[0]}, // n
        NTup{Vector2I{-1,  0}, &rv[1]}, // w
        NTup{Vector2I{ 0,  1}, &rv[2]}, // s
        NTup{Vector2I{ 1,  1}, &rv[3]}  // e
    };
    if (!get_if<Flat>(&grid(r))) {
        throw std::invalid_argument("");
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
