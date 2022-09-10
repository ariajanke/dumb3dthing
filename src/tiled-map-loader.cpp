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

#include "tiled-map-loader.hpp"
#include "map-loader.hpp"
#include "Components.hpp"
#include "RenderModel.hpp"
#include "Texture.hpp"

#include <common/StringUtil.hpp>

#include <map>
#include <unordered_map>

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

class StringSplitterIteratorEnd {};

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc>
class StringSplitterIterator {
public:
    StringSplitterIterator(CharIter beg, CharIter end,
                           const SplitterFunc & splitter_,
                           const WithAdditionalFunc & with_):
        m_beg(beg), m_segment_end(beg), m_end(end),
        m_splitter_f(std::move(splitter_)),
        m_with_f(std::move(with_))
    { update_end_segment(); }

    bool operator == (const StringSplitterIteratorEnd &) const noexcept
        { return is_at_end(); }

    bool operator != (const StringSplitterIteratorEnd &) const noexcept
        { return !is_at_end(); }

    StringSplitterIterator & operator ++ () {
        move_to_next();
        update_end_segment();
        return *this;
    }

    cul::View<CharIter> operator * () const {
        auto b = m_beg;
        auto e = m_segment_end;
        m_with_f(b, e);
        return cul::View<CharIter>{b, e};
    }

private:
    bool is_at_end() const noexcept
        { return m_beg == m_end; }

    void update_end_segment()
        { m_segment_end = std::find_if(m_beg, m_end, m_splitter_f); }

    void move_to_next() {
        m_beg = m_segment_end;
        if (m_beg != m_end) ++m_beg;
    }

    CharIter m_beg, m_segment_end, m_end;
    SplitterFunc m_splitter_f;
    WithAdditionalFunc m_with_f;
};

constexpr const auto is_comma = [](char c) { return c == ','; };
#if 0
constexpr const auto is_whitespace = [](char c)
    { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; };
#else
bool is_whitespace(char c)
    { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }
#endif
#if 0
void trim_whitespace(const char *& beg, const char *& end) {
    cul::trim<is_whitespace>(beg, end);
}
#else
constexpr const auto trim_whitespace = [] (const char *& beg, const char *& end) {
    cul::trim<is_whitespace>(beg, end);
};
#endif

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc>
using SplitRangeView =
    cul::View<StringSplitterIterator<CharIter, SplitterFunc, WithAdditionalFunc>,
              StringSplitterIteratorEnd>;

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc>
SplitRangeView<CharIter, SplitterFunc, WithAdditionalFunc>
    split_range(CharIter beg, CharIter end,
                const SplitterFunc & splitter_,
                const WithAdditionalFunc & with_ = [](CharIter &, CharIter &){})
{
    return SplitRangeView<CharIter, SplitterFunc, WithAdditionalFunc> {
        StringSplitterIterator{beg, end, std::move(splitter_), std::move(with_)},
        StringSplitterIteratorEnd{}};
}

enum class CardinalDirections {
    n, s, e, w,
    nw, sw, se, ne
};

class TileFactory;

using Size2 = cul::Size2<Real>;
class TileSet final {
public:
    using ConstTileSetPtr = std::shared_ptr<const TileSet>;
    using TileSetPtr      = std::shared_ptr<TileSet>;

    void load_information(Platform::ForLoaders &, tinyxml2::XMLElement * tileset);

    void set_texture_information
        (SharedPtr<const Texture> texture, const Size2 & tile_size,
         const Size2 & texture_size)
    {
        m_texture = texture;
        m_texture_size = texture_size;
        m_tile_size = tile_size;
    }

    TileFactory & insert_factory(UniquePtr<TileFactory> uptr, int tid);

    // there may, or may not be a factory for a particular id
    TileFactory * operator () (int tid) const {
        auto itr = m_factory_map.find(tid);
        if (itr == m_factory_map.end()) return nullptr;
        return itr->second.get();
    }

    int total_tile_count() const { return m_tile_count; }

private:
    std::map<int, UniquePtr<TileFactory>> m_factory_map;

    SharedPtr<const Texture> m_texture;
    Size2 m_texture_size;
    Size2 m_tile_size;
    int m_tile_count = 0;
};

// straight up yoinked from tmap
class GidTidTranslator final {
public:
    using ConstTileSetPtr = TileSet::ConstTileSetPtr;
    using TileSetPtr      = TileSet::TileSetPtr;

    GidTidTranslator() {}

    GidTidTranslator(const std::vector<TileSetPtr> & tilesets, const std::vector<int> & startgids);

    std::pair<int, ConstTileSetPtr> gid_to_tid(int gid) const;

    std::pair<int, TileSetPtr> gid_to_tid(int gid);

    int tid_to_gid(int tid, ConstTileSetPtr tileset) const;

    void swap(GidTidTranslator &);

private:
    template <bool kt_is_const>
    struct GidAndTileSetPtrImpl {
        using TsPtrType = typename std::conditional_t<kt_is_const, ConstTileSetPtr, TileSetPtr>;
        GidAndTileSetPtrImpl() {}
        GidAndTileSetPtrImpl(int sid, const TsPtrType & ptr): starting_id(sid), tileset(ptr) {}
        int starting_id = 0;
        TsPtrType tileset;
    };

    using GidAndTileSetPtr      = GidAndTileSetPtrImpl<false>;
    using GidAndConstTileSetPtr = GidAndTileSetPtrImpl<true>;

    static bool order_by_gids(const GidAndTileSetPtr &, const GidAndTileSetPtr &);
    static bool order_by_ptrs(const GidAndConstTileSetPtr &, const GidAndConstTileSetPtr &);

    std::vector<GidAndConstTileSetPtr> m_ptr_map;
    std::vector<GidAndTileSetPtr> m_gid_map;
    int m_gid_end = 0;
};

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

std::pair<int, GidTidTranslator::ConstTileSetPtr> GidTidTranslator::gid_to_tid(int gid) const {
    if (gid < 1 || gid >= m_gid_end) {
        throw InvArg("Given gid is either the empty tile or not contained in "
                     "this map; translatable gids: [1 " + std::to_string(m_gid_end)
                     + ").");
    }
    GidAndTileSetPtr samp;
    samp.starting_id = gid;
    auto itr = std::upper_bound(m_gid_map.begin(), m_gid_map.end(), samp, order_by_gids);
    if (itr == m_gid_map.begin()) {
        throw RtError("Library error: GidTidTranslator said that it owned a "
                      "gid, but does not have a tileset for it.");
    }
    --itr;
    assert(gid >= itr->starting_id);
    return std::make_pair(gid - itr->starting_id, itr->tileset);
}

std::pair<int, GidTidTranslator::TileSetPtr> GidTidTranslator::gid_to_tid(int gid) {
    const auto & const_this = *this;
    auto gv = const_this.gid_to_tid(gid);
    return std::make_pair(gv.first, std::const_pointer_cast<TileSet>(gv.second));
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

void GidTidTranslator::swap(GidTidTranslator & rhs) {
    m_ptr_map.swap(rhs.m_ptr_map);
    m_gid_map.swap(rhs.m_gid_map);

    std::swap(m_gid_end, rhs.m_gid_end);
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


class EntityAndTrianglesAdder final {
public:
    EntityAndTrianglesAdder(Loader::Callbacks & callbacks_, TrianglesAdder & adder_):
        m_callbacks(callbacks_), m_tri_adder(adder_) {}

    void add_triangle(SharedPtr<TriangleSegment> ptr)
        { m_tri_adder.add_triangle(ptr); }

    void add_entity(const Entity & ent)
        { m_callbacks.add_to_scene(ent); }

    TrianglesAdder & triangle_adder()
        { return m_tri_adder; }

private:
    Loader::Callbacks & m_callbacks;
    TrianglesAdder & m_tri_adder;
};

class TileFactory {
public:
    virtual ~TileFactory() {}

    class NeighborInfo final {
    public:
        NeighborInfo
            (SharedPtr<const TileSet> ts, const Grid<int> & layer,
             Vector2I tilelocmap, Vector2I spawner_offset):
            m_tileset(*ts), m_layer(layer), m_loc(tilelocmap),
            m_offset(spawner_offset) {}

        // +x second (east)
        Tuple<Real, Real> north_elevations() const;

        Tuple<Real, Real> south_elevations() const;

        // +z second (north)
        Tuple<Real, Real> east_elevations() const;

        Tuple<Real, Real> west_elevations() const;

        Vector2I tile_location() const { return m_loc + m_offset; }

        Vector2I tile_location_in_map() const { return m_loc; }
    private:
        const TileSet & m_tileset;
        const Grid<int> & m_layer;
        Vector2I m_loc;
        Vector2I m_offset;
    };

    virtual void operator ()
        (EntityAndTrianglesAdder &, const NeighborInfo &, Platform::ForLoaders &) const = 0;

    // turn xml parameters into components
    // how should I handle walls?
    // think I might be getting too mixed up

    static UniquePtr<TileFactory> tileset_factory
        (const char * type, tinyxml2::XMLElement * properties);

    virtual void setup(Vector2I loc_in_ts, tinyxml2::XMLElement * properties, Platform::ForLoaders &) = 0;

    // need all elevations, I need to prevent triangle link conflicts
    // I'm going to need it to account for translation, which will be different
    // than model's slopes
    virtual Slopes tile_elevations() const = 0;

    // as assign implies, these resources must outlive the life of the object
    void set_shared_texture_information
        (SharedCPtr<Texture> & texture_ptr_,
         Size2 & texture_size_,
         Size2 & tile_size_)
    {
        m_texture_ptr = texture_ptr_;
        m_texture_size = texture_size_;
        m_tile_size = tile_size_;
    }

protected:
    SharedCPtr<Texture> common_texture() const
        { return m_texture_ptr; }

    std::array<Vector2, 4> common_texture_positions_from(Vector2I ts_r) const {
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

    static Vector grid_position_to_v3(Vector2I r)
        { return Vector{r.x, 0, -r.y}; }

    static const char * find_property(const char * name_, tinyxml2::XMLElement * properties) {
        for (auto itr = properties; itr; itr = itr->NextSiblingElement("property")) {
            auto name = itr->Attribute("name");
            auto val = itr->Attribute("value");
            if (!val || !name) continue;
            if (strcmp(name, name_)) continue;
            return val;
        }
        return nullptr;
    }

    SharedCPtr<RenderModel> make_render_model_with_common_texture_positions
        (Platform::ForLoaders & platform,
         const Slopes & slopes,
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

    static void add_triangles_based_on_model_details
        (Vector2I gridloc, Vector translation, const Slopes & slopes,
         TrianglesAdder & adder)
    {
        const auto & els = TileGraphicGenerator::get_common_elements();
        const auto pos = TileGraphicGenerator::get_points_for(slopes);
        auto offset = grid_position_to_v3(gridloc) + translation;
        adder.add_triangle(make_shared<TriangleSegment>(
            pos[els[0]] + offset, pos[els[1]] + offset, pos[els[2]] + offset));
        adder.add_triangle(make_shared<TriangleSegment>(
            pos[els[3]] + offset, pos[els[4]] + offset, pos[els[5]] + offset));
    }

    Entity make_entity(Platform::ForLoaders & platform, Vector translation,
                       SharedCPtr<RenderModel> model_ptr) const
    {
        assert(model_ptr);
        auto ent = platform.make_renderable_entity();
        ent.add<SharedCPtr<RenderModel>, SharedCPtr<Texture>, Translation, Visible>() =
                make_tuple(model_ptr, common_texture(), Translation{translation}, true);
        return ent;
    }

private:
    SharedCPtr<Texture> m_texture_ptr;
    Size2 m_texture_size;
    Size2 m_tile_size;
};

/* static */ UniquePtr<TileFactory> make_tileset_factory(const char * type);

void TileSet::load_information(Platform::ForLoaders & platform, tinyxml2::XMLElement * tileset) {
    int tile_width = tileset->IntAttribute("tilewidth");
    int tile_height = tileset->IntAttribute("tileheight");
    int tile_count = tileset->IntAttribute("tilecount");
    auto to_ts_loc = [tileset] {
        int columns = tileset->IntAttribute("columns");
        return [columns] (int n) { return Vector2I{n % columns, n / columns}; };
    } ();

    auto image_el = tileset->FirstChildElement("image");
    int tx_width = image_el->IntAttribute("width");
    int tx_height = image_el->IntAttribute("height");

    auto tx = platform.make_texture();
    (*tx).load_from_file(image_el->Attribute("source"));

    set_texture_information(tx, Size2{tile_width, tile_height}, Size2{tx_width, tx_height});
    m_tile_count = tile_count;
    for (auto itr = tileset->FirstChildElement("tile"); itr;
         itr = itr->NextSiblingElement("tile"))
    {
        int id = itr->IntAttribute("id");
        auto tileset_factory = make_tileset_factory(itr->Attribute("type"));
        if (!tileset_factory)
            continue;
        insert_factory(std::move(tileset_factory), id)
            .setup(to_ts_loc(id),
                   itr->FirstChildElement("properties")->FirstChildElement("property"),
                   platform);
    }
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

class TranslatableTileFactory : public TileFactory {
public:
    void setup(Vector2I, tinyxml2::XMLElement * properties, Platform::ForLoaders &) override {
        // eugh... having to run through elements at a time
        // not gonna worry about it this iteration
        if (const auto * val = find_property("translation", properties)) {
            auto list = { &m_translation.x, &m_translation.y, &m_translation.z };
            auto itr = list.begin();
            for (auto value_str : split_range(val, val + ::strlen(val),
                                              is_comma, trim_whitespace))
            {
                bool is_num = cul::string_to_number(value_str.begin(), value_str.end(), **itr);
                assert(is_num);
                ++itr;
            }
        }
    }

protected:
    Vector translation() const { return m_translation; }

    Entity make_entity(Platform::ForLoaders & platform, Vector2I tile_loc,
                       SharedCPtr<RenderModel> model_ptr) const
    {
        return TileFactory::make_entity(platform,
            m_translation + grid_position_to_v3(tile_loc), model_ptr);
    }

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

class WallTileFactory final : public TranslatableTileFactory {
public:

    static void set_shared_texture_coordinates(const std::array<Vector2, 4> & coords)
        { s_wall_texture_coords = coords; }

private:

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
        (Vector2I loc_in_ts, tinyxml2::XMLElement * properties, Platform::ForLoaders &) final
    {
        m_dir = cardinal_direction_from(find_property("direction", properties));
        // visually factory wide: two render models...
        auto mk_base_pts = [] { return TileGraphicGenerator::get_points_for(Slopes{0, 0, 0, 0, 0}); };
        auto txposs = common_texture_positions_from(loc_in_ts);
        // ^ want to split that in half
        using Cd = CardinalDirections;
        switch (m_dir) {
        case Cd::n: case Cd::s: {
            // horizontal split
            auto arr = mk_base_pts();
            translate_points(Vector{0, 0, 0.5}, arr.begin(), arr.end());
            scale_points_z(0.5, arr.begin(), arr.end());
            ;
            }
            break;
        case Cd::w: case Cd::e:
            break;
        default: break;
        }
    }

    Slopes tile_elevations() const final {
        // it is possible that some elevations are indeterminent...
    }

    void operator ()
        (EntityAndTrianglesAdder & adder, const NeighborInfo & ninfo,
         Platform::ForLoaders & platform) const final
    {
        // it would be nice if I could share render models between similar walls
        // saves on models, and time building them
        // physical triangles wise:
        // the ledge extends *all* the to the end of the time, with no flooring
        // on the bottom
        // visually:
        // there appears to be a wall half-way through the tile

        // wall tiles may add multiple entities
        // one for the flat
        // wall needs to know something about its neighbors
        ninfo.tile_location();
    }

    SharedCPtr<RenderModel> m_bottom, m_top;
    CardinalDirections m_dir = CardinalDirections::ne;

    static std::array<Vector2, 4> s_wall_texture_coords;
};

class SlopesBasedModelTile : public TranslatableTileFactory {
protected:

    virtual Slopes model_tile_elevations() const = 0;

    Slopes tile_elevations() const final
        { return translate_y(model_tile_elevations(), translation().y); }

    Entity make_entity(Platform::ForLoaders & platform, Vector2I r) const {
        return TranslatableTileFactory::make_entity(platform, r, m_render_model);
    }

    void add_triangles_based_on_model_details(Vector2I gridloc, TrianglesAdder & adder) const {
        TileFactory::add_triangles_based_on_model_details(gridloc, translation(), model_tile_elevations(), adder);
    }

    void setup(Vector2I loc_in_ts, tinyxml2::XMLElement * properties, Platform::ForLoaders & platform) override {
        TranslatableTileFactory::setup(loc_in_ts, properties, platform);
        m_render_model = make_render_model_with_common_texture_positions(
            platform, model_tile_elevations(), loc_in_ts);
    }

    void operator ()
        (EntityAndTrianglesAdder & adder,
         const NeighborInfo & ninfo,
         Platform::ForLoaders & platform) const final
    {
        auto r = ninfo.tile_location();
        add_triangles_based_on_model_details(r, adder.triangle_adder());
        adder.add_entity(make_entity(platform, r));
    }

private:
    SharedCPtr<RenderModel> m_render_model;
};

class Ramp : public SlopesBasedModelTile {
public:
    template <typename T>
    static Tuple<const std::vector<Vector> &, const std::vector<unsigned> &>
        get_model_positions_and_elements_(const Slopes & slopes)
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

protected:
    virtual void set_direction(const char *) = 0;

    void setup(Vector2I loc_in_ts, tinyxml2::XMLElement * properties, Platform::ForLoaders & platform) final {
        if (const auto * val = find_property("direction", properties)) {
            set_direction(val);
        }
        SlopesBasedModelTile::setup(loc_in_ts, properties, platform);
    }
};

class CornerRamp : public Ramp {
protected:
    CornerRamp() {}

    virtual Slopes non_rotated_slopes() const = 0;

private:
    Slopes model_tile_elevations() const final
        { return m_slopes; }

    void set_direction(const char * dir) final {
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

    void set_direction(const char * dir) final {
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
    Slopes m_slopes;
};

class FlatTileFactory final : public SlopesBasedModelTile {
    Slopes model_tile_elevations() const final
        { return Slopes{0, 0, 0, 0, 0}; }
};

/* static */ UniquePtr<TileFactory> make_tileset_factory
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
    };
    auto itr = k_factory_map.find(type);
    if (itr == k_factory_map.end()) return nullptr;
    return (*itr->second)();
}

using tinyxml2::XMLDocument;

class TiledMapPreloader final : public Preloader {
public:
    TiledMapPreloader(const char * filename, Vector2I map_offset,
                      Platform::ForLoaders & platfrom):
        m_file_contents(platfrom.promise_file_contents(filename)),
        m_platform(platfrom),
        m_map_offset(map_offset)
    {}

    // another thing... maps could/should potentially be reloaded from the
    // loader
    UniquePtr<Loader> operator () () {
        if (m_file_contents) { if (m_file_contents->is_ready()) {
            do_content_load(m_file_contents->retrieve());
            m_file_contents = nullptr;
        }}
        if (check_has_remaining_pending_tilesets()) return nullptr;
        if (!m_file_contents && m_pending_tilesets.empty()) {
            m_tidgid_translater = GidTidTranslator{m_tilesets, m_startgids};
            Grid<int> layer = std::move(m_layer);
            // this call is deferred, "this" instance could be long gone
            auto tidgid_translator = std::move(m_tidgid_translater);
            auto map_offset = m_map_offset;

            return Loader::make_loader(
                [layer = std::move(layer),
                 tidgid_translator = std::move(tidgid_translator),
                 map_offset]
                (Loader::Callbacks & callbacks)
            {
                auto e = Entity::make_sceneless_entity();
                callbacks.add_to_scene(e);
                e.add<std::vector<TriangleLinks>>() =
                    add_triangles_and_link(layer.width(), layer.height(),
                    [&] (Vector2I r, TrianglesAdder adder)
                {
                    auto [tid, tileset] = tidgid_translator.gid_to_tid(layer(r)); {}
                    auto * factory = (*tileset)(tid);
                    if (!factory) return;

                    EntityAndTrianglesAdder etadder{callbacks, adder};
                    (*factory)(etadder,
                            TileFactory::NeighborInfo{tileset, layer, r, map_offset},
                            callbacks.platform());
                });
            });
        }
        return nullptr;
    }

private:
    bool check_has_remaining_pending_tilesets() {
        // no short circuting permitted, therefore STL sequence algorithms
        // not appropriate
        static constexpr const std::size_t k_no_idx = std::string::npos;
        bool rv = m_pending_tilesets.empty();
        for (auto & [idx, future] : m_pending_tilesets) {
            if (!future->is_ready()) continue;
            XMLDocument document;
            auto contents = future->retrieve();
            document.Parse(contents.c_str());
            m_tilesets[idx]->load_information(m_platform, document.RootElement());
            idx = k_no_idx;
        }
        auto end_itr = m_pending_tilesets.end();
        m_pending_tilesets.erase(
            std::remove_if(m_pending_tilesets.begin(), end_itr,
                [](const Tuple<std::size_t, FutureString> & tup)
                { return std::get<std::size_t>(tup) == k_no_idx; }),
            end_itr);
        return rv;
    }

    void do_content_load(std::string contents) {
        XMLDocument document;
        if (document.Parse(contents.c_str()) != tinyxml2::XML_SUCCESS) {
            // ...idk
            throw RtError{"Problem parsing XML x.x"};
        }

        auto * root = document.RootElement();

        for (auto itr = root->FirstChildElement("tileset"); itr;
             itr = itr->NextSiblingElement("tileset"))
        {
            add_tileset(itr);
        }

        auto * layer = root->FirstChildElement("layer");
        assert(layer);

        int width = layer->IntAttribute("width");
        int height = layer->IntAttribute("height");
        m_layer.set_size(width, height);
        auto * data = layer->FirstChildElement("data");
        assert(data);
        assert(!::strcmp(data->Attribute( "encoding" ), "csv"));
        auto data_text = data->GetText();
        assert(data_text);
        ; // and now I need a parsing helper for CSV strings
        Vector2I r;
        for (auto value_str : split_range(data_text, data_text + ::strlen(data_text),
                                          is_comma, trim_whitespace))
        {
            int tile_id = 0;
            bool is_num = cul::string_to_number(value_str.begin(), value_str.end(), tile_id);
            assert(is_num);
            m_layer(r) = tile_id;
            r = m_layer.next(r);
        }
    }

    void add_tileset(tinyxml2::XMLElement * tileset) {
        m_tilesets.emplace_back(make_shared<TileSet>());
        m_startgids.emplace_back(tileset->IntAttribute("firstgid"));
        if (const auto * source = tileset->Attribute("source")) {
            m_pending_tilesets.emplace_back(
                m_tilesets.size() - 1,
                m_platform.promise_file_contents(source));
        } else {
            m_tilesets.back()->load_information(m_platform, tileset);
        }
    }

    FutureString m_file_contents;

    Grid<int> m_layer;
    Platform::ForLoaders & m_platform;

    std::vector<SharedPtr<TileSet>> m_tilesets;
    std::vector<int> m_startgids;

    std::vector<Tuple<std::size_t, FutureString>> m_pending_tilesets;
    Vector2I m_map_offset;
    GidTidTranslator m_tidgid_translater;
};

} // end of <anonymous> namespace

UniquePtr<Preloader> make_tiled_map_preloader
    (const char * filename, Vector2I map_offset, Platform::ForLoaders & platform)
{
    return make_unique<TiledMapPreloader>(filename, map_offset, platform);
}
