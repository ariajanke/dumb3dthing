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

#pragma once

#include "SlopesBasedTileFactory.hpp"

#include "../../RenderModel.hpp"
#include "../../platform.hpp"

#include <bitset>

// want to "cache" graphics
// graphics are created as needed
// physical triangles need not be reused

template <typename T>
class CornersArray final {
public:
    CornersArray & set(CardinalDirection dir, const T & val) {
        m_values[corner_index(dir)] = val;
        return *this;
    }

    auto & nw(const T & val)
        { return set(CardinalDirection::nw, val); }

    auto & sw(const T & val)
        { return set(CardinalDirection::sw, val); }

    auto & ne(const T & val)
        { return set(CardinalDirection::ne, val); }

    auto & se(const T & val)
        { return set(CardinalDirection::se, val); }

    T operator [] (CardinalDirection dir) const
        { return m_values[corner_index(dir)]; }

    T & operator [] (CardinalDirection dir)
        { return m_values[corner_index(dir)]; }

private:
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

    std::array<T, 4> m_values;
};

class WallTileGraphicKey final {
public:
    CardinalDirection direction;
    CornersArray<Real> dip_heights;

    bool operator == (const WallTileGraphicKey & rhs) const noexcept
        { return compare(rhs) == 0; }

    bool operator != (const WallTileGraphicKey & rhs) const noexcept
        { return compare(rhs) != 0; }

    bool operator < (const WallTileGraphicKey & rhs) const noexcept
        { return compare(rhs) < 0; }

private:
    static Real difference_between
        (const CornersArray<Real> & lhs, const CornersArray<Real> & rhs);

    int compare(const WallTileGraphicKey & rhs) const noexcept;
};

class TriangleToVerticies {
public:
    virtual ~TriangleToVerticies() {}

    using VertexArray = std::array<Vertex, 3>;
    using Triangle = TriangleSegment;

    virtual VertexArray operator () (const Triangle &) const = 0;
};

class TriangleToFloorVerticies final : public TriangleToVerticies {
public:
    TriangleToFloorVerticies(const TileTexture & ttex_, Real ytrans):
        m_ttex(ttex_), m_ytrans(ytrans) {}

    VertexArray operator () (const Triangle & triangle) const final;

    template <typename Func>
    static auto make(Func && f) {
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

private:
    TileTexture m_ttex;
    Real m_ytrans;
};

// class is too god like
// well what are the responsiblities this class is handling?
// and how should I seperate them?
// use utility classes perhaps
class WallTileFactoryBase : public SlopesBasedTileFactory {
public:
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
    using Triangle = TriangleSegment;
    using KnownCorners = CornersArray<bool>;
    enum SplitOpt {
        k_bottom_only         = 1 << 0,
        k_top_only            = 1 << 1,
        k_wall_only           = 1 << 2,
        k_both_flats_and_wall = k_bottom_only | k_top_only | k_wall_only
    };

    void operator ()
        (const SlopeGroupNeighborhood &,
         ProducableTileCallbacks &) const final;

    Slopes computed_tile_elevations(const SlopeGroupNeighborhood &) const;

    // should have translations and all
    void make_physical_triangles
        (const SlopeGroupNeighborhood &, ProducableTileCallbacks &) const;

    Slopes tile_elevations() const final;

protected:
    using GraphicMap = std::map<WallTileGraphicKey, WeakPtr<const RenderModel>>;

    static constexpr const Real k_visual_dip_thershold   = -0.25;
    static constexpr const Real k_physical_dip_thershold = -0.5;

    CardinalDirection direction() const noexcept;

    virtual bool is_okay_wall_direction(CardinalDirection) const noexcept = 0;

    virtual KnownCorners make_known_corners() const = 0;

    virtual void make_triangles(const Slopes &, Real thershold, SplitOpt, const TriangleAdder &) const = 0;

private:
    template <typename Func>
    static auto make_triangle_to_verticies(Func && f)
        { return TriangleToFloorVerticies::make(std::move(f)); }

    // v belongs in a base class?
    TileTexture floor_texture() const;

    WallTileGraphicKey graphic_key(const SlopeGroupNeighborhood &) const;

    Real known_elevation() const;

    SharedPtr<const RenderModel> ensure_bottom_model
        (const SlopeGroupNeighborhood & neighborhood,
         ProducableTileCallbacks & callbacks) const;

    template <typename MakerFunc>
    SharedPtr<const RenderModel> ensure_model
        (const SlopeGroupNeighborhood & neighborhood, GraphicMap & graphic_map,
         MakerFunc && make_model) const;

    SharedPtr<const RenderModel>
        ensure_wall_graphics
        (const SlopeGroupNeighborhood & neighborhood,
         ProducableTileCallbacks &) const;

    SharedPtr<const RenderModel>
        make_bottom_graphics
        (const SlopeGroupNeighborhood & neighborhood,
         ProducableTileCallbacks & callbacks) const;

    std::array<Tuple<bool, CardinalDirection>, 4>
        make_known_corners_with_preposition() const;

    SharedPtr<const RenderModel>
        make_model_graphics
        (const Slopes & elevations, SplitOpt,
         const TriangleToVerticies &,
         ProducableTileCallbacks &) const;

    SharedPtr<const RenderModel>
        make_model_graphics
        (const Slopes & elevations, SplitOpt,
         const TriangleToVerticies &,
         SharedPtr<RenderModel> && model_to_use) const;

    SharedPtr<const RenderModel> make_top_model(Platform &) const;

    auto make_triangle_to_floor_verticies() const
        { return TriangleToFloorVerticies{floor_texture(), -translation().y}; }

    SharedPtr<const RenderModel>
        make_wall_graphics
        (const SlopeGroupNeighborhood & neighborhood,
         ProducableTileCallbacks &) const;

    void setup_(const TileProperties &, Platform &,
                const SlopeFillerExtra &,
                const Vector2I & location_on_tileset) final;

    CardinalDirection verify_okay_wall_direction(CardinalDirection dir) const {
        if (!is_okay_wall_direction(dir)) {
            throw std::invalid_argument{"bad wall"};
        }
        return dir;
    }

    TileTexture wall_texture() const;

    CardinalDirection m_dir = CardinalDirection::ne;
    Vector2I m_tileset_location;
    TileTexture m_wall_texture_coords = s_default_texture;
    SharedPtr<const RenderModel> m_top_model;

    static GraphicMap s_wall_graphics_cache;
    static GraphicMap s_bottom_graphics_cache;
    static const TileTexture s_default_texture;
};
