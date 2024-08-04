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

#include "ParseHelpers.hpp"
#include "ViewGrid.hpp"
#include "RegionAxisAddressAndSide.hpp"

#include "../Definitions.hpp"
#include "../Components.hpp"

class ScaleComputation final {
public:
    static Optional<ScaleComputation> parse(const char *);

    static ScaleComputation tile_scale_from_map(const TiXmlElement & map_root);

    static ScaleComputation pixel_scale_from_map(const TiXmlElement & map_root);

    ScaleComputation(): m_factor(1, 1, 1) {}

    ScaleComputation(Real eastwest_factor,
                     Real updown_factor,
                     Real northsouth_factor);

    TriangleSegment of(const TriangleSegment &) const;

    Vector of(const Vector & r) const
        { return scale(r); }

    Vector2I of(const Vector2I & r) const
        { return Vector2I{scale_x(r.x), scale_z(r.y)}; }

    Size2I of(const Size2I & size) const
        { return Size2I{scale_x(size.width), scale_z(size.height)}; }

    RectangleI of(const RectangleI &) const;

    ScaleComputation of(const ScaleComputation &) const;

    bool operator == (const ScaleComputation &) const;

    ModelScale to_model_scale() const;

private:
    static constexpr Vector k_no_scaling{1, 1, 1};

    static int scale_int(Real x, int n) { return int(std::round(x*n)); };

    int scale_x(int n) const { return scale_int(m_factor.x, n); }

    int scale_z(int n) const { return scale_int(m_factor.z, n); }

    Vector scale(const Vector &) const;

    Vector m_factor = k_no_scaling;
};

class ScaledTriangleViewGrid final {
public:
    using ViewGridTriangle = ViewGrid<SharedPtr<TriangleLink>>;

    ScaledTriangleViewGrid() {}

    ScaledTriangleViewGrid
        (const SharedPtr<ViewGridTriangle> & triangle_grid,
         const ScaleComputation & scale):
        m_triangle_grid(triangle_grid),
        m_scale(scale) {}

    std::array<RegionAxisAddressAndSide, 4>
        sides_and_addresses_at(const Vector2I & on_field_position) const;

    template <typename Func>
    void for_each_link_on_side(RegionSide side, Func && f) const;

    auto all_links() const { return m_triangle_grid->elements(); }

private:
    SharedPtr<ViewGridTriangle> m_triangle_grid;
    ScaleComputation m_scale;
};

template <typename Func>
    void for_each_tile_on_edge
    (const RectangleI & bounds, RegionSide side, Func && f);

// ----------------------------------------------------------------------------

template <typename Func>
void ScaledTriangleViewGrid::for_each_link_on_side
    (RegionSide side, Func && f) const
{
    for_each_tile_on_edge
        (RectangleI{Vector2I{}, m_triangle_grid->size2()}, side,
         [f = std::move(f), this] (int x, int y) {
            for (const auto & link : (*m_triangle_grid)(x, y))
                { f(link); };
         });
}

// ----------------------------------------------------------------------------

template <typename Func>
void for_each_tile_on_edge
    (const RectangleI & bounds, RegionSide side, Func && f)
{
    using Side = RegionSide;
    using cul::right_of, cul::bottom_of;

    auto for_each_horz_ = [bounds, &f] (int y_pos) {
        for (int x = bounds.left; x != right_of(bounds); ++x)
            { f(x, y_pos); }
    };

    auto for_each_vert_ = [bounds, &f] (int x_pos) {
        for (int y = bounds.top; y != bottom_of(bounds); ++y)
            { f(x_pos, y); }
    };

    switch (side) {
    case Side::left  : return for_each_vert_(bounds.left          );
    case Side::right : return for_each_vert_(right_of (bounds) - 1);
    case Side::bottom: return for_each_horz_(bottom_of(bounds) - 1);
    case Side::top   : return for_each_horz_(bounds.top           );
    default: return;
    }
}
