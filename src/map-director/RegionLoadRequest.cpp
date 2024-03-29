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

#include "../Components.hpp"
#include "../TriangleSegment.hpp"

#include "../point-and-plane.hpp"
#include "../geometric-utilities.hpp"

namespace {

constexpr const auto k_bad_facing_msg =
    "RegionLoadRequest::find_triangle: player_facing must be either empty of a "
    "normal vector";
constexpr Real k_triangle_area = RegionLoadRequest::k_triangle_area;
constexpr auto k_plane_normal  = RegionLoadRequest::k_plane_normal;
constexpr Vector k_default_facing = Vector{1, 0, 0};

TriangleSegment find_triangle_with_adjusted
    (const Vector & position, const Vector & facing, Real speed);

} // end of <anonymous> namespace

RectanglePoints::RectanglePoints(const cul::Rectangle<Real> & rect):
    m_top_left    (cul::top_left_of    (rect)),
    m_top_right   (cul::top_right_of   (rect)),
    m_bottom_left (cul::bottom_left_of (rect)),
    m_bottom_right(cul::bottom_right_of(rect)) {}

// ----------------------------------------------------------------------------

RegionLoadRequest::RegionLoadRequest
    (const Vector2 & triangle_a,
     const Vector2 & triangle_b,
     const Vector2 & triangle_c,
     Size2I max_region_size):
    RegionLoadRequest
        (bounds_for(triangle_a, triangle_b, triangle_c),
         triangle_a,
         triangle_b,
         triangle_c,
         max_region_size) {}

/* static */ RegionLoadRequest RegionLoadRequest::find
    (const Entity & physical_ent)
{ return find(find_triangle(physical_ent)); }

/* static */ RegionLoadRequest RegionLoadRequest::find
    (const Vector & player_position,
     const Optional<Vector> & player_facing,
     const Vector & player_velocity,
     Size2I max_region_size)
{
    return find
        (find_triangle(player_position, player_facing, player_velocity),
         max_region_size);
}

/* static */ RegionLoadRequest RegionLoadRequest::find
    (const TriangleSegment & positional_triangle,
     Size2I max_region_size)
{
    auto to_v2 = [](const Vector & r) { return Vector2{r.x, r.z}; };
    return RegionLoadRequest
        {to_v2(positional_triangle.point_a()),
         to_v2(positional_triangle.point_b()),
         to_v2(positional_triangle.point_c()),
         max_region_size};
}

/* static */ TriangleSegment RegionLoadRequest::find_triangle
    (const Entity & physical_ent)
{
    auto facing = [&physical_ent] () -> Optional<Vector> {
        auto & camera = physical_ent.get<Camera>();
        if (are_very_close(camera.target, camera.position))
            return {};
        return normalize(camera.target - camera.position);
    } ();
    auto player_position = point_and_plane::location_of(physical_ent.get<PpState>());
    auto player_velocity = physical_ent.get<Velocity>().value;
    return find_triangle(player_position, facing, player_velocity);
}

/* static */ TriangleSegment RegionLoadRequest::find_triangle
    (const Vector & player_position,
     const Optional<Vector> & player_facing,
     const Vector & player_velocity)
{
    // check parameters
    if (player_facing) {
        if (!are_very_close(magnitude(*player_facing), 1))
            { throw InvalidArgument{k_bad_facing_msg}; }
    }

    // adjust parameters
    auto facing = player_facing.value_or(k_default_facing);
    auto position = project_onto_plane(player_position, k_plane_normal);
    auto speed = magnitude(project_onto_plane(player_velocity, k_plane_normal));

    // angles are controlled by a fixed area
    // the resulting triangle will be an isosceles
    return find_triangle_with_adjusted(position, facing, speed);
}

/* static */ cul::Rectangle<Real> RegionLoadRequest::to_on_field_rectangle
    (const RectangleI & tile_rectangle)
{
    // grid position 0, 0 -> -0.5, y,  0.5
    //               1, 1 ->  0.5, y, -0.5
    //               2, 2 ->  1.5, y, -1.5
    // this "flip" doesn't seem right either
    // there should be a central place for going back and forth
    // from grid positions to in game positions
    auto flip = [] (const Vector2I & r) { return Vector2{r.x, -r.y}; };
    auto bottom_left =
        flip(cul::top_left_of(tile_rectangle)) +
        Vector2{k_tile_top_left.x, k_tile_top_left.z};
    auto top_left = bottom_left - Vector2{0, tile_rectangle.height};
    auto size = cul::convert_to<cul::Size2<Real>>(cul::size_of(tile_rectangle));
    return cul::Rectangle<Real>{top_left, size};
}

bool RegionLoadRequest::overlaps_with
    (const RectangleI & tile_rectangle) const
{
    auto field_rectangle = to_on_field_rectangle(tile_rectangle);
    return overlaps_with_field_rectangle(field_rectangle);
}

bool RegionLoadRequest::overlaps_with_field_rectangle
    (const cul::Rectangle<Real> & field_rectangle) const
{
    RectanglePoints tile_bounds_pts{field_rectangle};
    // my other solution maybe much more complicated... :c
    // it doesn't mean it can't be attempted
    // 2023-6-3 1402 in notebook
    return has_any_intersecting_lines_with(tile_bounds_pts) ||
           contains_any_points_of(tile_bounds_pts) ||
           any_point_is_contained_in(field_rectangle);
}

bool RegionLoadRequest::has_any_intersecting_lines_with
    (const RectanglePoints & rect) const
{
    auto triangle_lines = {
        make_tuple(m_pt_a, m_pt_b),
        make_tuple(m_pt_b, m_pt_c),
        make_tuple(m_pt_c, m_pt_a)
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
            bool lines_intersect = ::find_intersection
                (get<0>(triangle_line ), get<1>(triangle_line ),
                 get<0>(rectangle_line), get<1>(rectangle_line)).
                has_value();

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
    (const Vector2 & triangle_a,
     const Vector2 & triangle_b,
     const Vector2 & triangle_c)
{
    Vector2 low { k_inf,  k_inf};
    Vector2 high{-k_inf, -k_inf};
    for (auto pt : std::array { triangle_a, triangle_b, triangle_c }) {
        using std::min, std::max;
        low .x = min(low.x , pt.x);
        low .y = min(low.y , pt.y);
        high.x = max(high.x, pt.x);
        high.y = max(high.y, pt.y);
    }
    return cul::Rectangle<Real>
        {low.x, low.y, high.x - low.x, high.y - low.y};
}

/* private */ RegionLoadRequest::RegionLoadRequest
    (const cul::Rectangle<Real> & triangle_bounds,
     const Vector2 & triangle_a,
     const Vector2 & triangle_b,
     const Vector2 & triangle_c,
     Size2I max_region_size):
    m_triangle_bounds(triangle_bounds),
    m_pt_a(triangle_a),
    m_pt_b(triangle_b),
    m_pt_c(triangle_c),
    m_max_size(max_region_size) {}

// ----------------------------------------------------------------------------

namespace {

Real interpolate(Real t, Real low, Real high)
    { return t*high + (1 - t)*low; };

TriangleSegment find_triangle_with_adjusted
    (const Vector & position, const Vector & facing, Real speed)
{
    static constexpr Real k_max_speed   = 8;
    static constexpr Real k_low_offset  = 4.5;
    static constexpr Real k_high_offset = 1.5;
    static_assert(k_low_offset > k_high_offset);
    static constexpr Real k_out_point_offset_low  = k_low_offset + 8;
    static constexpr Real k_out_point_offset_high = k_low_offset + 12;

    Real normalized_speed = std::min(speed, k_max_speed) / k_max_speed;
    auto a = position -
             interpolate(normalized_speed, k_low_offset, k_high_offset)*
             facing;
    auto out_point_offset =
        interpolate(normalized_speed, k_out_point_offset_low, k_out_point_offset_high);
    auto to_out_point = normalize(position - a)*out_point_offset;
    auto out_point = a + to_out_point;
    auto out_point_offset_to_bc = k_triangle_area / out_point_offset;
    auto to_bc_dir = normalize(cross(k_plane_normal, to_out_point));
    auto b = out_point + out_point_offset_to_bc*to_bc_dir;
    auto c = out_point - out_point_offset_to_bc*to_bc_dir;
    return TriangleSegment{a, b, c};
}

} // end of <anonymous> namespace
