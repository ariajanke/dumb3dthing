/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "SlopesTilesetTile.hpp"

#include "../../TriangleSegment.hpp"

struct StripVertex final {
    enum class StripSide { a, b, both };

    StripVertex() {}

    StripVertex
        (const Vector & pt,
         Optional<Real> position_,
         StripSide side_):
        point(pt),
        strip_position(position_),
        strip_side(side_) {}

    Vector point;
    // [0 1]
    // absent -> indicates the one and only point on that side
    Optional<Real> strip_position;
    // but strip side can also be a Real?
    // StripSide strip_side = StripSide::both;

    // [0 1]
    // 0 -> a side
    // 1 -> b side
    // absent -> "neither" or "both"
    StripSide strip_side;
};

// ----------------------------------------------------------------------------

class StripTriangle final {
public:
    StripTriangle() {}

    StripTriangle
        (const StripVertex & a,
         const StripVertex & b,
         const StripVertex & c);

    TriangleSegment to_triangle_segment() const;

    StripVertex vertex_a() const;

    StripVertex vertex_b() const;

    StripVertex vertex_c() const;

    template <Vector(*kt_transform_vector_f)(const Vector &)>
    StripTriangle transform_points() const {
        auto new_vertex = [](const StripVertex & vtx) {
            return StripVertex
                {kt_transform_vector_f(vtx.point),
                 vtx.strip_position,
                 vtx.strip_side};
        };
        return StripTriangle{new_vertex(m_a), new_vertex(m_b), new_vertex(m_c)};
    }

private:
    StripVertex m_a, m_b, m_c;
};

// ----------------------------------------------------------------------------

class LinearStripTriangleCollection {
public:
    using ToPlanePositionFunction = Vector2(*)(const Vector &);

    virtual ~LinearStripTriangleCollection() {}

    void make_strip
        (const Vector & a_start, const Vector & a_last,
         const Vector & b_start, const Vector & b_last,
         int steps_count);

    virtual void add_triangle
        (const TriangleSegment &, ToPlanePositionFunction) = 0;

    virtual void add_triangle(const StripTriangle &) = 0;

private:
    void triangle_strip
        (const Vector & a_point,
         const Vector & b_start,
         const Vector & b_last,
         StripVertex::StripSide a_side,
         int steps_count);
};

// ----------------------------------------------------------------------------

template <Vector(*kt_transform_vector_f)(const Vector &)>
class TransformedTriangleStrip final : public LinearStripTriangleCollection {
public:
    explicit TransformedTriangleStrip
        (LinearStripTriangleCollection & original):
        m_original(original) {}

    void add_triangle(const StripTriangle & triangle) final {
        m_original.add_triangle
            (triangle.transform_points<kt_transform_vector_f>());
    }

    void add_triangle
        (const TriangleSegment & triangle, ToPlanePositionFunction f) final
    {
        TriangleSegment transformed_triangle
            {kt_transform_vector_f(triangle.point_a()),
             kt_transform_vector_f(triangle.point_b()),
             kt_transform_vector_f(triangle.point_c())};
        m_original.add_triangle(transformed_triangle, f);
    }

private:
    LinearStripTriangleCollection & m_original;
};

// ----------------------------------------------------------------------------

class SplitWallGeometry {
public:
    struct WithSplitWallGeometry {
        virtual ~WithSplitWallGeometry() {}

        virtual void operator () (const SplitWallGeometry &) const = 0;
    };

    class GeometryGenerationStrategy {
    public:
        virtual ~GeometryGenerationStrategy() {}

        virtual void with_splitter_do
            (const TileCornerElevations &,
             Real division_z,
             const WithSplitWallGeometry &) const = 0;

        virtual TileCornerElevations filter_to_known_corners
            (TileCornerElevations) const = 0;
    };

    using GeometryGenerationStrategySource =
        GeometryGenerationStrategy &(*)(CardinalDirection);

    // are any static methods unused?

    static Vector invert_z(const Vector & r) { return Vector{r.x, r.y, -r.z}; }

    static Vector invert_x(const Vector & r) { return Vector{-r.x, r.y, r.z}; }

    static Vector invert_xz(const Vector & r) { return Vector{-r.x, r.y, -r.z}; }

    static Vector xz_swap_roles(const Vector & r)
        { return Vector{r.z, r.y, r.x}; };

    static Vector invert_x_swap_xz(const Vector & r)
        { return invert_x(xz_swap_roles(r)); }

    static Vector2 cut_y(const Vector & r)
        { return Vector2{r.x + 0.5, r.z + 0.5}; }

    static GeometryGenerationStrategy & null_generation_strategy
        (CardinalDirection);

    virtual ~SplitWallGeometry() {}

    virtual void make_top(LinearStripTriangleCollection &) const = 0;

    virtual void make_bottom(LinearStripTriangleCollection &) const = 0;

    virtual void make_wall(LinearStripTriangleCollection &) const = 0;
};
// ----------------------------------------------------------------------------

template <Vector(*kt_transform_vector_f)(const Vector &)>
class TransformedSplitWallGeometry : public SplitWallGeometry {
public:
    void make_top(LinearStripTriangleCollection & col) const final {
        TransformedTriangleStripType impl{col};
        original_split().make_top(impl);
    }

    void make_bottom(LinearStripTriangleCollection & col) const final {
        TransformedTriangleStripType impl{col};
        original_split().make_bottom(impl);
    }

    void make_wall(LinearStripTriangleCollection & col) const final {
        TransformedTriangleStripType impl{col};
        original_split().make_wall(impl);
    }

protected:
    virtual const SplitWallGeometry & original_split() const = 0;

private:
    using TransformedTriangleStripType =
        TransformedTriangleStrip<kt_transform_vector_f>;
};
