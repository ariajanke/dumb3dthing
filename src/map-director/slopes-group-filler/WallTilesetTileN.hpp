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

#include "SlopesTilesetTileN.hpp"
#include "../../RenderModel.hpp"
#include "../../TriangleSegment.hpp"

class LinearStripTriangleCollection {
public:
    virtual ~LinearStripTriangleCollection() {}

    // point c will be the closet to last
    void make_strip
        (const Vector & a_start, const Vector & a_last,
         const Vector & b_start, const Vector & b_last,
         int steps_count);

    virtual void add_triangle(const TriangleSegment &) = 0;
};

// ----------------------------------------------------------------------------

class TwoWaySplitBase {
public:
    // my "virtual constructor"
    // it's really just strategy again
    struct WithTwoWaySplit {
        virtual ~WithTwoWaySplit() {}

        virtual void operator () (const TwoWaySplitBase &) const = 0;
    };

    using TwoWaySplitStrategy = void(*)
        (CardinalDirection,
         const TileCornerElevations &,
         Real division_z,
         const WithTwoWaySplit &);

    static void choose_on_direction_
        (CardinalDirection,
         const TileCornerElevations &,
         Real division_z,
         const WithTwoWaySplit &);

    virtual ~TwoWaySplitBase() {}

    virtual void make_top(LinearStripTriangleCollection &) const = 0;

    virtual void make_bottom(LinearStripTriangleCollection &) const = 0;

    virtual void make_wall(LinearStripTriangleCollection &) const = 0;
};

// ----------------------------------------------------------------------------

class NorthSouthSplit final : public TwoWaySplitBase {
public:
    NorthSouthSplit
        (const TileCornerElevations &,
         Real division_z);

    NorthSouthSplit
        (Real north_west_y,
         Real north_east_y,
         Real south_west_y,
         Real south_east_y,
         Real division_z);

    void make_top(LinearStripTriangleCollection &) const;

    void make_bottom(LinearStripTriangleCollection &) const;

    void make_wall(LinearStripTriangleCollection &) const;

private:
    void check_non_top_assumptions() const;

    Real south_west_y() const { return m_div_sw.y; }

    Real south_east_y() const { return m_div_se.y; }

    Real north_west_y() const { return m_div_nw.y; }

    Real north_east_y() const { return m_div_ne.y; }

    Vector m_div_nw;
    Vector m_div_sw;
    Vector m_div_ne;
    Vector m_div_se;
};

// ----------------------------------------------------------------------------

template <std::size_t kt_capacity_in_triangles>
class CollidablesCollection final : public LinearStripTriangleCollection {
public:
    void append(const View<const Vertex *> & vertices) {
        auto count = vertices.end() - vertices.begin();
        if (count % 3 != 0) {
            throw RuntimeError{"number of vertices must be divisble by three"};
        }
        for (auto itr = vertices.begin(); itr != vertices.end(); itr += 3) {
            auto & a = *itr;
            auto & b = *(itr + 1);
            auto & c = *(itr + 2);
            verify_capacity_for_another();
            m_elements[m_count++] =
                TriangleSegment{a.position, b.position, c.position};
        }
    }

    void add_triangle(const TriangleSegment & triangle) final {
        verify_capacity_for_another();
        m_elements[m_count++] = triangle;
    }

    View<const TriangleSegment *> collidables() const
        { return View{&m_elements[0], &m_elements[0] + m_count}; }

private:
    void verify_capacity_for_another() const {
        if (m_count + 1 > kt_capacity_in_triangles*3) {
            throw RuntimeError{"capacity exceeded"};
        }
    }
    std::array<TriangleSegment, kt_capacity_in_triangles> m_elements;
    std::size_t m_count = 0;
};

// ----------------------------------------------------------------------------
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
    using TwoWaySplitStrategy = TwoWaySplitBase::TwoWaySplitStrategy;

    void load
        (const MapTilesetTile &,
         const TilesetTileTexture &,
         PlatformAssetsStrategy & platform) final;

    TileCornerElevations corner_elevations() const final;

    void make
        (const NeighborCornerElevations & neighboring_elevations,
         ProducableTileCallbacks & callbacks) const;

    void set_split_strategy(TwoWaySplitStrategy strat_f)
        { m_split_strategy = strat_f; }

private:
    template <typename Func>
    void choose_on_direction
        (const TileCornerElevations & elvs,
         Real division_z,
         Func && f) const;

    SharedPtr<const RenderModel> m_top_model;
    // SharedPtr<const Texture> m_texture_ptr;
    TilesetTileTexture m_tileset_tile_texture;
    TileCornerElevations m_elevations;
    CardinalDirection m_direction;
    TwoWaySplitStrategy m_split_strategy = TwoWaySplitBase::choose_on_direction_;
};

template <typename Func>
/* private */ void WallTilesetTile::choose_on_direction
    (const TileCornerElevations & elvs,
     Real division_z,
     Func && f) const
{
    class Impl final : public TwoWaySplitBase::WithTwoWaySplit {
    public:
        explicit Impl(Func && f_):
            m_f(std::move(f_)) {}

        void operator () (const TwoWaySplitBase & two_way_split) const final
            { m_f(two_way_split); }

    private:
        Func m_f;
    };
    Impl impl{std::move(f)};
    m_split_strategy(m_direction, elvs, division_z, impl);
}
