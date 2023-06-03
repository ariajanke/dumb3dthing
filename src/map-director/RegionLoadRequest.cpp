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

#include "RegionLoadRequest.hpp"

#include "MapDirector.hpp"

RectanglePoints::RectanglePoints(const cul::Rectangle<Real> & rect):
    m_top_left    (cul::top_left_of    (rect)),
    m_top_right   (cul::top_right_of   (rect)),
    m_bottom_left (cul::bottom_left_of (rect)),
    m_bottom_right(cul::bottom_right_of(rect)) {}

// ----------------------------------------------------------------------------

RegionLoadRequest::RegionLoadRequest
    (const Vector2 & triangle_a, const Vector2 & triangle_b,
     const Vector2 & triangle_c, Size2I max_region_size):
    m_triangle_bounds(bounds_for(triangle_a, triangle_b, triangle_c)),
    m_pt_a(triangle_a),
    m_pt_b(triangle_b),
    m_pt_c(triangle_c),
    m_max_size(max_region_size)
{}

/* static */ RegionLoadRequest RegionLoadRequest::find
    (const Vector & player_position, const Optional<Vector> & player_facing,
     const Vector & player_velocity, Size2I max_region_size)
{
    auto triangle = find_triangle(player_position, player_facing, player_velocity);
    return RegionLoadRequest
        {to_global_tile_position(triangle.point_a()),
         to_global_tile_position(triangle.point_b()),
         to_global_tile_position(triangle.point_c()),
         max_region_size};
}

/* static */ TriangleSegment RegionLoadRequest::find_triangle
    (const Vector & player_position, const Optional<Vector> & player_facing,
     const Vector & player_velocity)
{
    static constexpr Real k_max_speed   = 8;
    static constexpr Real k_low_offset  = 4;
    static constexpr Real k_high_offset = 1;
    static_assert(k_low_offset > k_high_offset);
    static constexpr Real k_out_point_offset_low  = k_low_offset + 6;
    static constexpr Real k_out_point_offset_high = k_low_offset + 10;

    // angles are controlled by a fixed area
    // the resulting triangle will be an isosceles
    static auto interpol = [] (Real t, Real low, Real high)
        { return t*high + (t - 1)*low; };

    Real normalized_speed =
        std::min(magnitude(player_velocity), k_max_speed) / k_max_speed;
    auto a = player_position -
             interpol(normalized_speed, k_low_offset, k_high_offset)*
             player_facing.value_or(Vector{1, 0, 0});
    auto out_point_offset =
        interpol(normalized_speed, k_out_point_offset_low, k_out_point_offset_high);
    auto to_out_point = normalize(player_position - a)*out_point_offset;
    auto out_point = a + to_out_point;
    auto out_point_offset_to_bc = k_triangle_area / out_point_offset;
    auto to_bc_dir = normalize(cross(k_plane_normal, to_out_point));
    auto b = out_point + out_point_offset_to_bc*to_bc_dir;
    auto c = out_point - out_point_offset_to_bc*to_bc_dir;
    return TriangleSegment{a, b, c};
}

/* static */ bool RegionLoadRequest::are_intersecting_lines
    (const Vector2 & a_first, const Vector2 & a_second,
     const Vector2 & b_first, const Vector2 & b_second)
{
    auto p = a_first;
    auto r = a_second - p;

    auto q = b_first;
    auto s = b_second - q;

    auto r_cross_s = cross(r, s);
    if (r_cross_s == 0) return false;

    auto q_sub_p = q - p;
    auto t_num = cross(q_sub_p, s);


    auto outside_0_1 = [](const Real & num, const Real & denom)
        { return num*denom < 0 || magnitude(num) > magnitude(denom); };

    // overflow concerns?
    if (outside_0_1(t_num, r_cross_s)) return false;

    if (outside_0_1(cross(q_sub_p, r), r_cross_s)) return false;

    return true;
}

bool RegionLoadRequest::overlaps_with(const RectangleI & tile_rectangle) const {
    // between reals and integers...
    cul::Rectangle<Real> tile_bounds
        {Real(tile_rectangle.left), Real(tile_rectangle.top),
         Real(tile_rectangle.width + 1), Real(tile_rectangle.height + 1)};
    RectanglePoints tile_bounds_pts{tile_bounds};
    // my other solution maybe much more complicated... :c
    // it doesn't mean it can't be attempted
    // 2023-6-3 1402 in notebook
    return has_any_intersecting_lines_with(tile_bounds_pts) ||
           contains_any_points_of(tile_bounds_pts) ||
           any_point_is_contained_in(tile_bounds);
}

bool RegionLoadRequest::has_any_intersecting_lines_with
    (const RectanglePoints & rect) const
{
    auto triangle_lines = {
        make_tuple(m_pt_a, m_pt_b),
        make_tuple(m_pt_b, m_pt_c),
        make_tuple(m_pt_b, m_pt_c)
    };
    auto rectangle_lines = {
        make_tuple(rect.top_left    (), rect.top_right   ()),
        make_tuple(rect.top_right   (), rect.bottom_right()),
        make_tuple(rect.bottom_right(), rect.bottom_left ()),
        make_tuple(rect.bottom_left (), rect.top_left    ())
    };
    for (auto & triangle_line : triangle_lines) {
        for (auto & rectangle_line : rectangle_lines) {
            using std::get;
            // 12 intersection checks
            bool lines_intersect = are_intersecting_lines
                (get<0>(triangle_line ), get<1>(triangle_line ),
                 get<0>(rectangle_line), get<1>(rectangle_line));
            if (lines_intersect) return true;
        }
    }
    return false;
}

bool RegionLoadRequest::contains_any_points_of
    (const RectanglePoints & rect) const
{
    return contains_point(rect.top_left    ()) &&
           contains_point(rect.top_right   ()) &&
           contains_point(rect.bottom_left ()) &&
           contains_point(rect.bottom_right());
}

bool RegionLoadRequest::contains_point(const Vector2 & r) const {
    return cul::is_inside_triangle(m_pt_a, m_pt_b, m_pt_c, r);
}

bool RegionLoadRequest::any_point_is_contained_in
    (const cul::Rectangle<Real> & rect) const
{
    auto rect_contains = [rect] (const Vector2 & r)
        { return cul::is_contained_in(r, rect); };
    return rect_contains(m_pt_a) &&
           rect_contains(m_pt_b) &&
           rect_contains(m_pt_c);
}

/* private static */ cul::Rectangle<Real> RegionLoadRequest::bounds_for
    (const Vector2 & triangle_a, const Vector2 & triangle_b,
     const Vector2 & triangle_c)
{
    Vector2 low { k_inf,  k_inf};
    Vector2 high{-k_inf, -k_inf};
    for (auto pt : { triangle_a, triangle_b, triangle_c }) {
        using std::min, std::max;
        low .x = min(low.x , pt.x);
        low .y = min(low.y , pt.y);
        high.x = max(high.x, pt.x);
        high.y = max(high.y, pt.y);
    }
    return cul::Rectangle<Real>
        {low.x, low.y, high.x - low.x, high.y - low.y};
}
