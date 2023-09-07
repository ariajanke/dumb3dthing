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

#include "../Definitions.hpp"

#include <ariajanke/cul/RectangleUtils.hpp>

class ScaleComputation;

class RectanglePoints final {
public:
    explicit RectanglePoints(const cul::Rectangle<Real> & rect);

    const Vector2 & top_left() const { return m_top_left; }

    const Vector2 & top_right() const { return m_top_right; }

    const Vector2 & bottom_left() const { return m_bottom_left; }

    const Vector2 & bottom_right() const { return m_bottom_right; }

private:
    Vector2 m_top_left, m_top_right, m_bottom_left, m_bottom_right;
};

class RegionLoadRequest final {
public:
    static constexpr Size2I k_default_max_region_size = Size2I{2, 2}; //Size2I{10, 10};
    static constexpr Real k_triangle_area = 0.5*16*10;
    static constexpr auto k_plane_normal  = k_up;

    static RegionLoadRequest find
        (const Vector & player_position,
         const Optional<Vector> & player_facing,
         const Vector & player_velocity,
         Size2I max_region_size = k_default_max_region_size);

    static TriangleSegment find_triangle
        (const Vector & player_position, const Optional<Vector> & player_facing,
         const Vector & player_velocity);

    static cul::Rectangle<Real> to_on_field_rectangle
        (const RectangleI & tile_rectangle);

    RegionLoadRequest(const Vector2 & triangle_a,
                      const Vector2 & triangle_b,
                      const Vector2 & triangle_c,
                      Size2I max_region_size = k_default_max_region_size);

    bool overlaps_with(const RectangleI & tile_rectangle) const;

    bool overlaps_with_field_rectangle
        (const cul::Rectangle<Real> & field_rectangle) const;

    // data clumpy, but how can I cleanly separate it out?
    Size2I max_region_size() const { return m_max_size; }

    // not sure, maybe scale something else? :c
    // RegionLoadRequest down_scale(const ScaleComputation &) const;

private:
    static cul::Rectangle<Real> bounds_for
        (const Vector2 & triangle_a, const Vector2 & triangle_b,
         const Vector2 & triangle_c);

    RegionLoadRequest(const cul::Rectangle<Real> & triangle_bounds,
                      const Vector2 & triangle_a,
                      const Vector2 & triangle_b,
                      const Vector2 & triangle_c,
                      Size2I max_region_size);

    bool has_any_intersecting_lines_with(const RectanglePoints &) const;

    bool contains_any_points_of(const RectanglePoints &) const;

    bool contains_point(const Vector2 &) const;

    bool any_point_is_contained_in(const cul::Rectangle<Real> &) const;

    cul::Rectangle<Real> m_triangle_bounds;
    Vector2 m_pt_a;
    Vector2 m_pt_b;
    Vector2 m_pt_c;
    Size2I m_max_size;
};
