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
    bool is_at_end() const noexcept {
        return m_beg == m_end;
    }

    void update_end_segment() {
        m_segment_end = std::find_if(m_beg, m_end, m_splitter_f);
    }

    void move_to_next() {
        m_beg = m_segment_end;
        if (m_beg != m_end) ++m_beg;
    }

    CharIter m_beg, m_segment_end, m_end;
    SplitterFunc m_splitter_f;
    WithAdditionalFunc m_with_f;
};

constexpr const auto is_comma = [](char c) { return c == ','; };//  bool is_comma(char c) { return c == ','; }
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

class TileLoader;

using TileSet = std::unordered_map<int, UniquePtr<TileLoader>>;

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

class TileLoader {
public:
    virtual ~TileLoader() {}

    class NeighborInfo final {
    public:
        NeighborInfo
            (const TileSet & ts, const Grid<int> & layer, Vector2I tileloc):
            m_tileset(ts), m_layer(layer), m_loc(tileloc) {}

        // +x second (east)
        Tuple<Real, Real> north_elevations() const;

        Tuple<Real, Real> south_elevations() const;

        // +z second (north)
        Tuple<Real, Real> east_elevations() const;

        Tuple<Real, Real> west_elevations() const;

        Vector2I tile_location() const { return m_loc; }

    private:
        const TileSet & m_tileset;
        const Grid<int> & m_layer;
        Vector2I m_loc;
    };

    virtual void operator ()
        (EntityAndTrianglesAdder &, const NeighborInfo &, Platform::ForLoaders &) const = 0;

    // turn xml parameters into components
    // how should I handle walls?
    // think I might be getting too mixed up

    static UniquePtr<TileLoader> tileset_factory
        (const char * type, tinyxml2::XMLElement * properties);

    static void set_common_texture(SharedCPtr<Texture> txptr) {
        s_texture = txptr;

    }

    static void set_tileset_size_info(Vector2 tssize, Vector2 tilesize) {
        s_texture_size = tssize;
        s_tile_size = tilesize;
    }

    virtual void setup(Vector2I loc_in_ts, tinyxml2::XMLElement * properties, Platform::ForLoaders &) = 0;

    // need all elevations, I need to prevent triangle link conflicts
    // I'm going to need it to account for translation, which will be different
    // than model's slopes
    virtual Slopes tile_elevations() const = 0;

protected:
    static SharedCPtr<Texture> common_texture()
        { return s_texture; }

    static std::array<Vector2, 4> common_texture_positions_from(Vector2I ts_r) {
        const Real x_scale = s_tile_size.x / s_texture_size.x;
        const Real y_scale = s_tile_size.y / s_texture_size.y;
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

    static Vector grid_position_to_v3(Vector2I r) {
        return Vector{r.x, 0, -r.y};
    }

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

    static SharedCPtr<RenderModel> make_render_model_with_common_texture_positions
        (Platform::ForLoaders & platform,
         const Slopes & slopes,
         Vector2I loc_in_ts)
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


    static SharedCPtr<RenderModel> make_render_model_with_common_texture_positions
        (Platform::ForLoaders & platform,
         const Tuple<const std::vector<Vector> &, const std::vector<unsigned> &> & tup,
         Vector2I loc_in_ts)
    {
        const auto & [pos, els] = tup; {}
        auto txpos = common_texture_positions_from(loc_in_ts);

        std::vector<Vertex> verticies;
        assert(txpos.size() == pos.size());
        for (int i = 0; i != int(pos.size()); ++i) {
            verticies.emplace_back(pos[i], txpos[i]);
        }

        auto render_model = platform.make_render_model(); // need platform
        render_model->load(verticies, els);
        return render_model;
    }

    static void add_triangles_based_on_model_details
        (Vector2I gridloc,
         const Tuple<const std::vector<Vector> &, const std::vector<unsigned> &> & tup,
         TrianglesAdder & adder)
    {
        const auto & [pos, els] = tup; {}
        assert(els.size() == 6);
        auto offset = grid_position_to_v3(gridloc);
        adder.add_triangle(make_shared<TriangleSegment>(
            pos[els[0]] + offset, pos[els[1]] + offset, pos[els[2]] + offset));
        adder.add_triangle(make_shared<TriangleSegment>(
            pos[els[3]] + offset, pos[els[4]] + offset, pos[els[5]] + offset));
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
        ent.add<SharedCPtr<RenderModel>, SharedCPtr<Texture>, Translation>() =
                make_tuple(model_ptr, common_texture(), Translation{translation});
        return ent;
    }

private:
    static SharedCPtr<Texture> s_texture;
    static Vector2 s_texture_size;
    static Vector2 s_tile_size;
};
/* static */ SharedCPtr<Texture> TileLoader::s_texture;
/* static */ Vector2 TileLoader::s_texture_size;
/* static */ Vector2 TileLoader::s_tile_size;

class TranslatableTile : public TileLoader {
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
        return TileLoader::make_entity(platform,
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
#if 0
YRotation corner_direction_to_rotation(const char * dir_) {
    using Cd = CardinalDirections;
    switch (cardinal_direction_from(dir_)) {
    case Cd::nw: return YRotation{0};
    case Cd::sw: return YRotation{k_pi*0.5};
    case Cd::se: return YRotation{k_pi};
    case Cd::ne: return YRotation{k_pi*1.5};
    default: throw InvArg{""};
    }
}
#endif
class WallTile final : public TranslatableTile {
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

class SlopesBasedModelTile : public TranslatableTile {
protected:

    virtual Slopes model_tile_elevations() const = 0;

    Slopes tile_elevations() const final
        { return translate_y(model_tile_elevations(), translation().y); }

    Entity make_entity(Platform::ForLoaders & platform, Vector2I r) const {
        return TranslatableTile::make_entity(platform, r, m_render_model);
    }

    void add_triangles_based_on_model_details(Vector2I gridloc, TrianglesAdder & adder) const {
        TileLoader::add_triangles_based_on_model_details(gridloc, translation(), model_tile_elevations(), adder);
    }

    void setup(Vector2I loc_in_ts, tinyxml2::XMLElement * properties, Platform::ForLoaders & platform) override {
        TranslatableTile::setup(loc_in_ts, properties, platform);
        m_render_model = make_render_model_with_common_texture_positions(
            platform, model_tile_elevations(), loc_in_ts);
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
#   if 0
    virtual YRotation direction_to_rotation(const char *) const = 0;
#   endif
    virtual void set_direction(const char *) = 0;

    void operator ()
        (EntityAndTrianglesAdder & adder,
         const NeighborInfo & ninfo,
         Platform::ForLoaders & platform) const final
    {
        auto r = ninfo.tile_location();
        add_triangles_based_on_model_details(r, adder.triangle_adder());
        adder.add_entity(make_entity(platform, r));
    }

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
#   if 0
    YRotation direction_to_rotation(const char * val) const final {
        return corner_direction_to_rotation(val);
    }
#   endif
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

class InRamp final : public CornerRamp {
    Slopes non_rotated_slopes() const final
        { return Slopes{0, 1, 1, 1, 0}; }
};

class OutRamp final : public CornerRamp {
    Slopes non_rotated_slopes() const final
        { return Slopes{0, 0, 0, 0, 1}; }
};

class TwoRamp final : public Ramp {
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
#   if 0
    YRotation direction_to_rotation(const char * val) const final {
        using Cd = CardinalDirections;
        switch (cardinal_direction_from(val)) {
        case Cd::n: return YRotation{-0};
        case Cd::w: return YRotation{-k_pi*0.5};
        case Cd::s: return YRotation{-k_pi};
        case Cd::e: return YRotation{-k_pi*1.5};
        default: throw InvArg{""};
        }
    }
#   endif
    Slopes m_slopes;
};

class FlatTile final : public SlopesBasedModelTile {

    Slopes model_tile_elevations() const final
        { return Slopes{0, 0, 0, 0, 0}; }

    void operator ()
        (EntityAndTrianglesAdder & adder,
         const NeighborInfo & ninfo,
         Platform::ForLoaders & platform) const final
    {
        auto r = ninfo.tile_location();
        add_triangles_based_on_model_details(r, adder.triangle_adder());
        adder.add_entity(make_entity(platform, r));
    }
};

/* static */ UniquePtr<TileLoader> tileset_factory
    (const char * type)
{
    using TlPtr = UniquePtr<TileLoader>;
    using FnPtr = UniquePtr<TileLoader>(*)();
    using std::make_pair;
    static const std::map<std::string, FnPtr> k_factory_map {
        make_pair("flat"    , [] { return TlPtr{make_unique<FlatTile>()}; }),
        make_pair("ramp"    , [] { return TlPtr{make_unique<TwoRamp >()}; }),
        make_pair("in-ramp" , [] { return TlPtr{make_unique<InRamp  >()}; }),
        make_pair("out-ramp", [] { return TlPtr{make_unique<OutRamp >()}; }),
    };
    auto itr = k_factory_map.find(type);
    if (itr == k_factory_map.end()) return nullptr;
    return (*itr->second)();
}

using tinyxml2::XMLDocument;

class TiledMapPreloader final : public Preloader {
public:
    TiledMapPreloader(const char * filename, Platform::ForLoaders & platfrom):
        m_file_contents(platfrom.promise_file_contents(filename)),
        m_platform(platfrom)
    {}

    // another thing... maps could/should potentially be reloaded from the
    // loader
    UniquePtr<Loader> operator () () {
        if (m_file_contents) { if (m_file_contents->is_ready()) {
            do_content_load(m_file_contents->retrieve());
            m_file_contents = nullptr;
        }}
        if (m_tileset_contents) { if (m_tileset_contents->is_ready()) {
            do_tileset_load(m_tileset_contents->retrieve());
            m_tileset_contents = nullptr;
        }}
        if (!m_file_contents && !m_tileset_contents) {
            Grid<int> layer = std::move(m_layer);
            TileSet tileset = std::move(m_tileset);
            return Loader::make_loader(
                [layer = std::move(layer), tileset = std::move(tileset)]
                (Loader::Callbacks & callbacks)
            {
                auto e = Entity::make_sceneless_entity();
                callbacks.add_to_scene(e);
                e.add<std::vector<TriangleLinks>>() =
                    add_triangles_and_link(layer.width(), layer.height(),
                    [&] (Vector2I r, TrianglesAdder adder)
                {
                    auto itr = tileset.find(layer(r) - 1);
                    if (itr == tileset.end()) return;
                    auto & uptr = itr->second;
                    EntityAndTrianglesAdder etadder{callbacks, adder};
                    (*uptr)(etadder, TileLoader::NeighborInfo{tileset, layer, r}, callbacks.platform());
                });
                TileLoader::set_common_texture(nullptr);
            });
        }
        return nullptr;
    }

private:
    void do_content_load(std::string contents) {
        XMLDocument document;
        if (document.Parse(contents.c_str()) != tinyxml2::XML_SUCCESS) {
            // ...idk
            throw RtError{"Problem parsing XML x.x"};
        }

        auto * root = document.RootElement();
        // don't wanna do a whole lot of checking...
        // I'm going to make a *ton* of assumptions
        auto itr = root->FirstChildElement("tileset");
        assert(itr);
        auto * source = itr->Attribute("source");
        assert(source);
        m_tileset_contents = m_platform.promise_file_contents(source);
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

    void do_tileset_load(std::string tileset_contents) {
        XMLDocument document;
        document.Parse(tileset_contents.c_str());
        // this part is much harder
        // we're not quite at a complete picture of the new map, but close
        TileSet & tileset = m_tileset;
        auto ts_el = document.RootElement();//->FirstChildElement("tileset");
        int tile_width = ts_el->IntAttribute("tilewidth");
        int tile_height = ts_el->IntAttribute("tileheight");
        auto to_ts_loc = [ts_el]() {
            int columns = ts_el->IntAttribute("columns");
            return [columns] (int n) { return Vector2I{n % columns, n / columns}; };
        } ();

        auto image_el = ts_el->FirstChildElement("image");
        int tx_width = image_el->IntAttribute("width");
        int tx_height = image_el->IntAttribute("height");
        TileLoader::set_tileset_size_info(Vector2{tx_width, tx_height}, Vector2{tile_width, tile_height});
        auto tx = m_platform.make_texture();
        (*tx).load_from_file(image_el->Attribute("source"));
        TileLoader::set_common_texture(tx);
        for (auto itr = ts_el->FirstChildElement("tile"); itr;
             itr = itr->NextSiblingElement("tile"))
        {
            int id = itr->IntAttribute("id");
            auto tileset_tile = tileset_factory(itr->Attribute("type"));
            if (!tileset_tile)
                continue;
            tileset[id] = std::move(tileset_tile);
            tileset[id]->setup(to_ts_loc(id),
                itr->FirstChildElement("properties")->FirstChildElement("property"),
                m_platform);
        }
    }

    FutureString m_file_contents;
    FutureString m_tileset_contents;
    Grid<int> m_layer;
    Platform::ForLoaders & m_platform;
    TileSet m_tileset;
};

}

UniquePtr<Preloader> make_tiled_map_preloader
    (const char * filename, Platform::ForLoaders & platform)
{
    return make_unique<TiledMapPreloader>(filename, platform);
}
