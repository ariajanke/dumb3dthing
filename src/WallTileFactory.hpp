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

class WallTileFactory final : public TranslatableTileFactory {
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
        k_flats_only = 1 << 0,
        k_wall_only  = 1 << 1,
        k_both_flats_and_wall = k_flats_only | k_wall_only
    };

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


    void assign_render_model_wall_cache(WallRenderModelCache & cache) final
        { m_render_model_cache = &cache; }

    void setup
        (Vector2I loc_in_ts, const tinyxml2::XMLElement * properties, Platform::ForLoaders & platform) final;

    Slopes tile_elevations() const final;

private:
    using Triangle = TriangleSegment;

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
        const NeighborInfo & ninfo,
        Platform::ForLoaders &) const;

    CardinalDirection m_dir = CardinalDirection::ne;
    WallRenderModelCache * m_render_model_cache = nullptr;
    Vector2I m_tileset_location;

    // I still need to known the wall texture coords
};
