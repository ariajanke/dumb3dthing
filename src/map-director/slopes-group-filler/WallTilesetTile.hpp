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

#include "SplitWallGeometry.hpp"

#if 0
class WallGeometryCache final {
public:
    static WallGeometryCache & instance();

    enum class WallType { two_way, in, out, none };

    struct WallAndBottomElement final {
        std::vector<TriangleSegment> collidable_triangles;
        // may need to utilize texture translation to get correct mapping
        SharedPtr<RenderModel> model;
    };

    template <typename Func>
    const WallAndBottomElement & ensure
        (WallType,
         CardinalDirection,
         const TileCornerElevations &,
         Func &&);

private:
    struct Ensurer {
        virtual ~Ensurer() {}

        virtual WallAndBottomElement operator () () const = 0;
    };

    struct Key final {
        static constexpr const auto k_any_direction = CardinalDirection::east;
        WallType type = WallType::none;
        CardinalDirection direction = k_any_direction;
        TileCornerElevations elevations;
    };

    const WallAndBottomElement & ensure(WallType, CardinalDirection, Ensurer &&);

};
#endif
// ----------------------------------------------------------------------------

class WallTilesetTile final : public SlopesTilesetTile {
public:
    using GeometryGenerationStrategySource =
        SplitWallGeometry::GeometryGenerationStrategySource;

    WallTilesetTile() {}

    explicit WallTilesetTile(GeometryGenerationStrategySource);

    void load
        (const MapTilesetTile &,
         const TilesetTileTexture &,
         PlatformAssetsStrategy & platform) final;

    TileCornerElevations corner_elevations() const final;

    void make
        (const NeighborCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const;

private:
    using GeometryGenerationStrategy = SplitWallGeometry::GeometryGenerationStrategy;

    template <typename Func>
    void choose_on_direction
        (const TileCornerElevations & elvs,
         Func && f) const;

    SharedPtr<const RenderModel> m_top_model;
    TilesetTileTexture m_tileset_tile_texture;
    TileCornerElevations m_elevations;
    Vector2I m_wall_texture_location;

    GeometryGenerationStrategySource m_strategy_source =
        SplitWallGeometry::null_generation_strategy;
    GeometryGenerationStrategy * m_startegy = nullptr;
};

// ----------------------------------------------------------------------------

template <typename Func>
/* private */ void WallTilesetTile::choose_on_direction
    (const TileCornerElevations & elvs,
     Func && f) const
{
    class Impl final : public SplitWallGeometry::WithSplitWallGeometry {
    public:
        explicit Impl(Func && f_):
            m_f(std::move(f_)) {}

        void operator () (const SplitWallGeometry & two_way_split) const final
            { m_f(two_way_split); }

    private:
        Func m_f;
    };
    Impl impl{std::move(f)};
    m_startegy->with_splitter_do(elvs, -0.25, impl);
}
