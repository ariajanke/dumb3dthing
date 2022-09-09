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

#include "TriangleSegment.hpp"

namespace {

#define MACRO_MAKE_BAD_BRANCH_EXCEPTION() BadBranchException(__LINE__, __FILE__)

using namespace cul::exceptions_abbr;

using Side = TriangleSide;
using SideCrossing = TriangleSegment::SideCrossing;
using LimitIntersection = TriangleSegment::LimitIntersection;

using cul::find_smallest_diff, cul::make_nonsolution_sentinel,
      cul::make_zero_vector;
using std::min_element;

template <typename T>
using EnableBoolIfVec = std::enable_if_t<cul::k_is_vector_type<T>, bool>;

Real find_intersecting_position_for_first
    (Vector2 first_line_a , Vector2 first_line_b ,
     Vector2 second_line_a, Vector2 second_line_b);

bool is_solution(Real x);

template <typename Vec>
EnableBoolIfVec<Vec> are_parallel(const Vec & a, const Vec & b);

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
    m_a(Vector{1, 0, 0}),
    m_b(Vector{0, 1, 0}),
    m_c(Vector{0, 0, 1}),
    m_bx_2d(find_point_b_x_in_2d(m_a, m_b)),
    m_c_2d(find_point_c_in_2d(m_a, m_b, m_c))
    { check_invarients(); }

TriangleSegment::TriangleSegment
    (const Vector & a, const Vector & b, const Vector & c):
    m_a(a), m_b(b), m_c(c),
    m_bx_2d(find_point_b_x_in_2d(a, b)),
    m_c_2d(find_point_c_in_2d(a, b, c))
{
    if (!is_real(a) || !is_real(b) || !is_real(c)) {
        throw InvArg{"TriangleSegment::TriangleSegment: points a, b, and c must "
                     "have all real components."};
    }
    if (are_parallel(b - a, b - c)) {
        throw InvArg{"TriangleSegment::TriangleSegment: points must not be "
                     "co-linear."};
    }
    assert(!are_parallel(point_b_in_2d() - point_a_in_2d(),
                         point_b_in_2d() - point_c_in_2d()));
    if (are_very_close(a, b) || are_very_close(b, c) || are_very_close(c, a)) {
        throw InvArg{"TriangleSegment::TriangleSegment: all three points must "
                     "be far enough apart, as to be recognized as a triangle."};
    }

    check_invarients();
}

Vector TriangleSegment::basis_i() const
    { return normalize(point_b() - point_a()); }

Vector TriangleSegment::basis_j() const {
    // basis j must be a vector such that i x j = n
    // unit vector cross unit vector = another unit vector
    auto rv = cross(normal(), basis_i());
    assert(are_very_close(magnitude(rv), 1.));
    return rv;
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
            next_in_direction(inx, far_out - far_in)); {}
        return SideCrossing{r, li, fo};
    };

    auto a2 = point_a_in_2d();
    auto b2 = point_b_in_2d();
    auto c2 = point_c_in_2d();
    auto gv = find_intersecting_position_for_first(a2, b2, old, new_);
    if (is_solution(gv))
        { return mk_crossing(k_side_ab, a2 + gv*(b2 - a2)); }

    gv = find_intersecting_position_for_first(b2, c2, old, new_);
    if (is_solution(gv))
        { return mk_crossing(k_side_bc, b2 + gv*(c2 - b2)); }

    gv = find_intersecting_position_for_first(c2, a2, old, new_);
    if (is_solution(gv))
        { return mk_crossing(k_side_ca, c2 + gv*(a2 - c2)); }

    return mk_crossing(
        contains_point(new_) ? point_region(old) : point_region(new_),
        old + new_);
}

Vector2 TriangleSegment::closest_contained_point(Vector p) const {
    check_invarients();
    auto r = closest_point(p);
    // do something to r...
    if (contains_point(r)) return r;

    // beautifully self-referential
    return check_for_side_crossing(center_in_2d(), r).inside;
}

Vector2 TriangleSegment::closest_point(Vector p) const {
    check_invarients();
    // find via projection
    // https://math.stackexchange.com/questions/633181/formula-to-project-a-vector-onto-a-plane
    // by copyright law, mathematics/geometry cannot be copyrighted

    auto pa = p - point_a();
    auto pa_on_plane = pa - dot(pa, normal())*normal();
    return Vector2{get_component_for_basis(pa_on_plane, basis_i()),
                   get_component_for_basis(pa_on_plane, basis_j())};
}

bool TriangleSegment::contains_point(Vector2 r) const noexcept
    { return is_real(r) ? point_region(r) == k_inside : false; }

TriangleSegment TriangleSegment::flip() const noexcept {
    auto old_norm = normal();
    TriangleSegment rv{point_b(), point_a(), point_c()};
    auto new_norm = rv.normal();
    assert(are_very_close(angle_between(new_norm, old_norm), k_pi));
    return rv;
}

Vector2 TriangleSegment::intersection(Vector a, Vector b) const {
    return limit_with_intersection(a, b).intersection;
#   if 0
    check_invarients();
    // multiple sources on this
    // from stackoverflow (multiple posts), blender, rosetta code, and on, and on
    // again, maintaining that simple geometic formulae as uncopyrightable
    constexpr const auto k_no_sol = make_nonsolution_sentinel<Vector2>();

    auto norm = normal();
    auto ba = b - a;
    auto denom = dot(norm, ba);
    if (are_very_close(denom, 0)) return k_no_sol;

    auto back_from_head = dot(norm, b - point_a()) / denom;
    // 0 at head, 1 at tail
    if (back_from_head < 0 || back_from_head > 1)
        { return k_no_sol; }

    // [NTS] back_from_head > 1 will be before the intersection
    //       I will need to find a value s.t. it's not counted as being on the
    //       plane (that is intersecting)
    auto on_plane = b - ba*back_from_head;
    auto r = closest_point(on_plane);
    // it's possible to hit the plane, but not be inside the triangle segment
    return contains_point(r) ? r : k_no_sol;
#   endif
}

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
    // v this algebra is wrong!!
    auto on_plane2 = a*back_from_head;
    auto on_plane = b - (b - a)*back_from_head;
    //assert(are_very_close(b - (b - a)*back_from_head, on_plane2));
    auto r = closest_point(on_plane);
    // it's possible to hit the plane, but not be inside the triangle segment
    if (!contains_point(r)) return make_never_intersects();

    // as we approach b from a, search for a back_from_head that's barely over 1

    auto along_t = [a, b](Real t) { return a + (b - a)*t; };

    auto t = cul::find_highest_false<Real>([&find_back_from_head, &along_t] (Real t)
        { return within_01(find_back_from_head(along_t(t))); }, 0.0005, 1 - back_from_head);
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

Vector TriangleSegment::point_at(Vector2 r) const {
    if (!is_real(r)) {
         throw InvArg{"TriangleSurface::point_at: given point must have all "
                      "real number components."};
    }
    return point_a() + basis_i()*r.x + basis_j()*r.y;
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
        { return is_solution(find_intersecting_position_for_first(a, b, center, r)); };
#   if 0
    auto intersecting_pt = [center, r](const Vector2 & a, const Vector2 & b)
        { return find_intersecting_position_for_first(a, b, center, r); };
#   endif
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

static const constexpr Real k_no_intersection_2d =
    std::numeric_limits<Real>::infinity();

Real find_intersecting_position_for_first
    (Vector2 first_line_a , Vector2 first_line_b ,
     Vector2 second_line_a, Vector2 second_line_b)
{    
    auto p = first_line_a;
    auto r = first_line_b - p;

    auto q = second_line_a;
    auto s = second_line_b - q;

    auto r_cross_s = cross(r, s);
    if (r_cross_s == 0.0) return k_no_intersection_2d;

    auto q_sub_p = q - p;
    auto t = cross(q_sub_p, s) / r_cross_s;
    if (t < 0. || t > 1.) return k_no_intersection_2d;

    auto u = cross(q_sub_p, r) / r_cross_s;
    if (u < 0. || u > 1.) return k_no_intersection_2d;

    if (are_parallel(first_line_a - first_line_b, second_line_a - second_line_b))
        { return k_no_intersection_2d; }

    return t;
}

bool is_solution(Real x)
    { return !std::equal_to<Real>{}(x, k_no_intersection_2d); }

template <typename Vec>
EnableBoolIfVec<Vec> are_parallel(const Vec & a, const Vec & b) {
#   if 0 // gdb *really* doesn't like me short circutting functions :c
    auto mag = magnitude(normalize(a) + normalize(b));
    return are_very_close(mag, 0) || are_very_close(mag, 2);
#   elif 1
    auto mk_zero = [] {
        constexpr auto k_dim = cul::VectorTraits<Vec>::k_dimension_count;
        static_assert(k_dim == 2 || k_dim == 3,
            "<anonymous>::are_parallel: must be used only with 2 or 3 dimensions.");
        if constexpr (k_dim == 2) return 0;
        else return make_zero_vector<Vec>();
    };
    return are_very_close(mk_zero(), cross(a, b));
#   else
    auto frac = (dot(a, b)*dot(a, b)) / (sum_of_squares(a)*sum_of_squares(b));
    return are_very_close(magnitude(frac), 1);
#   endif
}

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
    auto i_proj = project_onto(c - a, b - a);
    return Vector2{magnitude(i_proj), magnitude((c - a) - i_proj)};
}

Real find_point_b_x_in_2d
    (const Vector & a, const Vector & b)
{ return magnitude(b - a); }

} // end of <anonymous> namespace
