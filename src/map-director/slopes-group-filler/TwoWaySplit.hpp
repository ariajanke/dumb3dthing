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

template <Vector(*kt_transform_vector_f)(const Vector &)>
class TransformedTriangleStrip final : public LinearStripTriangleCollection {
public:
    explicit TransformedTriangleStrip
        (LinearStripTriangleCollection & original):
        m_original(original) {}

    void add_triangle(const TriangleSegment & triangle) final {
        TriangleSegment transformed
            {kt_transform_vector_f(triangle.point_a()),
             kt_transform_vector_f(triangle.point_b()),
             kt_transform_vector_f(triangle.point_c())};
        m_original.add_triangle(transformed);
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

    using TwoWaySplitStrategy = void(*)
        (CardinalDirection,
         const TileCornerElevations &,
         Real division_z,
         const WithTwoWaySplit &);

    static Vector invert_z(const Vector & r) { return Vector{r.x, r.y, -r.z}; }

    static Vector invert_x(const Vector & r) { return Vector{-r.x, r.y, r.z}; }

    static Vector xz_swap_roles(const Vector & r)
        { return Vector{r.z, r.y, r.x}; };

    static Vector invert_x_swap_xz(const Vector & r)
        { return invert_x(xz_swap_roles(r)); }

    static void choose_on_direction_
        (CardinalDirection,
         const TileCornerElevations &,
         Real division_z,
         const WithTwoWaySplit &);

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

// ----------------------------------------------------------------------------

class SouthNorthSplit final :
    public TransformedNorthSouthSplit<TwoWaySplit::invert_z>
{
public:
    SouthNorthSplit
        (const TileCornerElevations &,
         Real division_z);
};

// ----------------------------------------------------------------------------

class WestEastSplit final :
    public TransformedNorthSouthSplit<TwoWaySplit::invert_x_swap_xz>
{
public:
    WestEastSplit
        (const TileCornerElevations &,
         Real division_z);
};

// ----------------------------------------------------------------------------

class EastWestSplit final :
    public TransformedNorthSouthSplit<TwoWaySplit::xz_swap_roles>
{
public:
    EastWestSplit
        (const TileCornerElevations &,
         Real division_z);
};

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
