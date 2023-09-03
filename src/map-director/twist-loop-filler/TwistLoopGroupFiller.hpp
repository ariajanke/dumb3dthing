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

#include "../ProducableGroupFiller.hpp"
#include "../../RenderModel.hpp"
#include "../ViewGrid.hpp"

#include "twist-geometry-helpers.hpp"

#include <ariajanke/cul/Util.hpp>

// how does tile generation work here?
// only load in geometry relevant to a tile? (yes)

class TileSetXmlGrid;
class TwistTileGroup {
public:
    using Rectangle = cul::Rectangle<Real>;

    virtual ~TwistTileGroup() {}

    virtual void operator ()
        (const Vector2I & position_in_group, const Vector2I & tile_offset,
         ProducableTileCallbacks &) const = 0;

    Vector2I group_start() const { return m_group_start; }

    // stay in integer land for as long as possible
    void load(const RectangleI &);

protected:
    virtual void load_(const RectangleI &) = 0;

private:
    Vector2I m_group_start;
};

inline void TwistTileGroup::load(const RectangleI & rect)
{
    m_group_start = top_left_of(rect);
    load_(rect);
}

class NorthSouthTwistTileGroup final : public TwistTileGroup {
public:
    static constexpr const Real k_breaks_per_segment = 2;

    void operator ()
        (const Vector2I & position_in_group, const Vector2I & tile_offset,
         ProducableTileCallbacks &) const final;

private:
    struct ElementsVerticesPair final {
        std::vector<Vertex> vertices;
        std::vector<unsigned> elements;
    };

    void load_(const RectangleI &) final;

    Grid<ElementsVerticesPair> m_elements_vertices;
    ViewGrid<TriangleSegment> m_collision_triangles;
};

class TwistLoopTile final : public ProducableTile {
public:
    TwistLoopTile
        (const Vector2I & position_in_map,
         const Vector2I position_in_group,
         const SharedPtr<TwistTileGroup> & tile_group):
        m_position_in_map(position_in_map),
        m_position_in_group(position_in_group),
        m_twist_group(tile_group) {}

    void operator () (const Vector2I & maps_offset,
                      ProducableTileCallbacks & callbacks) const final
    {
        (*m_twist_group)
            (m_position_in_group, m_position_in_map + maps_offset, callbacks);
    }

private:
    Vector2I m_position_in_map;
    Vector2I m_position_in_group;
    SharedPtr<const TwistTileGroup> m_twist_group;
};

// "Fillers" are a poor name, it needs to be clear that it's a tileset level
// object
class TwistLoopGroupFiller final : public ProducableGroupFiller {
public:
    static Size2I size_of_map(const std::vector<TileLocation> &);

    static Grid<bool> map_positions_to_grid
        (const std::vector<TileLocation> &);

    void load(const TileSetXmlGrid & xml_grid, Platform & platform);

    ProducableGroupTileLayer operator ()
        (const std::vector<TileLocation> &, ProducableGroupTileLayer &&) const final;

private:
#   if 0
    Grid<SharedPtr<TwistTileGroup>> m_tile_groups;
#   endif
};

class RectanglarGroupOfPred {
public:
    static RectangleI get_rectangular_group_of
        (const Vector2I & start, const RectanglarGroupOfPred &);

    virtual ~RectanglarGroupOfPred() {}

    virtual bool operator () (const Vector2I &) const = 0;
};

template <typename Func>
RectangleI get_rectangular_group_of(const Vector2I & start, Func && f);

template <typename Func>
RectangleI get_rectangular_group_of
    (const Vector2I & start, Func && f)
{
    using RectGroupPred = RectanglarGroupOfPred;
    class Impl final : public RectGroupPred {
    public:
        explicit Impl(Func && f_):
            m_f(std::move(f_)) {}

        bool operator () (const Vector2I & r) const final
            { return m_f(r); }

    private:
        Func m_f;
    };
    return RectGroupPred::get_rectangular_group_of(start, Impl{std::move(f)});
}

