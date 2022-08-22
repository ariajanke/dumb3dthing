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

#include "PointAndPlaneDriver.hpp"

#include <common/TestSuite.hpp>

namespace {

#define MACRO_MAKE_BAD_BRANCH_EXCEPTION() BadBranchException(__LINE__, __FILE__)

using namespace cul::exceptions_abbr;
using namespace point_and_plane;
using std::get_if, std::get;
using cul::find_smallest_diff, cul::is_solution, cul::project_onto,
      cul::sum_of_squares, cul::EnableIf;
using SegmentTransfer = EventHandler::SegmentTransfer;

class DriverComplete final : public Driver {
public:
    void add_triangle(const TriangleLinks &) final;

    void remove_triangle(const TriangleLinks &) final;

    void clear_all_triangles() final;

    Driver & update() final;

    State operator () (const State &, const EventHandler &) const final;

private:
    // the job of each method here is to reduce displacement
    State handle_freebody(const InAir &, const EventHandler &) const;

    State handle_tracker(const OnSurface &, const EventHandler &) const;

    const TriangleLinks & find_links_for(std::shared_ptr<const Triangle>) const;

    // both containers are owning
    std::unordered_map<std::size_t, TriangleLinks> m_links;
    std::vector<std::shared_ptr<const Triangle>> m_triangles;
};

} // end of <anonymous> namespace

namespace point_and_plane {

TriangleLinks::TriangleLinks(SharedCPtr<Triangle> tptr):
    m_segment(tptr)
{
    if (tptr) return;
    throw InvArg{"TriangleLinks::TriangleLinks: must own a triangle."};
}

TriangleLinks & TriangleLinks::attempt_attachment_to(SharedCPtr<Triangle> tptr) {
    return  attempt_attachment_to(tptr, Side::k_side_ab)
           .attempt_attachment_to(tptr, Side::k_side_bc)
           .attempt_attachment_to(tptr, Side::k_side_ca);
}

TriangleLinks & TriangleLinks::attempt_attachment_to
    (SharedCPtr<Triangle> other, Side other_side)
{
    if (other == m_segment) {
        throw InvArg{"TriangleLinks::attempt_attachment_to: attempted to "
                     "attach triangle to an identical triangle."};
    }
    verify_valid_side("TriangleLinks::attempt_attachment_to", other_side);
    auto [oa, ob] = other->side_points(other_side); {}
    for (auto this_side : { Side::k_side_ab, Side::k_side_bc, Side::k_side_ca }) {
        auto [ta, tb] = m_segment->side_points(this_side); {}
        bool has_flipped_points    = are_very_close(oa, tb) && are_very_close(ob, ta);
        bool has_nonflipped_points = are_very_close(oa, ta) && are_very_close(ob, tb);
        if (!has_flipped_points && !has_nonflipped_points) continue;

        auto & info = m_triangle_sides[this_side];
        info.flip = has_flipped_points;
        info.side = other_side;
        // next call gets really complicated!
        info.inverts = !has_opposing_normals(*other, other_side, *m_segment, this_side);
        info.target = other;
        break;
    }
    return *this;
}

bool TriangleLinks::has_side_attached(Side side) const {
    verify_valid_side("TriangleLinks::has_side_attached", side);
    return !!m_triangle_sides[side].target.lock();
}

const Triangle & TriangleLinks::segment() const {
    assert(m_segment);
    return *m_segment;
}

TriangleLinks::Transfer TriangleLinks::transfers_to(Side side) const {
    verify_valid_side("TriangleLinks::transfers_to", side);
    auto & info = m_triangle_sides[side];
    Transfer rv;
    rv.flips = info.flip;
    rv.inverts = info.inverts;
    rv.side = info.side;
    rv.target = info.target.lock();
    return rv;
}

/* static */ void TriangleLinks::run_tests() {
#   define mark MACRO_MARK_POSITION_OF_CUL_TEST_SUITE
    using namespace cul::ts;
    using Vec2 = Vector2;
    static auto make_tri = [](Vec2 a, Vec2 b, Vec2 c) {
        auto to_v3 = [](Vec2 r) { return Vector{r.x, r.y, 0}; };
        return std::make_shared<Triangle>(to_v3(a), to_v3(b), to_v3(c));
    };
    TestSuite suite;
    suite.start_series("TriangleLinks");
    mark(suite).test([] {
        auto triangle_a = make_tri(Vec2{0, 0}, Vec2{0, 1}, Vec2{ 1, 1});
        auto triangle_b = make_tri(Vec2{0, 0}, Vec2{0, 1}, Vec2{-1, 0});
        TriangleLinks links{triangle_a};
        links.attempt_attachment_to(triangle_b);
        return test(    links.transfers_to(Side::k_side_ab).target == triangle_b
                    && !links.transfers_to(Side::k_side_bc).target
                    && !links.transfers_to(Side::k_side_ca).target);
    });
#   if 1
    mark(suite).test([] {
        // triangle a's ca side to...
        auto triangle_a = std::make_shared<Triangle>(
            // pt_a             , pt_b                , pt_c
            Vector{2.5, 0, -3.5}, Vector{2.5, 0, -4.5}, Vector{3.5, 0, -4.5});
        // triangle b's ab
        auto triangle_b = std::make_shared<Triangle>(
            // pt_a             , pt_c                , pt_d
            Vector{2.5, 0, -3.5}, Vector{3.5, 0, -4.5}, Vector{3.5, 0, -3.5});

        TriangleLinks links{triangle_b};
        links.attempt_attachment_to(triangle_a);
        auto trans = links.transfers_to(Side::k_side_ab);
        return test(    trans.target && trans.inverts
                    && !links.transfers_to(Side::k_side_bc).target
                    && !links.transfers_to(Side::k_side_ca).target);
    });
    mark(suite).test([] {
        // triangle a's ca side to...
        auto triangle_a = std::make_shared<Triangle>(
            // pt_a             , pt_b                , pt_c
            Vector{2.5, 0, -3.5}, Vector{2.5, 0, -4.5}, Vector{3.5, 0, -4.5});
        // triangle b's ab
        auto triangle_b = std::make_shared<Triangle>(
            // pt_a             , pt_c                , pt_d
            Vector{2.5, 0, -3.5}, Vector{3.5, 0, -4.5}, Vector{3.5, 0, -3.5});

        TriangleLinks links{triangle_a};
        links.attempt_attachment_to(triangle_b);
        auto trans = links.transfers_to(Side::k_side_ca);
        return test(    trans.target && trans.inverts
                    && !links.transfers_to(Side::k_side_bc).target
                    && !links.transfers_to(Side::k_side_ab).target);
    });
    mark(suite).test([] {
        // triangle a's ca side to...
        auto triangle_a = std::make_shared<Triangle>(
            // pt_a             , pt_b                , pt_c
            Vector{2.5, 0, -3.5}, Vector{2.5, 0, -4.5}, Vector{3.5, 0, -4.5});
        // triangle b's ab
        auto triangle_b = std::make_shared<Triangle>(
            // pt_a             , pt_c                , pt_d
            Vector{2.5, 0, -3.5}, Vector{3.5, 0, -4.5}, Vector{3.5, 0, -3.5});

        TriangleLinks a_links{triangle_a};
        a_links.attempt_attachment_to(triangle_b);
        auto a_trans = a_links.transfers_to(Side::k_side_ca);

        TriangleLinks b_links{triangle_b};
        b_links.attempt_attachment_to(triangle_a);
        auto b_trans = b_links.transfers_to(Side::k_side_ab);

        return test(    a_trans.target && a_trans.inverts
                    && !a_links.transfers_to(Side::k_side_bc).target
                    && !a_links.transfers_to(Side::k_side_ab).target
                    &&  b_trans.target && b_trans.inverts
                    && !b_links.transfers_to(Side::k_side_bc).target
                    && !b_links.transfers_to(Side::k_side_ca).target);
    });
    // something else?
    // false positive
    mark(suite).test([] {
        Vector2 displacement{0.018206371897582618,0.018211294926158639};
        Vector2 location{0.35605308997654295, 0.35604975301640995};
        Triangle triangle{Vector{2.5, 0, 0.5}, Vector{3.5, 0, -0.5}, Vector{3.5, 0, 0.5}};
        auto new_loc_ = location + displacement;
        // deeper inside... ruled as parallel to side of the triangle
        auto inside_maybe = triangle.check_for_side_crossing(location, new_loc_).side;
        if (inside_maybe != Side::k_inside) return test(true);
        // if "side_crossing" returns k_inside, then if must contain "new_loc_"
        return test(triangle.contains_point(new_loc_));
    });
#   endif
#   undef mark
}

/* private static */ inline bool TriangleLinks::has_opposing_normals
    (const Triangle & lhs, Side left_side, const Triangle & rhs, Side right_side)
{
    // assumption, sides "line up"
    assert(([&] {
        auto [la, lb] = lhs.side_points(left_side);
        auto [ra, rb] = rhs.side_points(right_side);
        return    (are_very_close(la, ra) && are_very_close(lb, rb))
               || (are_very_close(la, rb) && are_very_close(lb, ra));
    } ()));


    auto [la, lb] = lhs.side_points(left_side); {}
    auto plane_v = lb - la;

    using cul::project_onto_plane, cul::angle_between, cul::project_onto, cul::find_closest_point_to_line, cul::sum_of_squares;

    // first... project everything relevant onto plane orthogonal to rotation
    // axis
    auto rotme = project_onto_plane(lhs.opposing_point(left_side), plane_v);
    auto segend = project_onto_plane(rhs.opposing_point(right_side), plane_v);
    auto pivot = project_onto_plane(la, plane_v);

    auto project_onto_line_segment = [](const Vector & a, const Vector & b, const Vector & ex) -> Vector {
        // all assumed to be roughly coplanar...
        // choose an origin, why not a?
        // then re-adjust (what does that mean?)
        return a + project_onto(b - a, ex - a);
    };
    // I will have to change the origin again to use this!
    auto make_rotate_around_axis = [](const Vector & axis) {
        // and so follows... Rodrigues' formula
        auto k = normalize(axis);
        using std::sin, std::cos;
        return [k](const Vector & v, Real t) {
            auto cos_t = cos(t);
            return v*cos_t + cross(k, v)*sin(t) + k*dot(k, v)*(1 - cos_t);
        };
    };
    // rotme projected onto line described by the other triangle
    auto rotprojd = project_onto_line_segment(pivot, segend, rotme);
    // get possible rotations
    auto t0 = angle_between(rotme - pivot, rotprojd - pivot);
    auto t1 = k_pi - t0;

    auto segmid = 0.5*(pivot + segend);
    auto rotate_vec = make_rotate_around_axis(plane_v);
    bool t0_is_sol =
        sum_of_squares(rotate_vec(rotme - pivot, t0) - (segmid - pivot)) <
        sum_of_squares(rotate_vec(rotme - pivot, t1) - (segmid - pivot));

    // cancel out? they are opposing
    // not so? they are NOT opposing
    auto rot_norm = rotate_vec(lhs.normal(), t0_is_sol ? t0 : t1);
    return are_very_close(sum_of_squares( rot_norm - rhs.normal() ), 0);
}


/* private static */ TriangleSide TriangleLinks::verify_valid_side
    (const char * caller, Side side)
{
    switch (side) {
    case Side::k_side_ab: case Side::k_side_bc: case Side::k_side_ca:
        return side;
    default: break;
    }
    throw InvArg{  std::string{caller}
                 + ": side must be valid value and not k_inside."};
}

Vector location_of(const State & state) {
    auto * in_air = std::get_if<InAir>(&state);
    if (in_air) return in_air->location;
    auto & on_surf = std::get<OnSurface>(state);
    return on_surf.segment->point_at(on_surf.location);
}

/* static */ std::unique_ptr<Driver> Driver::make_driver()
    { return std::make_unique<DriverComplete>(); }

} // end of point_and_plane namespace


namespace {

template <typename Vec1, typename Vec2>
void verify_decreasing_displacement
    (EnableIf<cul::VectorTraits<Vec1>::k_is_vector_type, const Vec1 &> displc,
     EnableIf<cul::VectorTraits<Vec2>::k_is_vector_type, const Vec2 &> old_displacement,
     const char * caller);

inline std::size_t to_key(const SharedCPtr<Triangle> & tptr)
    { return std::hash<const Triangle *>{}(tptr.get()); }

// should/add remove fast
void DriverComplete::add_triangle(const TriangleLinks & links) {
    m_links.insert(std::make_pair( links.hash(), links ));
}

void DriverComplete::remove_triangle(const TriangleLinks & links) {
    m_links.erase(links.hash());
}

void DriverComplete::clear_all_triangles() {
    m_triangles.clear();
    m_links.clear();
}

Driver & DriverComplete::update() {
    m_triangles.clear();
    m_triangles.reserve(m_links.size());
    for (auto & pair : m_links) {
        m_triangles.emplace_back(pair.second.segment_ptr());
    }
    return *this;
}

State DriverComplete::operator ()
    (const State & state, const EventHandler & env) const
{
    // before returning, this must be true:
    //
    // are_very_close(/* something */.displacement, make_zero_vector<Vector>())

    auto next_state = [this](const State & state, const EventHandler & env) {
        if (auto * freebody = get_if<InAir>(&state)) {
            return handle_freebody(*freebody, env);
        }
        if (auto * tracker = get_if<OnSurface>(&state)) {
            return handle_tracker(*tracker, env);
        }
        throw MACRO_MAKE_BAD_BRANCH_EXCEPTION();
    };

    auto has_near_zero_displacement = [](const State & state) {
        if (auto * freebody = get_if<InAir>(&state)) {
            return are_very_close(freebody->displacement, Vector{});
        }
        if (auto * tracker = get_if<OnSurface>(&state)) {
            return are_very_close(tracker->displacement, Vector2{});
        }
        throw MACRO_MAKE_BAD_BRANCH_EXCEPTION();
    };

    auto cur_state = state;
    while (!has_near_zero_displacement(cur_state)) {
         cur_state = next_state(cur_state, env);
    }
    return cur_state;
}

/* private */ State DriverComplete::handle_freebody
    (const InAir & freebody, const EventHandler & env) const
{
    const auto * beg = &m_triangles.front();
    const auto * end = beg + m_triangles.size();
    auto new_loc = freebody.location + freebody.displacement;
    for (auto itr = beg; itr != end; ++itr) {
        auto & triangle = **itr;
        auto r = triangle.intersection(freebody.location, new_loc);

        if (!is_solution(r)) continue;
        auto gv = env.displacement_after_triangle_hit(
            triangle, freebody.location, new_loc, triangle.point_at(r));
        if (auto * disv2 = get_if<Vector2>(&gv)) {
            // displacement must be real, and decreasing
            // (decreasing by how much?
            triangle.intersection(freebody.location, new_loc);
            verify_decreasing_displacement<Vector2, Vector>(
                *disv2, freebody.displacement,
                "DriverComplete::handle_freebody");
            bool heads_against_normal = are_very_close(
                  normalize(project_onto(new_loc - freebody.location,
                                         triangle.normal()          ))
                - triangle.normal(), Vector{});
            std::cout << "Made landing" << std::endl;
            return OnSurface{*itr, heads_against_normal, r, *disv2};
        } else if (auto * disv3 = get_if<Vector>(&gv)) {
            verify_decreasing_displacement<Vector, Vector>(
                *disv3, freebody.displacement,
                "DriverComplete::handle_freebody");
            // sigmoid false -> true
            auto [before, after] = find_smallest_diff<Real>([&triangle, &freebody](Real x)
                { return !is_solution( triangle.intersection( freebody.location, freebody.location + freebody.displacement*x) ); });
            return InAir{freebody.location + before*freebody.displacement,
                         freebody.displacement*(1 - after)               };
        }
        throw MACRO_MAKE_BAD_BRANCH_EXCEPTION();
    }
    return InAir{freebody.location + freebody.displacement, Vector{}};
}

/* private */ State DriverComplete::handle_tracker
    (const OnSurface & tracker, const EventHandler & env) const
{
    assert(tracker.segment);
    // ground opens up from underneath

    const auto & triangle = *tracker.segment;

    // check collisions with other surfaces while traversing the "tracked" segment
    const TriangleLinks * beg = nullptr;
    const TriangleLinks * end = nullptr;
    // I think I may skip this for now, until I have some results

    // usual segment transfer
    auto gv = triangle.check_for_side_crossing(tracker.location, tracker.location + tracker.displacement);
    if (gv.side == TriangleSide::k_inside) {
        if (!tracker.segment->contains_point(tracker.location + tracker.displacement)) {
            triangle.check_for_side_crossing(tracker.location, tracker.location + tracker.displacement);
        }
        assert(tracker.segment->contains_point(tracker.location + tracker.displacement));
        return OnSurface{tracker.segment, tracker.invert_normal,
                         tracker.location + tracker.displacement, Vector2{}};
    }

    const auto transfer = find_links_for(tracker.segment).transfers_to(gv.side);
    if (!transfer.target) {
        // without a callback I cannot cancel velocity
        if (env.cling_to_edge(*tracker.segment, gv.side)) {
            OnSurface rv{tracker};
            rv.location = gv.inside;
            rv.displacement = Vector2{};
            std::cout << "Cling to edge" << std::endl;
            return rv;
        }
        std::cout << "Slip off without attached" << std::endl;
        return InAir{triangle.point_at( gv.outside ), Vector{}};
    }

    auto inx_pt = triangle.point_at( gv.outside );
    auto rem_displc_var = env.pass_triangle_side(
        triangle, transfer.target.get(), inx_pt,
        inx_pt - triangle.point_at(tracker.location));
    if (auto * to_tri = get_if<SegmentTransfer>(&rem_displc_var)) {
        static int i = 0;
        std::cout << "intersegment (" << (i++) << ") transfer occured..."
                  << &triangle << " to " << transfer.target.get() << " : "
                  << tracker.segment->normal() << " to " << transfer.target->normal() << std::endl;
        // you mean to transfer
        verify_decreasing_displacement<Vector2, Vector2>(
            to_tri->displacement, tracker.displacement,
            "DriverComplete::handle_tracker");
        return OnSurface{transfer.target, bool(transfer.inverts ^ to_tri->invert),
            transfer.target->closest_contained_point(inx_pt), to_tri->displacement};
    } else if (auto * disv3 = get_if<Vector>(&rem_displc_var)) {
        // you mean to "slip off"
        std::cout << "Slip off w/ attached" << std::endl;
        verify_decreasing_displacement<Vector, Vector2>(
            *disv3, tracker.displacement,
            "DriverComplete::handle_tracker");
        return InAir{inx_pt, *disv3};
    }

    throw MACRO_MAKE_BAD_BRANCH_EXCEPTION();
}

/* private */ const TriangleLinks & DriverComplete::find_links_for
    (std::shared_ptr<const Triangle> tptr) const
{
    auto itr = m_links.find(std::hash<const Triangle *>{}(tptr.get()));
    if (itr == m_links.end()) {
        throw RtError{"Cannot find triangle links, was the triangle occupied "
                      "by the subject state added to the driver?"};
    }
    return itr->second;
}

// ----------------------------------------------------------------------------

template <typename Vec1, typename Vec2>
void verify_decreasing_displacement
    (EnableIf<cul::VectorTraits<Vec1>::k_is_vector_type, const Vec1 &> displc,
     EnableIf<cul::VectorTraits<Vec2>::k_is_vector_type, const Vec2 &> old_displacement,
     const char * caller)
{
    constexpr const auto k_min_dec =
        cul::ScalarTypeOf<Vec1>(0.05)*cul::ScalarTypeOf<Vec2>(0.05);
    if (!is_real(displc)) {
        throw InvArg{std::string{caller} + ": new displacement must be a real vector."};
    }
    if (sum_of_squares(displc) > sum_of_squares(old_displacement)) {
        throw InvArg{std::string{caller} + ": new displacement must be decreasing."};
    }
}

} // end of <anonymous> namespace
