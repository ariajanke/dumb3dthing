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

#include "TileSet.hpp"
#include "Texture.hpp"
#include "RenderModel.hpp"
#include "tiled-map-loader.hpp"

// can always clean up embedded testing later
#include <common/TestSuite.hpp>

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

using ConstTileSetPtr    = TileSet::ConstTileSetPtr;
using TileSetPtr         = TileSet::TileSetPtr;
using Triangle           = TriangleSegment;
#if 0
using CardinalDirections = TileFactory::CardinalDirections;
#endif
class TranslatableTileFactory : public TileFactory {
public:
    void setup(Vector2I, const tinyxml2::XMLElement * properties, Platform::ForLoaders &) override;

protected:
    Vector translation() const { return m_translation; }

    Entity make_entity(Platform::ForLoaders & platform, Vector2I tile_loc,
                       SharedCPtr<RenderModel> model_ptr) const;

private:
    Vector m_translation;
};

CardinalDirections cardinal_direction_from(const char * str) {
    auto seq = [str](const char * s) { return !::strcmp(str, s); };
    using Cd = CardinalDirections;
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
#if 0
bool is_solution(Real);
#endif
enum SplitOpt {
    k_flats_only = 1 << 0,
    k_wall_only  = 1 << 1,
    k_both_flats_and_wall = k_flats_only | k_wall_only
};

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
void east_west_split
    (Real north_east_y, Real north_west_y,
     Real south_east_y, Real south_west_y,
     Real division_z, Func && f);

auto make_get_next_for_dir_split(Vector end, Real dir) {
    return [end, dir] (Vector east_itr) {
        auto cand_next = east_itr + dir*Vector{0, 1, 0};
        if (are_very_close(cand_next, end)) return cand_next;

        if (are_very_close(normalize(end - east_itr ),
                           normalize(end - cand_next)))
        { return cand_next; }
        return end;
    };
};


template <SplitOpt k_opt, typename Func>
void north_south_split
    (Real north_east_y, Real north_west_y,
     Real south_east_y, Real south_west_y,
     Real division_z, Func && f)
{
    // it really doesn't have to be "elegant"
    // division z must make sense
    assert(division_z >= -0.5 && division_z <= 0.5);

    // both sets of y values' directions must be the same
    assert((north_east_y - north_west_y)*(south_east_y - south_west_y) >= 0);

    const Vector div_nw{-0.5, north_west_y, division_z};
    const Vector div_ne{ 0.5, north_east_y, division_z};

    const Vector div_sw{-0.5, south_west_y, division_z};
    const Vector div_se{ 0.5, south_east_y, division_z};
    // We must handle division_z being 0.5
    if constexpr ((k_opt & k_flats_only) && false) {
        if (!are_very_close(division_z, 0.5)) {
            Vector nw{-0.5, north_west_y, 0.5};
            Vector ne{ 0.5, north_east_y, 0.5};
            f(Triangle{nw, ne    , div_nw});
            f(Triangle{nw, div_nw, div_ne});
        }
        if (!are_very_close(division_z, -0.5)) {
            Vector sw{-0.5, south_west_y, 0.5};
            Vector se{ 0.5, south_east_y, 0.5};
            f(Triangle{sw, se    , div_sw});
            f(Triangle{sw, div_sw, div_se});
        }
    }
    // We should only skip triangles along the wall if
    // there's no elevation difference to cover
    if constexpr (k_opt & k_wall_only) {
        // it doesn't matter so much where I start, so long as
        // 1ue steps are used
        auto normalize_or_zero = [] (Real x) { return are_very_close(x, 0) ? 0 : normalize(x); };
        // must accept that one direction or both maybe "0"
        const auto east_dir = normalize_or_zero(north_east_y - north_west_y);
        const auto west_dir = normalize_or_zero(south_east_y - south_west_y);
        if (are_very_close(east_dir, 0) && are_very_close(west_dir, 0))
            return;

        auto east_itr = div_ne;
        const auto & east_end = div_se;

        auto west_itr = div_nw;
        const auto & west_end = div_sw;

        auto get_east_next = make_get_next_for_dir_split(east_end, east_dir);
        auto get_west_next = make_get_next_for_dir_split(west_end, west_dir);

        while (   !are_very_close(east_itr, east_end)
               || !are_very_close(west_itr, west_end))
        {
            auto east_next = get_east_next(east_itr);
            auto west_next = get_west_next(west_itr);

            if (!are_very_close(east_itr, east_end)) {
                f(Triangle{east_itr, west_itr, east_next});
            }
            if (!are_very_close(west_itr, west_end)) {
                f(Triangle{west_itr, east_itr, west_next});
            }

            east_itr = east_next;
            west_itr = west_next;
        }
    }
}

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

class WallTileFactory final : public TranslatableTileFactory {
public:
    void assign_render_model_wall_cache(WallRenderModelCache & cache) final
        { m_render_model_cache = &cache; }

private:
    static constexpr const Real k_visual_dip_thershold = 0.5;
    static constexpr const Real k_physical_dip_thershold = 1;

    template <typename Iter>
    static void translate_points(Vector r, Iter beg, Iter end) {
        for (auto itr = beg; itr != end; ++itr) {
            *itr += r;
        }
    }

    template <typename Iter>
    static void scale_points_x(Real x, Iter beg, Iter end) {
        for (auto itr = beg; itr != end; ++itr) {
            itr->x *= x;
        }
    }

    template <typename Iter>
    static void scale_points_z(Real x, Iter beg, Iter end) {
        for (auto itr = beg; itr != end; ++itr) {
            itr->z *= x;
        }
    }

    void setup
        (Vector2I loc_in_ts, const tinyxml2::XMLElement * properties, Platform::ForLoaders & platform) final
    {
        TranslatableTileFactory::setup(loc_in_ts, properties, platform);
        m_dir = cardinal_direction_from(find_property("direction", properties));
        m_tileset_location = loc_in_ts;
    }

    Slopes tile_elevations() const final {
        // it is possible that some elevations are indeterminent...
        Real y = translation().y + 1;
        return Slopes{0, y, y, y, y};
    }

    // a wall has at least one elevation that's known
    // a wall only generates if it's dip sides connect to tiles whose elevations
    // are lower than the known elevation

    void operator ()
        (EntityAndTrianglesAdder & adder, const NeighborInfo & ninfo,
         Platform::ForLoaders & platform) const final
    { make_tile(adder, ninfo, platform); }

    void make_tile
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

    void make_entities_and_triangles(
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
            adder.add_triangle(make_shared<Triangle>(triangle.move(translation())));
        }
        // mmm
        // make_entity(platform, ninfo.tile_location(), render_model);
    }

    static int corner_index(CardinalDirections dir) {
        using Cd = CardinalDirections;
        switch (dir) {
        case Cd::nw: return 0;
        case Cd::sw: return 1;
        case Cd::se: return 2;
        case Cd::ne: return 3;
        default: break;
        }
        throw RtError{""};
    }

    static std::array<bool, 4> make_known_corners(CardinalDirections dir) {
        using Cd = CardinalDirections;
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

    WallElevationAndDirection elevations_and_direction
        (const NeighborInfo & ninfo) const
    {
        WallElevationAndDirection rv;
        // I need a way to address each corner...
        using Cd = CardinalDirections;
        // elevations for knowns for this tile
        auto known_elevation = translation().y + 1;

        rv.direction = m_dir;
        rv.tileset_location = m_tileset_location;
        auto known_corners = make_known_corners(m_dir);
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

    Tuple<SharedPtr<const RenderModel>, std::vector<Triangle>>
        make_render_model_and_triangles(
        const WallElevationAndDirection & wed,
        const NeighborInfo & ninfo,
        Platform::ForLoaders &) const
    {
        auto offset = grid_position_to_v3(ninfo.tile_location())
            + translation() + Vector{0, 1, 0};
        std::vector<Triangle> triangles;
        struct Impl final {
            Impl(std::vector<Triangle> & triangles_):
                triangles(triangles_)
            {}

            void operator () (const Triangle & triangle) const {
                triangles.push_back(triangle);
            }

            std::vector<Triangle> & triangles;
        };
        Impl add_triangle_{triangles};
        auto & add_triangle = add_triangle_;
#       if 0
        auto add_triangle = [&triangles] (const Triangle & triangle) {
            triangles.push_back(triangle);
        };
#       endif
        // we have cases depending on the number of dips
        // okay, I should like to handle only two (adjacent) and three dips
        using Cd = CardinalDirections;
        auto adjusted_thershold = k_physical_dip_thershold - 0.5;
        // nvm, direction describes which case
        switch (m_dir) {
        // a north wall, all point on the north are known
        case Cd::n :
            north_south_split<k_both_flats_and_wall>(
                offset.y - wed.dip_heights[corner_index(Cd::ne)], // ne
                offset.y - wed.dip_heights[corner_index(Cd::nw)],
                offset.y, offset.y,
                adjusted_thershold, add_triangle);
            break;
        case Cd::s :
            north_south_split<k_both_flats_and_wall>(
                offset.y, offset.y,
                offset.y - wed.dip_heights[corner_index(Cd::se)], // se
                offset.y - wed.dip_heights[corner_index(Cd::sw)],
                1 - adjusted_thershold, add_triangle);
            break;
        case Cd::e :
            east_west_split<k_both_flats_and_wall>(
                offset.y - wed.dip_heights[corner_index(Cd::ne)], // ne
                offset.y - wed.dip_heights[corner_index(Cd::se)],
                offset.y, offset.y,
                adjusted_thershold, add_triangle);
            break;
        case Cd::w :
            east_west_split<k_both_flats_and_wall>(
                offset.y, offset.y,
                offset.y - wed.dip_heights[corner_index(Cd::nw)], // ne
                offset.y - wed.dip_heights[corner_index(Cd::sw)],
                1 - adjusted_thershold, add_triangle);
            break;
        // maybe have seperate divider functions for these cases?
        case Cd::nw: ;
        case Cd::sw: ;
        case Cd::se: ;
        case Cd::ne: ;
            throw RtError{"unimplemented"};
        default: break;
        }
        return make_tuple(nullptr, std::move(triangles));
    }

    CardinalDirections m_dir = CardinalDirections::ne;
    WallRenderModelCache * m_render_model_cache = nullptr;
    Vector2I m_tileset_location;

    // I still need to known the wall texture coords
};

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

// there may, or may not be a factory for a particular id
TileFactory * TileSet::operator () (int tid) const {
    auto itr = m_factory_map.find(tid);
    if (itr == m_factory_map.end()) return nullptr;
    return itr->second.get();
}


TileFactory & TileSet::insert_factory(UniquePtr<TileFactory> uptr, int tid) {
    auto itr = m_factory_map.find(tid);
    if (itr != m_factory_map.end()) {
        throw RtError{"TileSet::insert_factory: tile id already assigned a "
                      "factory. Only one factory is permitted per id."};
    }
    auto & factory = *(m_factory_map[tid] = std::move(uptr));
    factory.set_shared_texture_information(m_texture, m_texture_size, m_tile_size);
    return factory;
}

void TileSet::load_information(Platform::ForLoaders & platform,
                               const tinyxml2::XMLElement & tileset)
{
    int tile_width = tileset.IntAttribute("tilewidth");
    int tile_height = tileset.IntAttribute("tileheight");
    int tile_count = tileset.IntAttribute("tilecount");
    auto to_ts_loc = [&tileset] {
        int columns = tileset.IntAttribute("columns");
        return [columns] (int n) { return Vector2I{n % columns, n / columns}; };
    } ();

    auto image_el = tileset.FirstChildElement("image");
    int tx_width = image_el->IntAttribute("width");
    int tx_height = image_el->IntAttribute("height");

    auto tx = platform.make_texture();
    (*tx).load_from_file(image_el->Attribute("source"));

    set_texture_information(tx, Size2{tile_width, tile_height}, Size2{tx_width, tx_height});
    m_tile_count = tile_count;
    for (auto itr = tileset.FirstChildElement("tile"); itr;
         itr = itr->NextSiblingElement("tile"))
    {
        int id = itr->IntAttribute("id");
        auto tileset_factory = TileFactory::make_tileset_factory(itr->Attribute("type"));
        if (!tileset_factory)
            continue;
        insert_factory(std::move(tileset_factory), id)
            .setup(to_ts_loc(id),
                   itr->FirstChildElement("properties")->FirstChildElement("property"),
                   platform);
    }
}

void TileSet::set_texture_information
    (const SharedPtr<const Texture> & texture, const Size2 & tile_size,
     const Size2 & texture_size)
{
    m_texture = texture;
    m_texture_size = texture_size;
    m_tile_size = tile_size;
}

// ----------------------------------------------------------------------------

GidTidTranslator::GidTidTranslator
    (const std::vector<TileSetPtr> & tilesets, const std::vector<int> & startgids)
{
    if (tilesets.size() != startgids.size()) {
        throw RtError("Bug in library, GidTidTranslator constructor expects "
                      "both passed containers to be equal in size.");
    }
    m_gid_map.reserve(tilesets.size());
    for (std::size_t i = 0; i != tilesets.size(); ++i) {
        m_gid_map.emplace_back(startgids[i], tilesets[i]);
    }
    if (!startgids.empty()) {
        m_gid_end = startgids.back() + tilesets.back()->total_tile_count();
    }
    m_ptr_map.reserve(m_gid_map.size());
    for (auto & pair : m_gid_map) {
        m_ptr_map.emplace_back( pair.starting_id, pair.tileset );
    }

    std::sort(m_gid_map.begin(), m_gid_map.end(), order_by_gids);
    std::sort(m_ptr_map.begin(), m_ptr_map.end(), order_by_ptrs);
}

std::pair<int, ConstTileSetPtr> GidTidTranslator::gid_to_tid(int gid) const {
    if (gid < 1 || gid >= m_gid_end) {
        throw InvArg{"Given gid is either the empty tile or not contained in "
                     "this map; translatable gids: [1 " + std::to_string(m_gid_end)
                     + ")."};
    }
    GidAndTileSetPtr samp;
    samp.starting_id = gid;
    auto itr = std::upper_bound(m_gid_map.begin(), m_gid_map.end(), samp, order_by_gids);
    if (itr == m_gid_map.begin()) {
        throw RtError{"Library error: GidTidTranslator said that it owned a "
                      "gid, but does not have a tileset for it."};
    }
    --itr;
    assert(gid >= itr->starting_id);
    return std::make_pair(gid - itr->starting_id, itr->tileset);
}

std::pair<int, TileSetPtr> GidTidTranslator::gid_to_tid(int gid) {
    const auto & const_this = *this;
    auto gv = const_this.gid_to_tid(gid);
    return std::make_pair(gv.first, std::const_pointer_cast<TileSet>(gv.second));
}

void GidTidTranslator::swap(GidTidTranslator & rhs) {
    m_ptr_map.swap(rhs.m_ptr_map);
    m_gid_map.swap(rhs.m_gid_map);

    std::swap(m_gid_end, rhs.m_gid_end);
}

int GidTidTranslator::tid_to_gid(int tid, ConstTileSetPtr tileset) const {
    GidAndConstTileSetPtr samp;
    samp.tileset = tileset;
    auto itr = std::lower_bound(m_ptr_map.begin(), m_ptr_map.end(), samp, order_by_ptrs);
    static constexpr const auto k_unowned_msg = "Map/layer does not own this tile set.";
    if (itr == m_ptr_map.end()) {
        throw RtError(k_unowned_msg);
    } else if (itr->tileset != tileset) {
        throw RtError(k_unowned_msg);
    }
    return tid + itr->starting_id;
}

/* private static */ bool GidTidTranslator::order_by_gids
    (const GidAndTileSetPtr & lhs, const GidAndTileSetPtr & rhs)
{ return lhs.starting_id < rhs.starting_id; }

/* private static */ bool GidTidTranslator::order_by_ptrs
    (const GidAndConstTileSetPtr & lhs, const GidAndConstTileSetPtr & rhs)
{
    const void * lptr = lhs.tileset.get();
    const void * rptr = rhs.tileset.get();
    return std::less<const void *>()(lptr, rptr);
}

// ----------------------------------------------------------------------------

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
    TileFactory::NeighborInfo::make_no_neighbor(const TileSet & tileset)
{
    static Grid<int> s_layer;
    return NeighborInfo{tileset, s_layer, Vector2I{}, Vector2I{}};
}

Real TileFactory::NeighborInfo::neighbor_elevation(CardinalDirections dir) const {
    using Cd = CardinalDirections;

    using VecCdTup = Tuple<Vector2I, Cd>;
    auto select_el = [this] (const std::array<VecCdTup, 2> & arr) {
        // I miss map, transform sucks :c
        std::array<Real, 2> vals;
        std::transform(arr.begin(), arr.end(), vals.begin(), [this] (const VecCdTup & tup) {
            auto [r, d] = tup; {}
            return cul::is_real(neighbor_elevation(r, d));
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
            make_tuple(Vector2I{ 0, -1}, Cd::sw),
            make_tuple(Vector2I{-1 , 0}, Cd::ne)
        });
    case Cd::se:
        return select_el(std::array{
            make_tuple(Vector2I{ 0, -1}, Cd::sw),
            make_tuple(Vector2I{-1 , 0}, Cd::ne)
        });
    case Cd::ne:
        return select_el(std::array{
            make_tuple(Vector2I{ 0, -1}, Cd::sw),
            make_tuple(Vector2I{-1 , 0}, Cd::ne)
        });
    default: break;
    }
    throw BadBranchException{__LINE__, __FILE__};
}

/* private */ Real TileFactory::NeighborInfo::neighbor_elevation
    (const Vector2I & r, CardinalDirections dir) const
{
    using Cd = CardinalDirections;
    if (!m_layer.has_position(r)) return k_inf;
    const auto * ts = m_tileset(m_layer(r));
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

/* static */ bool TileFactory::test_wall_factory() {
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    using namespace cul::ts;
    TestSuite suite;
    suite.start_series("Wall Tile Factory");
    constexpr const auto k_tileset_fn = "test-tileset.tsx";
    mark(suite).test([] {
        TiXmlDocument document;
        document.LoadFile(k_tileset_fn);
        TileSet tileset;
        auto & platform = Platform::null_callbacks();
        tileset.load_information(platform, *document.RootElement());
        constexpr const int k_north_wall_no_translation = 34;
        auto * wall_tile_factory = tileset(k_north_wall_no_translation);
        if (!wall_tile_factory) {
            throw RtError{"uh oh, no wall factory!"};
        }

        class Impl final : public EntityAndTrianglesAdder {
        public:
            void add_triangle(const SharedPtr<TriangleSegment> & ptr) {
                triangles.push_back(*ptr);
            }

            void add_entity(const Entity &) final {}

            std::vector<Triangle> triangles;
        };

        Impl adder;
        // what the tile is, appears through the adder

        auto no_neighbor = NeighborInfo::make_no_neighbor(tileset);
        (*wall_tile_factory)(adder, no_neighbor, platform);
        return test(false);
    });
#   undef mark
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
    adder.add_triangle(make_shared<TriangleSegment>(
        pos[els[0]] + offset, pos[els[1]] + offset, pos[els[2]] + offset));
    adder.add_triangle(make_shared<TriangleSegment>(
        pos[els[3]] + offset, pos[els[4]] + offset, pos[els[5]] + offset));
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

// --------------------------- <anonymous> namespace --------------------------

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
    using Cd = CardinalDirections;
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
    using Cd = CardinalDirections;
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
