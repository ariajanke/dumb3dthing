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

#include "MapRegionTracker.hpp"

#include "../map-director.hpp"

TriangleSegment find_region_load_request
    (const Vector & player_position, const Vector & player_facing,
     const Vector & player_velocity);
#if 0
namespace rectangle_x_triangle {

class SegmentOverlap final {
public:
    SegmentOverlap(Vector2 top_tri_a, Vector2 top_tri)
private:
    Vector2 m_top_tri_a, m_top_tri_b;
    Vector2 m_bottom_tri_a, m_bottom_tri_b;
    // choice is arbitrary, it maybe either axis
    Real m_top_rect = 0., m_bottom_rect = 0.;
};

}
#endif
#if 0
class RectanglePoints final {
public:
    explicit RectanglePoints(const cul::Rectangle<Real> & rect):
        m_top_left    (cul::top_left_of    (rect)),
        m_top_right   (cul::top_right_of   (rect)),
        m_bottom_left (cul::bottom_left_of (rect)),
        m_bottom_right(cul::bottom_right_of(rect)) {}

    const Vector2 & top_left() const { return m_top_left; }

    const Vector2 & top_right() const { return m_top_right; }

    const Vector2 & bottom_left() const { return m_bottom_left; }

    const Vector2 & bottom_right() const { return m_bottom_right; }

private:
    Vector2 m_top_left, m_top_right, m_bottom_left, m_bottom_right;
};

class RegionLoadRequest final {
public:
    static RegionLoadRequest find
        (const Vector & player_position, const Optional<Vector> & player_facing,
         const Vector & player_velocity, Size2I max_region_size);

    static TriangleSegment find_triangle
        (const Vector & player_position, const Optional<Vector> & player_facing,
         const Vector & player_velocity);

    // cul issue 5
    static bool are_intersecting_lines
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

    static constexpr Real k_triangle_area = 32*32;
    static constexpr auto k_plane_normal  = k_up;

    RegionLoadRequest(const Vector2 & triangle_a, const Vector2 & triangle_b,
                      const Vector2 & triangle_c, Size2I max_region_size);

    bool overlaps_with(const RectangleI & tile_rectangle) const {
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

    bool has_any_intersecting_lines_with(const RectanglePoints & rect) const {
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

    bool contains_any_points_of(const RectanglePoints & rect) const {
        return contains_point(rect.top_left    ()) &&
               contains_point(rect.top_right   ()) &&
               contains_point(rect.bottom_left ()) &&
               contains_point(rect.bottom_right());
    }

    bool contains_point(const Vector2 & r) const {
        return cul::is_inside_triangle(m_pt_a, m_pt_b, m_pt_c, r);
    }

    bool any_point_is_contained_in(const cul::Rectangle<Real> & rect) const {
        auto rect_contains = [rect] (const Vector2 & r)
            { return cul::is_contained_in(r, rect); };
        return rect_contains(m_pt_a) &&
               rect_contains(m_pt_b) &&
               rect_contains(m_pt_c);
    }

    Size2I max_region_size() const { return m_max_size; }

private:
    static cul::Rectangle<Real> bounds_for
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

    cul::Rectangle<Real> m_triangle_bounds;
    Vector2 m_pt_a;
    Vector2 m_pt_b;
    Vector2 m_pt_c;
    Size2I m_max_size;
};
#endif
/** @brief The MapDirector turns player physics things into map region
 *         loading/unloading.
 *
 *  Map segments are loaded depending on the player's state.
 */
class MapDirector final : public MapDirector_ {
public:
    using PpDriver = point_and_plane::Driver;

    explicit MapDirector(PpDriver * ppdriver):
        m_ppdriver(ppdriver) {}

    SharedPtr<BackgroundTask> begin_initial_map_loading
        (const char * initial_map, Platform & platform,
         const Entity & player_physics) final;

    void on_every_frame
        (TaskCallbacks & callbacks, const Entity & physics_ent) final;

private:
    static Vector2I to_region_location
        (const Vector & location, const Size2I & segment_size);

    void check_for_other_map_segments
        (TaskCallbacks & callbacks, const Entity & physics_ent);

    // there's only one per game and it never changes
    PpDriver * m_ppdriver = nullptr;
    SharedPtr<MapRegionTracker> m_region_tracker;
};

Vector2 to_global_tile_position(const Vector &);
