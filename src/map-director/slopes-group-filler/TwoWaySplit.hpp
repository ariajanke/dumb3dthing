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

#include "../../TriangleSegment.hpp"

// semantically different from a model vertex
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

    // something returning individual vertices
private:
    StripVertex m_a, m_b, m_c;
};

// ----------------------------------------------------------------------------

class LinearStripTriangleCollection {
public:
    using ToPlanePositionFunction = Vector2(*)(const Vector &);

    virtual ~LinearStripTriangleCollection() {}

    // may, or may not be rectangluar, it may end on a point
    void make_strip
        (const Vector & a_start, const Vector & a_last,
         const Vector & b_start, const Vector & b_last,
         int steps_count);
#   if 1 // what if regular triangles had to be supported as well?
         // it depends on how I want to handle texture coord mapping
         // maybe each override says something about texture mapping
         // but not all that much

    virtual void add_triangle
        (const TriangleSegment &, ToPlanePositionFunction) = 0;

#   endif
    virtual void add_triangle(const StripTriangle &) = 0;
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

// needs renaming
// something along the lines of generating wall tile geometry
// needs to be clear that it's for computing triangles and nothing else
class TwoWaySplit {
public:
    // my take on "virtual constructor"
    // it's really just strategy again, it's useful for when you need an
    // instance once, and then you're done
    // saves you a dynamic allocation
    struct WithTwoWaySplit {
        virtual ~WithTwoWaySplit() {}

        virtual void operator () (const TwoWaySplit &) const = 0;
    };

    class GeometryGenerationStrategy {
    public:
        virtual ~GeometryGenerationStrategy() {}

        virtual void with_splitter_do
            (const TileCornerElevations &,
             Real division_z,
             const WithTwoWaySplit &) const = 0;

        virtual TileCornerElevations filter_to_known_corners
            (TileCornerElevations) const = 0;
    };

    using GeometryGenerationStrategySource =
        GeometryGenerationStrategy &(*)(CardinalDirection);

    static Vector invert_z(const Vector & r) { return Vector{r.x, r.y, -r.z}; }

    static Vector invert_x(const Vector & r) { return Vector{-r.x, r.y, r.z}; }

    static Vector invert_xz(const Vector & r) { return Vector{-r.x, r.y, -r.z}; }

    static Vector xz_swap_roles(const Vector & r)
        { return Vector{r.z, r.y, r.x}; };

    static Vector invert_x_swap_xz(const Vector & r)
        { return invert_x(xz_swap_roles(r)); }

    static Vector2 cut_y(const Vector & r)
        { return Vector2{r.x + 0.5, r.z + 0.5}; }

    static GeometryGenerationStrategy &
        choose_geometry_strategy(CardinalDirection);

    static GeometryGenerationStrategy &
        choose_out_wall_strategy(CardinalDirection);

    virtual ~TwoWaySplit() {}

    virtual void make_top(LinearStripTriangleCollection &) const = 0;

    virtual void make_bottom(LinearStripTriangleCollection &) const = 0;

    virtual void make_wall(LinearStripTriangleCollection &) const = 0;
};

// ----------------------------------------------------------------------------

class NorthSouthSplit final : public TwoWaySplit {
public:
    NorthSouthSplit
        (const TileCornerElevations &,
         Real division_z);

    NorthSouthSplit
        (Optional<Real> north_west_y,
         Optional<Real> north_east_y,
         Real south_west_y,
         Real south_east_y,
         Real division_z);

    void make_top(LinearStripTriangleCollection &) const final;

    void make_bottom(LinearStripTriangleCollection &) const final;

    void make_wall(LinearStripTriangleCollection &) const final;

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

template <Vector(*kt_transform_vector_f)(const Vector &)>
class TransformedTwoWaySplit : public TwoWaySplit {
    // commentary:
    // An intermediate class in an inheritance hierarchy.
    // I think it's worth it. It'll cover the other corner cases as well.
public:
    void make_top(LinearStripTriangleCollection & col) const final {
        TransformedTriangleStripType impl{col};
        original_split().make_top(col);
    }

    void make_bottom(LinearStripTriangleCollection & col) const final {
        TransformedTriangleStripType impl{col};
        original_split().make_bottom(col);
    }

    void make_wall(LinearStripTriangleCollection & col) const final {
        TransformedTriangleStripType impl{col};
        original_split().make_wall(col);
    }

protected:
    virtual const TwoWaySplit & original_split() const = 0;

private:
    using TransformedTriangleStripType =
        TransformedTriangleStrip<kt_transform_vector_f>;
};
#if 0
// ----------------------------------------------------------------------------

template <Vector(*kt_transform_vector_f)(const Vector &)>
class TransformedNorthSouthSplit : public TwoWaySplit {
public:
    // commentary:
    // An intermediate class in an inheritance hierarchy.
    // I think it's worth it, so long as leaf classes define only a constructor
    // and *nothing* else.

    void make_top(LinearStripTriangleCollection &) const final;

    void make_bottom(LinearStripTriangleCollection &) const final;

    void make_wall(LinearStripTriangleCollection &) const final;

protected:
    TransformedNorthSouthSplit(NorthSouthSplit &&);

private:
    // question:
    // how do I name "local" types?
    using TransformedTriangleStripType =
        TransformedTriangleStrip<kt_transform_vector_f>;

    NorthSouthSplit m_ns_split;
};
#endif
// ----------------------------------------------------------------------------

class SouthNorthSplit final :
    public TransformedTwoWaySplit<TwoWaySplit::invert_z>
{
public:
    SouthNorthSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const TwoWaySplit & original_split() const final
        { return m_ns_split; }

    NorthSouthSplit m_ns_split;
};

// ----------------------------------------------------------------------------

class WestEastSplit final :
    public TransformedTwoWaySplit<TwoWaySplit::invert_x_swap_xz>
{
public:
    WestEastSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const TwoWaySplit & original_split() const final
        { return m_ns_split; }

    NorthSouthSplit m_ns_split;
};

// ----------------------------------------------------------------------------

class EastWestSplit final :
    public TransformedTwoWaySplit<TwoWaySplit::xz_swap_roles>
{
public:
    EastWestSplit
        (const TileCornerElevations &,
         Real division_z);

private:
    const TwoWaySplit & original_split() const final
        { return m_ns_split; }

    NorthSouthSplit m_ns_split;
};
#if 0
// ----------------------------------------------------------------------------

template <Vector(*kt_transform_vector_f)(const Vector &)>
void TransformedNorthSouthSplit<kt_transform_vector_f>::make_top
    (LinearStripTriangleCollection & col) const
{
    TransformedTriangleStripType impl{col};
    m_ns_split.make_top(impl);
}

template <Vector(*kt_transform_vector_f)(const Vector &)>
void TransformedNorthSouthSplit<kt_transform_vector_f>::make_bottom
    (LinearStripTriangleCollection & col) const
{
    TransformedTriangleStripType impl{col};
    m_ns_split.make_bottom(impl);
}

template <Vector(*kt_transform_vector_f)(const Vector &)>
void TransformedNorthSouthSplit<kt_transform_vector_f>::make_wall
    (LinearStripTriangleCollection & col) const
{
    TransformedTriangleStripType impl{col};
    m_ns_split.make_wall(impl);
}

template <Vector(*kt_transform_vector_f)(const Vector &)>
/* protected */ TransformedNorthSouthSplit<kt_transform_vector_f>::
    TransformedNorthSouthSplit
    (NorthSouthSplit && ns_split_):
    m_ns_split(std::move(ns_split_)) {}
#endif
