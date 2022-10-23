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

#include "TileSet.hpp"
#include "TileFactory.hpp"
#include "RenderModel.hpp"

#include <iostream>

class TranslatableTileFactory : public TileFactory {
public:
    void setup(Vector2I, const tinyxml2::XMLElement * properties,
               Platform::ForLoaders &) override;

protected:
    Vector translation() const { return m_translation; }

    Entity make_entity(Platform::ForLoaders & platform, Vector2I tile_loc,
                       SharedCPtr<RenderModel> model_ptr) const;

private:
    Vector m_translation;
};

// ----------------------------------------------------------------------------

CardinalDirection cardinal_direction_from(const char * str);

// ramps are split into three classes, why not walls?

class WallTileFactoryBase : public TranslatableTileFactory {
public:
    class TriangleAdder {
    public:
        virtual ~TriangleAdder() {}

        virtual void operator ()(const TriangleSegment &) const = 0;

        template <typename Func>
        static auto make(Func && f) {
            class Impl final : public TriangleAdder {
            public:
                explicit Impl(Func && f_): m_f(std::move(f_)) {}

                void operator () (const TriangleSegment & tri) const final
                    { m_f(tri); }
            private:
                Func m_f;
            };
            return Impl{std::move(f)};
        }
    };

    enum SplitOpt {
        k_bottom_only         = 1 << 0,
        k_top_only            = 1 << 1,
        k_wall_only           = 1 << 2,
        k_both_flats_and_wall = k_bottom_only | k_top_only | k_wall_only
    };
    using TileTexture = TileSet::TileTexture;

    // design flaw? too many parameters?
    static void add_wall_triangles_to
        (CardinalDirection dir, Real nw, Real sw, Real se, Real ne, SplitOpt,
         Real division, const TriangleAdder &);

    static int corner_index(CardinalDirection dir);

    static WallElevationAndDirection elevations_and_direction
        (const NeighborInfo & ninfo, Real known_elevation,
         CardinalDirection dir, Vector2I tile_loc);

    void operator ()
        (EntityAndTrianglesAdder & adder, const NeighborInfo & ninfo,
         Platform::ForLoaders & platform) const final
    { make_tile(adder, ninfo, platform); }

    void assign_render_model_wall_cache(WallRenderModelCache & cache)
        { m_render_model_cache = &cache; }

    void setup
        (Vector2I loc_in_ts, const tinyxml2::XMLElement * properties, Platform::ForLoaders & platform) final;

    Slopes tile_elevations() const final;

    void assign_wall_texture(const TileTexture & tt)
        { m_wall_texture_coords = &tt; }

private:
    using Triangle = TriangleSegment;

    static constexpr const Real k_visual_dip_thershold   = -0.25;
    static constexpr const Real k_physical_dip_thershold = -0.5;

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

    void make_tile
        (EntityAndTrianglesAdder & adder, const NeighborInfo & ninfo,
         Platform::ForLoaders & platform) const;

    void make_entities_and_triangles(
        EntityAndTrianglesAdder & adder,
        Platform::ForLoaders & platform,
        const NeighborInfo & ninfo,
        const SharedPtr<const RenderModel> & render_model,
        const std::vector<Triangle> & triangles) const;

    static std::array<bool, 4> make_known_corners(CardinalDirection dir);

    WallElevationAndDirection elevations_and_direction
        (const NeighborInfo & ninfo) const;

    Tuple<SharedPtr<const RenderModel>, std::vector<Triangle>>
        make_render_model_and_triangles(
        const WallElevationAndDirection & wed,
        Platform::ForLoaders &) const;

    CardinalDirection m_dir = CardinalDirection::ne;
    WallRenderModelCache * m_render_model_cache = nullptr;
    Vector2I m_tileset_location;

    // I still need to known the wall texture coords
    const TileTexture * m_wall_texture_coords = nullptr;
};

class WallTileFactory final : public WallTileFactoryBase {};

// want to "cache" graphics
// graphics are created as needed
// physical triangles need not be reused

class WallTileGraphicKey final {
public:
    CardinalDirection direction;
    std::array<Real, 4> dip_heights;

    bool operator == (const WallTileGraphicKey & rhs) const noexcept
        { return compare(rhs) == 0; }

    bool operator != (const WallTileGraphicKey & rhs) const noexcept
        { return compare(rhs) != 0; }

    bool operator < (const WallTileGraphicKey & rhs) const noexcept
        { return compare(rhs) < 0; }

private:
    template <typename T, std::size_t kt_size>
    static T difference_between
        (const std::array<T, kt_size> & lhs, const std::array<T, kt_size> & rhs)
    {
        for (int i = 0; i != int(lhs.size()); ++i) {
            auto diff = lhs[i] - rhs[i];
            if (!are_very_close(diff, 0)) // <- should be okay for both fp & int
                return diff;
        }
        return 0;
    }

    int compare(const WallTileGraphicKey & rhs) const noexcept {
        auto diff = static_cast<int>(direction) - static_cast<int>(rhs.direction);
        if (diff) return diff;

        auto slopes_diff = difference_between(dip_heights, rhs.dip_heights);
        if (are_very_close(slopes_diff, 0))
            return 0;

        // do not truncate to zero
        return slopes_diff < 0 ? -1 : 1;
    }
};

class WallTileFactoryBaseN : public TranslatableTileFactory {
public:
    using Triangle = TriangleSegment;
    using TileTexture = TileSet::TileTexture;
    using SplitOpt = WallTileFactory::SplitOpt;

    void operator ()
        (EntityAndTrianglesAdder & adder, const NeighborInfo & ninfo,
         Platform::ForLoaders & platform) const final
    {
        // physical triangles
        make_physical_triangles(ninfo, adder);

        // top
        auto tile_loc = ninfo.tile_location();
        adder.add_entity(make_entity(platform, tile_loc, m_top_model));

        // wall graphics
        adder.add_entity(make_entity(
            platform, tile_loc, ensure_wall_graphics(ninfo, platform)));

        // bottom
        adder.add_entity(make_entity(
            platform, tile_loc, ensure_bottom_model(ninfo, platform)));
    }

    static SharedPtr<const RenderModel> make_wall_graphic_model
        (const NeighborInfo &, const Platform::ForLoaders & platform);

    // should have translations and all
    virtual void make_physical_triangles
        (const NeighborInfo &, EntityAndTrianglesAdder &) const = 0;

    void setup
        (Vector2I loc_in_ts, const TiXmlElement * properties, Platform::ForLoaders & platform) final
    {
        TranslatableTileFactory::setup(loc_in_ts, properties, platform);
        m_dir = verify_okay_wall_direction(cardinal_direction_from(find_property("direction", properties)));
        m_tileset_location = loc_in_ts;
        m_top_model = make_top_model(platform);
    }

    Slopes tile_elevations() const final {
        using Cd = CardinalDirection;
        auto is_known_corner = [this] {
            auto knowns = make_known_corners(m_dir);
            return [knowns] (Cd dir)
                { return knowns[corner_index(dir)]; };
        } ();

        auto elevation_for_corner = [this, &is_known_corner] {
            Real y = known_elevation();
            return [y, &is_known_corner] (Cd dir) {
                return is_known_corner(dir) ? y : k_inf;
            };
        } ();

        return Slopes{0,
            elevation_for_corner(Cd::ne), elevation_for_corner(Cd::nw),
            elevation_for_corner(Cd::sw), elevation_for_corner(Cd::se)};
    }

    Slopes computed_tile_elevations(const NeighborInfo & ninfo) const {
        using Cd = CardinalDirection;
        auto slopes = tile_elevations();
        auto update_corner = [&ninfo] (Real & x, Cd dir) {
            if (cul::is_real(x)) return;
            x = ninfo.neighbor_elevation(dir);
        };
        update_corner(slopes.nw, Cd::nw);
        update_corner(slopes.ne, Cd::ne);
        update_corner(slopes.se, Cd::se);
        update_corner(slopes.sw, Cd::sw);
        return slopes;
    }

    // to make a tile:
    // cache each "type" of graphic
    // - graphics
    //   - top
    //     - always the same between tiles
    //   - bottom
    //     - maybe different per tile
    //   - walls
    //     - maybe different per tile
    //     - one, at most two
    // - physics (easy)
    //   - triangles
    // - entities
    //   - contain graphics

    void assign_wall_texture(const TileTexture & tt)
        { m_wall_texture_coords = &tt; }

protected:
    static constexpr const Real k_visual_dip_thershold   = -0.25;
    static constexpr const Real k_physical_dip_thershold = -0.5;

    // is/maybe cached
    // bottom model elevations are determined by sub class's method:
    // tile_elevations
    virtual SharedPtr<const RenderModel> ensure_bottom_model
        (const NeighborInfo &, Platform::ForLoaders &) const = 0;

    // ensure relates to: whether it's in the cache, or not
    virtual SharedPtr<const RenderModel> ensure_wall_graphics
        (const NeighborInfo &, Platform::ForLoaders &) const = 0;

    // each subtype seems to have their own wall segments for graphics,
    // though shared between instances (no statics please)

    CardinalDirection direction() const noexcept
        { return m_dir; }

    static int corner_index(CardinalDirection dir) {
        using Cd = CardinalDirection;
        switch (dir) {
        case Cd::nw: return 0;
        case Cd::sw: return 1;
        case Cd::se: return 2;
        case Cd::ne: return 3;
        default: break;
        }
        throw BadBranchException{__LINE__, __FILE__};
    }

    static std::array<bool, 4> make_known_corners(CardinalDirection dir) {
        using Cd = CardinalDirection;
        auto mk_rv = [](bool nw, bool sw, bool se, bool ne) {
            std::array<bool, 4> rv;
            auto set_corner = [&rv] (Cd dir, bool val)
                { rv[corner_index(dir)] = val; };
            set_corner(Cd::nw, nw);
            set_corner(Cd::ne, ne);
            set_corner(Cd::sw, sw);
            set_corner(Cd::se, se);
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

    static std::array<Tuple<bool, CardinalDirection>, 4>
        make_known_corners_with_preposition(CardinalDirection dir)
    {
        using Cd = CardinalDirection;
        auto knowns = make_known_corners(dir);
        return {
            make_tuple(knowns[corner_index(Cd::ne)], Cd::ne),
            make_tuple(knowns[corner_index(Cd::nw)], Cd::nw),
            make_tuple(knowns[corner_index(Cd::sw)], Cd::sw),
            make_tuple(knowns[corner_index(Cd::se)], Cd::se),
        };
    }

    Real known_elevation() const
        { return translation().y + 1; }

    WallTileGraphicKey graphic_key(const NeighborInfo & ninfo) const {
        WallTileGraphicKey key;
        key.direction = m_dir;

        const auto known_elevation = translation().y + 1;
        for (auto [known, corner] : make_known_corners_with_preposition(m_dir)) {
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
            key.dip_heights[corner_index(corner)] = is_dip ? diff : 0;
        }
        return key;
    }

    using GraphicMap = std::map<WallTileGraphicKey, WeakPtr<const RenderModel>>;

    template <typename MakerFunc>
    SharedPtr<const RenderModel> ensure_model
        (const NeighborInfo & neighborhood, GraphicMap & graphic_map,
         MakerFunc && make_model) const
    {
        auto key = graphic_key(neighborhood);
        auto itr = graphic_map.find(key);
        if (itr == graphic_map.end()) {
            auto new_pair = std::make_pair(key, WeakPtr<const RenderModel>{});
            itr = std::get<0>(graphic_map.insert(new_pair));
            std::cout << graphic_map.size() << std::endl;
        }

        if (auto rv = itr->second.lock())
            return rv;

        SharedPtr<const RenderModel> rv = make_model();
        itr->second = rv;
        return rv;
    }

    virtual bool is_okay_wall_direction(CardinalDirection) const noexcept = 0;

    virtual SharedPtr<const RenderModel> make_top_model(Platform::ForLoaders & platform) const = 0;

    TileTexture wall_texture() const
        { return *m_wall_texture_coords; }

    TileTexture floor_texture() const;

    using TriangleAdder = WallTileFactory::TriangleAdder;
    class TriangleToVerticies {
    public:
        virtual ~TriangleToVerticies() {}

        virtual std::array<Vertex, 3> operator () (const Triangle &) const = 0;
    };

    class TriangleToFloorVerticies final : public TriangleToVerticies {
    public:
        TriangleToFloorVerticies(const TileTexture & ttex_, Real ytrans):
            m_ttex(ttex_), m_ytrans(ytrans) {}

        std::array<Vertex, 3> operator () (const Triangle & triangle) const final {
            auto to_vtx = [this] (const Vector & r) {
                Vector2 tx{r.x + 0.5, -r.z + 0.5};
                return Vertex{r, Vector2{
                    tx.x*m_ttex.se.x + m_ttex.nw.x*(1 - tx.x),
                    tx.y*m_ttex.se.y + m_ttex.nw.y*(1 - tx.y)
                }};
            };
            const auto tri_ = triangle.move(Vector{0, m_ytrans, 0});
            return std::array<Vertex, 3>{
                to_vtx(tri_.point_a()),
                to_vtx(tri_.point_b()),
                to_vtx(tri_.point_c()),
            };
        }

    private:
        TileTexture m_ttex;
        Real m_ytrans;
    };

    template <typename Func>
    static auto make_triangle_to_verticies(Func && f) {
        class Impl final : public TriangleToVerticies {
        public:
            explicit Impl(Func && f_):
                m_f(std::move(f_)) {}

            std::array<Vertex, 3> operator () (const Triangle & tri) const final
                { return m_f(tri); }

        private:
            Func m_f;
        };

        return Impl{std::move(f)};
    }

    auto make_triangle_to_floor_verticies() const
        { return TriangleToFloorVerticies{floor_texture(), -translation().y}; }

private:
    CardinalDirection verify_okay_wall_direction(CardinalDirection dir) const {
        if (!is_okay_wall_direction(dir)) {
            throw std::invalid_argument{"bad wall"};
        }
        return dir;
    }

    CardinalDirection m_dir = CardinalDirection::ne;
    Vector2I m_tileset_location;
    const TileTexture * m_wall_texture_coords = &s_default_texture;
    SharedPtr<const RenderModel> m_top_model;

    static const TileTexture s_default_texture;
};

class TwoWayWallTileFactory final : public WallTileFactoryBaseN {
public:
    // two known corners
    // two unknown corners
    // bottom:
    // - rectangle whose sides may have different elevations
    // top:
    // - both elevations are fixed
    // wall:
    // - single flat wall
    // if I use a common key, that should simplify things

    void make_physical_triangles(const NeighborInfo &, EntityAndTrianglesAdder &) const final;

private:
    SharedPtr<const RenderModel> ensure_bottom_model
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const final
    {
        return ensure_model
            (neighborhood, s_bottom_cache,
             [&] { return make_bottom_graphics(neighborhood, platform); });
    }

    SharedPtr<const RenderModel>
        ensure_wall_graphics
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const final
    {
        return ensure_model
            (neighborhood, s_cache,
             [&] { return make_wall_graphics(neighborhood, platform); });
    }

    bool is_okay_wall_direction(CardinalDirection dir) const noexcept final {
        using Cd = CardinalDirection;
        constexpr static std::array k_list = { Cd::n, Cd::e, Cd::s, Cd::w };
        return std::find(k_list.begin(), k_list.end(), dir) != k_list.end();
    }

    SharedPtr<const RenderModel> make_top_model(Platform::ForLoaders & platform) const final;

    SharedPtr<const RenderModel>
        make_wall_graphics
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const;

    SharedPtr<const RenderModel>
        make_bottom_graphics
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const;

    SharedPtr<const RenderModel>
        make_model_graphics
        (const Slopes & elevations, SplitOpt,
         const TriangleToVerticies &, Platform::ForLoaders & platform) const;

    void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const;

    static GraphicMap s_cache;
    static GraphicMap s_bottom_cache;
};

// in wall
// top: flat is always true flat
// bottom: not so, variable elevations
// wall "elements": can be shared between instances
// - whole squares
// - triangle peices
//
// both in and out walls split at the same place
//
// ensure top
// ensure bottom
// ensure wall(s)
// in and out have two
// two-way has only one

class CornerWallTileFactory : public WallTileFactoryBaseN {
protected:
    bool is_okay_wall_direction(CardinalDirection dir) const noexcept final {
        using Cd = CardinalDirection;
        constexpr static std::array k_list = { Cd::ne, Cd::nw, Cd::se, Cd::sw };
        return std::find(k_list.begin(), k_list.end(), dir) != k_list.end();
    }
};

class InWallTileFactory final : public CornerWallTileFactory {
public:
    // three known corners
    // one unknown corner

    void make_physical_triangles(const NeighborInfo &, EntityAndTrianglesAdder &) const final;

private:
    SharedPtr<const RenderModel> ensure_bottom_model
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const final
    {
        return ensure_model
            (neighborhood, s_bottom_cache,
             [&] { return make_bottom_graphics(neighborhood, platform); });
    }

    SharedPtr<const RenderModel>
        ensure_wall_graphics
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const final
    {
        return ensure_model
            (neighborhood, s_cache,
             [&] { return make_wall_graphics(neighborhood, platform); });
    }

    SharedPtr<const RenderModel>
        make_wall_graphics
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const;

    SharedPtr<const RenderModel>
        make_bottom_graphics
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const;

    SharedPtr<const RenderModel>
        make_model_graphics
        (const Slopes & elevations, SplitOpt,
         const TriangleToVerticies &, Platform::ForLoaders & platform) const;

    void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const;

    SharedPtr<const RenderModel> make_top_model(Platform::ForLoaders & platform) const final;

    static GraphicMap s_cache;
    static GraphicMap s_bottom_cache;
};

class OutWallTileFactory final : public CornerWallTileFactory {
public:
    // one known corner
    // three unknown corners

    void make_physical_triangles(const NeighborInfo &, EntityAndTrianglesAdder &) const final;

private:
    SharedPtr<const RenderModel> ensure_bottom_model
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const final
    {
        return ensure_model
            (neighborhood, s_bottom_cache,
             [&] { return make_bottom_graphics(neighborhood, platform); });
    }

    SharedPtr<const RenderModel>
        ensure_wall_graphics
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const final
    {
        return ensure_model
            (neighborhood, s_cache,
             [&] { return make_wall_graphics(neighborhood, platform); });
    }

    SharedPtr<const RenderModel>
        make_wall_graphics
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const;

    SharedPtr<const RenderModel>
        make_bottom_graphics
        (const NeighborInfo & neighborhood, Platform::ForLoaders & platform) const;

    SharedPtr<const RenderModel>
        make_model_graphics
        (const Slopes & elevations, SplitOpt,
         const TriangleToVerticies &, Platform::ForLoaders & platform) const;

    void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const;

    SharedPtr<const RenderModel> make_top_model(Platform::ForLoaders & platform) const final;

    static GraphicMap s_cache;
    static GraphicMap s_bottom_cache;
};
