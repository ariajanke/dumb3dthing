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

#include "TriangleSegment.hpp"

#include "geometric-utilities.hpp"

namespace {

#define MACRO_MAKE_BAD_BRANCH_EXCEPTION() BadBranchException(__LINE__, __FILE__)

using cul::exceptions_abbr::InvArg;

using Side = TriangleSide;
using SideCrossing = TriangleSegment::SideCrossing;
using LimitIntersection = TriangleSegment::LimitIntersection;

using cul::find_smallest_diff, cul::make_nonsolution_sentinel,
      cul::make_zero_vector;
using std::min_element;

Real get_component_for_basis(const Vector & pt_on_place, const Vector & basis);

Vector2 find_point_c_in_2d(const Vector & a, const Vector & b, const Vector & c);

Real find_point_b_x_in_2d(const Vector & a, const Vector & b);

inline bool within_01(Real x) { return x >= 0 && x <= 1; }

} // end of <anonymous> namespace

TriangleSegment::SideCrossing::SideCrossing
    (TriangleSide r, const Vector2 & li, const Vector2 & fo):
    side(r), inside(li), outside(fo)
{}

TriangleSegment::TriangleSegment():
    m_a(k_east ),
    m_b(k_up   ),
    m_c(k_north),
    m_bx_2d(find_point_b_x_in_2d(m_a, m_b)),
    m_c_2d(find_point_c_in_2d(m_a, m_b, m_c))
    { check_invarients(); }

TriangleSegment::TriangleSegment
    (const Vector & a, const Vector & b, const Vector & c):
    m_a(a), m_b(b), m_c(c)
{
    if (!is_real(a) || !is_real(b) || !is_real(c)) {
        throw InvArg{"TriangleSegment::TriangleSegment: points a, b, and c must "
                     "have all real components."};
    }
    if (are_very_close(a, b) || are_very_close(b, c) || are_very_close(c, a)) {
        throw InvArg{"TriangleSegment::TriangleSegment: all three points must "
                     "be far enough apart, as to be recognized as a triangle."};
    }
    if (are_parallel(b - a, b - c)) {
        throw InvArg{"TriangleSegment::TriangleSegment: points must not be "
                     "co-linear."};
    }
    // early exceptions should catch bad a, b, c values
    m_bx_2d = find_point_b_x_in_2d(a, b);
    m_c_2d  = find_point_c_in_2d(a, b, c);
    assert(!are_parallel(point_b_in_2d() - point_a_in_2d(),
                         point_b_in_2d() - point_c_in_2d()));

    check_invarients();
}

Real TriangleSegment::area() const
    { return cul::area_of_triangle(point_a(), point_b(), point_c()); }

Vector TriangleSegment::basis_i() const
    { return normalize(point_b() - point_a()); }

Vector TriangleSegment::basis_j() const {
    // basis j must be a vector such that i x j = n
    // unit vector cross unit vector = another unit vector
    auto rv = cross(normal(), basis_i());
#   ifdef MACRO_DEBUG
    assert(are_very_close(magnitude(rv), 1.));
#   endif
    return rv;
}

bool TriangleSegment::projectable_onto(const Vector & n) const noexcept {
    auto [a, b, c] = project_onto_plane_(n);
    return !are_parallel(b - a, b - c);
}

Vector TriangleSegment::center() const noexcept
    { return (1. / 3.)*(point_a() + point_b() + point_c()); }

Vector2 TriangleSegment::center_in_2d() const noexcept
    { return (1. / 3.)*(point_a_in_2d() + point_b_in_2d() + point_c_in_2d()); }

SideCrossing TriangleSegment::check_for_side_crossing
    (const Vector2 & old, const Vector2 & new_) const
{
    check_invarients();
    if (old == new_ || contains_point(old) == contains_point(new_))
        { return SideCrossing{}; }

    auto mk_crossing = [this, old, new_] (TriangleSide r, Vector2 inx) {
        bool contains_old = contains_point(old);
        auto far_in  = contains_old ? old  : new_;
        auto far_out = contains_old ? new_ : old ;
        auto [li, fo] = move_to_last_in_and_first_out(
            far_in, far_out,
            next_in_direction(inx, far_in  - far_out),
            next_in_direction(inx, far_out - far_in));
        return SideCrossing{r, li, fo};
    };

    auto a2 = point_a_in_2d();
    auto b2 = point_b_in_2d();
    auto c2 = point_c_in_2d();

    if (auto gv = ::find_intersection(a2, b2, old, new_))
        { return mk_crossing(k_side_ab, gv.value()); }
    if (auto gv = ::find_intersection(b2, c2, old, new_))
        { return mk_crossing(k_side_bc, gv.value()); }
    if (auto gv = ::find_intersection(c2, a2, old, new_))
        { return mk_crossing(k_side_ca, gv.value()); }

    return mk_crossing(
        contains_point(new_) ? point_region(old) : point_region(new_),
        old + new_);
}

Vector2 TriangleSegment::closest_contained_point(const Vector & p) const {
    check_invarients();
    auto r = closest_point(p);
    // do something to r...
    if (contains_point(r)) return r;

    // beautifully self-referential
    return check_for_side_crossing(center_in_2d(), r).inside;
}

Vector2 TriangleSegment::closest_point(const Vector & p) const {
    check_invarients();
    // find via projection
    // https://math.stackexchange.com/questions/633181/formula-to-project-a-vector-onto-a-plane
    // by copyright law, mathematics/geometry cannot be copyrighted

    auto pa = p - point_a();
    auto pa_on_plane = pa - dot(pa, normal())*normal();
    return Vector2{get_component_for_basis(pa_on_plane, basis_i()),
                   get_component_for_basis(pa_on_plane, basis_j())};
}

bool TriangleSegment::contains_point(const Vector2 & r) const noexcept
    { return is_real(r) ? point_region(r) == k_inside : false; }

TriangleSegment TriangleSegment::flip() const noexcept {
    auto old_norm = normal();
    TriangleSegment rv{point_b(), point_a(), point_c()};
    auto new_norm = rv.normal();
    assert(are_very_close(angle_between(new_norm, old_norm), k_pi));
    return rv;
}

Vector2 TriangleSegment::intersection(const Vector & a, const Vector & b) const
    { return limit_with_intersection(a, b).intersection; }

LimitIntersection TriangleSegment::limit_with_intersection
    (const Vector & a, const Vector & b) const
{
    check_invarients();
    auto make_never_intersects = [b] {
        constexpr const auto k_no_sol = make_nonsolution_sentinel<Vector2>();
        return LimitIntersection{k_no_sol, b};
    };

    auto find_back_from_head = [this, a] {
        auto norm = normal();
        return [this, norm, a] (Vector b) {
            auto ba = b - a;
            auto denom = dot(norm, ba);

            // basically a case where the displacement is parallel to the segment
            if (are_very_close(denom, 0)) return k_inf;

            return dot(norm, b - point_a()) / denom;
        };
    } ();

    auto back_from_head = find_back_from_head(b);

    // 0 at head, 1 at tail
    // this branch catches if back_from_head is infinity
    if (!within_01(back_from_head))
        { return make_never_intersects(); }

    auto on_plane = b - (b - a)*back_from_head;

    auto r = closest_point(on_plane);
    // it's possible to hit the plane, but not be inside the triangle segment
    if (!contains_point(r)) return make_never_intersects();

    // as we approach b from a, search for a back_from_head that's barely over 1

    auto along_t = [a, b](Real t) { return a + (b - a)*t; };

    auto t = cul::find_highest_false<Real>([&find_back_from_head, &along_t] (Real t)
        { return within_01(find_back_from_head(along_t(t))); }, 1 - back_from_head);
    return LimitIntersection{ r, along_t(t) };
}

Vector TriangleSegment::normal() const noexcept
    { return normalize(cross(point_b() - point_a(), point_c() - point_a())); }

Vector TriangleSegment::opposing_point(Side side) const {
    switch (side) {
    case Side::k_side_ab: return point_c();
    case Side::k_side_bc: return point_a();
    case Side::k_side_ca: return point_b();
    default:
        throw InvArg{"TriangleSegment::opposing_point: given side must "
                     "represent a side (and not the inside)."};
    }
}

Vector TriangleSegment::point_a() const noexcept { return m_a; }

Vector2 TriangleSegment::point_a_in_2d() const noexcept
    { return Vector2{}; }

Vector TriangleSegment::point_b() const noexcept { return m_b; }

Vector2 TriangleSegment::point_b_in_2d() const noexcept
    { return Vector2{m_bx_2d, 0.}; }

Vector TriangleSegment::point_c() const noexcept { return m_c; }

Vector2 TriangleSegment::point_c_in_2d() const noexcept
    { return m_c_2d; }

Vector TriangleSegment::point_at(const Vector2 & r) const {
    if (!is_real(r)) {
         throw InvArg{"TriangleSurface::point_at: given point must have all "
                      "real number components."};
    }
    return point_a() + basis_i()*r.x + basis_j()*r.y;
}

TriangleSegment TriangleSegment::project_onto_plane(const Vector & n) const {
    auto [a, b, c] = project_onto_plane_(n);
    return TriangleSegment{a, b, c};
}

Tuple<Vector, Vector> TriangleSegment::side_points(Side side) const {
    switch (side) {
    case Side::k_side_ab: return make_tuple(point_a(), point_b());
    case Side::k_side_bc: return make_tuple(point_b(), point_c());
    case Side::k_side_ca: return make_tuple(point_c(), point_a());
    default:
        throw InvArg{"TriangleSegment::side_points: given side must "
                     "represent a side of the triangle (and not the inside)."};
    }
}

Tuple<Vector2, Vector2> TriangleSegment::side_points_in_2d(Side side) const {
    switch (side) {
    case Side::k_side_ab: return make_tuple(point_a_in_2d(), point_b_in_2d());
    case Side::k_side_bc: return make_tuple(point_b_in_2d(), point_c_in_2d());
    case Side::k_side_ca: return make_tuple(point_c_in_2d(), point_a_in_2d());
    default:
        throw InvArg{"TriangleSegment::side_points_in_2d: given side must "
                     "represent a side of the triangle (and not the inside)."};
    }
}

/* private */ void TriangleSegment::check_invarients() const noexcept {
#   ifdef MACRO_DEBUG
    assert(is_real(m_a));
    assert(is_real(m_b));
    assert(is_real(m_c));
    assert(is_real(point_a_in_2d()));
    assert(is_real(point_b_in_2d()));
    assert(is_real(point_c_in_2d()));
    assert(are_very_close(point_at(point_a_in_2d()), point_a()));
    assert(are_very_close(point_at(point_b_in_2d()), point_b()));
    assert(are_very_close(point_at(point_c_in_2d()), point_c()));
    assert(!are_very_close(normal(), Vector{}));
    assert(contains_point(center_in_2d()));
#   endif
}

/* private */ Tuple<Vector2, Vector2>
    TriangleSegment::move_to_last_in_and_first_out
    (const Vector2 & far_inside, const Vector2 & far_outside,
     const Vector2 & hint_li, const Vector2 & hint_fo) const noexcept
{
    assert( contains_point(far_inside ));
    assert(!contains_point(far_outside));
    if (!contains_point( hint_li ) || contains_point( hint_fo )) {
        // same utility that's used with point and line
        auto pos_along_line = [far_inside, far_outside](Real x)
            { return far_inside + x*(far_outside - far_inside); };

        auto [high_fal, low_true] = find_smallest_diff<Real>(
            [pos_along_line, this](Real x)
            { return !contains_point(pos_along_line(x)); });

        return make_tuple(pos_along_line(high_fal), pos_along_line(low_true));
    }
    assert( contains_point(hint_li));
    assert(!contains_point(hint_fo));
    return make_tuple(hint_li, hint_fo);
}

/* private */ TriangleSide TriangleSegment::point_region
    (const Vector2 & r) const
{
    if (!is_real(r)) {
        throw InvArg{"TriangleSegment::point_region: only real vectors should "
                     "reach this function."};
    }
    // mmm... always use the same method? floating points are quite odd
    // if there's a solution... r must be outside
    auto center = center_in_2d();
    auto is_crossed_line = [r, center] (const Vector2 & a, const Vector2 & b)
        { return ::find_intersection(a, b, center, r).has_value(); };
    auto a = point_a_in_2d();
    auto b = point_b_in_2d();
    if (are_parallel(a - b, a - r)) return k_inside;
    if (is_crossed_line(a, b)) return k_side_ab;

    auto c = point_c_in_2d();
    // cold branch
    if (are_parallel(b - c, b - r) || are_parallel(c - a, c - r)) return k_inside;
    if (is_crossed_line(b, c)) return k_side_bc;
    if (is_crossed_line(c, a)) return k_side_ca;
    return k_inside;
}

/* private */ Tuple<Vector, Vector, Vector>
    TriangleSegment::project_onto_plane_(const Vector & n) const noexcept
{
    return make_tuple(cul::project_onto_plane(point_a(), n),
                      cul::project_onto_plane(point_b(), n),
                      cul::project_onto_plane(point_c(), n));
}

const char * to_string(TriangleSide side) {
    switch (side) {
    case Side::k_inside : return "inside";
    case Side::k_side_ab: return "ab"    ;
    case Side::k_side_bc: return "bc"    ;
    case Side::k_side_ca: return "ca"    ;
    }
    throw MACRO_MAKE_BAD_BRANCH_EXCEPTION();
}

namespace {

// a possible way to expand cul
// angle between being an object, check for obtuse, and possibly get the value
bool angle_between_is_obtuse(const Vector &, const Vector &);

Real get_component_for_basis(const Vector & pt_on_plane, const Vector & basis) {
    // basis is assumed to be a normal vector
    assert(are_very_close(magnitude(basis), 1.));

    auto proj = project_onto(pt_on_plane, basis);

    // anti-parallel? scalar value is negative
    bool is_antipara = dot(proj, basis) / (magnitude(proj) * 1.) < 0.;
    return (is_antipara ? -1. : 1.)*magnitude(proj);
}

Vector2 find_point_c_in_2d
    (const Vector & a, const Vector & b, const Vector & c)
{
    auto ca = c - a;
    auto ba = b - a;
    auto i_proj = project_onto(ca, ba);

    return Vector2{
        (angle_between_is_obtuse(ca, ba) ? -1 : 1)*magnitude(i_proj),
        magnitude(ca - i_proj)};
}

Real find_point_b_x_in_2d
    (const Vector & a, const Vector & b)
{ return magnitude(b - a); }

// ----------------------------------------------------------------------------

bool angle_between_is_obtuse(const Vector & a, const Vector & b) {
    auto res = dot(a, b);
    return res < 0;
}

} // end of <anonymous> namespace
