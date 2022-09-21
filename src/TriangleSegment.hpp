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

#include "Defs.hpp"

/** Describes a triangle, with various utilities for use with "point and
 *  plane" physics.
 *
 *  Generally, triangles must be three non-colinear points. All points must not
 *  be nearly equal to each other.
 *
 *  NTS: with floating points, objects hitting the very edges of triangles
 *       become *much* more likely, I need code to handle this situation!
 */
class TriangleSegment final {
public:

    /** Enumerations representing a side of the triangle, and one value for
     *  "inside the triangle".
     */
    enum Side { k_side_ab, k_side_bc, k_side_ca, k_inside };

    /** Represents a point's crossing of any of the triangle's borders. */
    struct SideCrossing final {

        /** Default instance, represent no crossing. */
        SideCrossing() {}

        /** Constructs a side crossing, where a crossing has in fact occured.
         *
         *  @param side_ @see TriangleSegment::SideCrossing::side
         *  @param inside_ @see TriangleSegment::SideCrossing::inside
         *  @param outside_ @see TriangleSegment::SideCrossing::outside
         */
        SideCrossing(Side side_, const Vector2 & inside_,
                     const Vector2 & outside_);

        /** Which side has been crossed, k_inside means no crossing has taken
         *  place.
         */
        Side side = k_inside;

        /** Represents the point inside the triangle closest to the crossing
         *  intersection.
         */
        Vector2 inside;

        /** Represents the point outside the triangle closest to the crossing
         *  intersection.
         */
        Vector2 outside;
    };

    /** @brief Represents how a displacement is limited by an intersection
     *         with a triangle.
     */
    struct LimitIntersection final {

        /** Point of intersection, describe by a point on the triangle's
         *  plane; is not a real vector if no such intersection exists
         */
        Vector2 intersection;

        /** The last 3D position of a displacement before that displacement
         *  hits the triangle; the limit vector is the same as the
         *  displacement's end point, if no intersection has occured
         */
        Vector limit;
    };

    /** The default triangle, have well defined locations for points a, b, and
     *  c. They are (1, 0, 0), (0, 1, 0), (0, 0, 1) respectively.
     */
    TriangleSegment();

    /** Constructs a triangle segment, based on provided points.
     *  @throws std::invalid_argument if any vector is non-real, or if any two
     *          of the three points are too close, or the three given points
     *          are co-linear
     *  @param a
     *  @param b
     *  @param c
     */
    TriangleSegment(const Vector & a, const Vector & b, const Vector & c);

    /** @returns the area of the triangle */
    Real area_of_triangle() const;

    /** This defines a basis vector for the plane on which this TriangleSegment
     *  exists.
     *
     *  @note essentially the "x" component on the triangle's plane
     */
    Vector basis_i() const;

    /** This defines a basis vector for the plane on which this TriangleSegment
     *  exists.
     *
     *  @note essentially the "y" component on the triangle's plane
     */
    Vector basis_j() const;

    /** @brief Check if this triangle can be projected onto a plane
     *  @param n vector, maybe the plane's normal, or simply orthogonal to the
     *         plane
     *  @returns true if this triangle can be projected onto a plane by the
     *           vector n
     */
    bool can_be_projected_onto(const Vector & n) const noexcept;

    /** @returns center of the triangle in three dimensions */
    Vector center() const noexcept;

    /** @returns center of the triangle in two dimensions, on the plane of the
     *           triangle
     */
    Vector2 center_in_2d() const noexcept;

    /** Checks if a side crossing occurs when a point moves from an old
     *  location to a new location.
     *
     *  @param old location on the plane of the triangle
     *  @param new_ location on the plane of the triangle
     *  @returns information on a crossing if it occurs
     *  @see TriangleSegment::SideCrossing
     */
    SideCrossing check_for_side_crossing(const Vector2 & old, const Vector2 & new_) const;

    /** @returns the closest point on the triangle's plane that is also inside
     *           the triangle
     */
    Vector2 closest_contained_point(const Vector &) const;

    /** @returns the closest point on the triangle's plane to the given vector
     *           this is essentially a projection of a point onto the plane
     *           describe by the triangle's points
     *  @throw throws when a vector with any non-real component
     */
    Vector2 closest_point(const Vector &) const;

    /** @return true if the given 2d point is found inside the three points
     *          defining the triangle surface, false otherwise. Always returns
     *          false for the default triangle surface.
     */
    bool contains_point(const Vector2 &) const noexcept;

    /** @returns new instance whose normal vector is anti-parallel to this
     *           triangle's normal
     */
    TriangleSegment flip() const noexcept;

    /** @returns the intersection between the two given points and the
     *           triangle, if there is no point in this given segment, then
     *           k_non_intersection is returned.
     *  @throw throws when a vector with any non-real component
     */
    Vector2 intersection(const Vector & a, const Vector & b) const;

    /** @brief Finds how a displacement is limited by the triangle
     *  @param a first point of the displacement
     *  @param b end point of the displacement
     *  @see TriangleSegment::LimitIntersection
     *  @return a structure that describes how the displacement's been limited
     *          (or not) by the triangle
     */
    LimitIntersection limit_with_intersection
        (const Vector & a, const Vector & b) const;

    /** Creates a new triangle segment, that is offset from this one. Each
     *  point still maintains there relative (to each other) positions.
     *  @param r a real vector offset to "move" the segment
     *  @returns new moved segment, this segment is unaffected
     */
    TriangleSegment move(const Vector & r) const
        { return TriangleSegment{point_a() + r, point_b() + r, point_c() + r}; }

    /** Each triangle segment, has a normal vector. The normal vector can be
     *  used to describe the plane of the triangle.
     *
     *  @return
     */
    Vector normal() const noexcept;

    /** @returns the point opposite to the side, e.g. k_side_ab -> point c */
    Vector opposing_point(Side) const;

    /** @returns point a */
    Vector point_a() const noexcept;

    /** @returns point a in 2D on the plane of the segment
     *  @note the client is discouraged from using implementation knowledge to
     *        replace this with a zero vector (as implementations may change)
     */
    Vector2 point_a_in_2d() const noexcept;

    /** @returns point b */
    Vector point_b() const noexcept;

    /** @returns point b in 2D on the plane of the segment
     *  @note the client is discouraged from using implementation knowledge to
     *        replace this with a "x" value difference between points b and a
     */
    Vector2 point_b_in_2d() const noexcept;

    /** @returns point c */
    Vector point_c() const noexcept;

    /** @returns point c in 2D on the plane of the segment */
    Vector2 point_c_in_2d() const noexcept;

    /** @returns position in greater 3-space while treating the triangle as an
     *           infinite plane.
     *  (detail) x follows basis i, y follows basis j
     *  @param vector describing a position on the plane
     *  @throw throws when a vector with any non-real component
     */
    Vector point_at(const Vector2 &) const;

    /** @brief Makes a new triangle with all points projected onto a plane
     *         described by a vector orthogonal to that plane.
     *
     *  @param n vector, maybe the plane's normal, or simply orthogonal to the
     *         plane
     *  @returns new projected triangle
     */
    TriangleSegment project_onto_plane(const Vector & n) const;

    /** @returns points belonging to a side in the order specified,
     *           e.g. k_side_ab returns point a as the first element, then
     *           point b
     */
    Tuple<Vector, Vector> side_points(Side) const;

    /** @returns points belonging to a side in the order specified
     *           unlike its sister method, this method returns points as they
     *           exist on the triangle's plane
     *  @see TriangleSegment::side_points(TriangleSegment::Side)
     */
    Tuple<Vector2, Vector2> side_points_in_2d(Side) const;

private:
    void check_invarients() const noexcept;

    Tuple<Vector2, Vector2> move_to_last_in_and_first_out
        (const Vector2 & far_inside, const Vector2 & far_outside,
         const Vector2 & hint_li, const Vector2 & hint_fo) const noexcept;

    /** @throws if non-real vector */
    Side point_region(const Vector2 &) const;

    Tuple<Vector, Vector, Vector> project_onto_plane_(const Vector &) const noexcept;

    Vector m_a, m_b, m_c;

    Real m_bx_2d;
    Vector2 m_c_2d;
};

using TriangleSide = TriangleSegment::Side;

/** Converts TriangleSide to a human readable string
 *  @return c-string representation of the side
 */
const char * to_string(TriangleSide);
