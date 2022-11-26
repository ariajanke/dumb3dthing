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

#include "../Defs.hpp"
#include "../TriangleSegment.hpp"
#include "../TriangleLink.hpp"

#include <ariajanke/cul/Grid.hpp>
#include <ariajanke/cul/RectangleUtils.hpp>

struct Slopes final {
    Slopes() {}

    Slopes(Real ne_, Real nw_, Real sw_, Real se_):
        nw(nw_), ne(ne_), sw(sw_), se(se_) {}

    bool operator == (const Slopes & rhs) const noexcept
        { return are_same(rhs); }

    bool operator != (const Slopes & rhs) const noexcept
        { return !are_same(rhs); }

    bool are_same(const Slopes & rhs) const noexcept {
        using Fe = std::equal_to<Real>;
        return    Fe{}(nw, rhs.nw) && Fe{}(ne, rhs.ne)
               && Fe{}(sw, rhs.sw) && Fe{}(se, rhs.se);
    }

    Real nw, ne, sw, se;
};

inline Slopes half_pi_rotations(const Slopes & s, int n) {
    if (n < 0) throw std::invalid_argument{""};
    if (n == 0) return s;
    return half_pi_rotations(Slopes{s.se, s.ne, s.nw, s.sw}, n - 1);
}

inline Slopes translate_y(const Slopes & s, Real y)
    { return Slopes{s.ne + y, s.nw + y, s.sw + y, s.se + y}; }

class TriangleAdder {
public:
    virtual ~TriangleAdder() {}

    virtual void operator () (const TriangleSegment &) const = 0;

    template <typename Func>
    static auto make(Func && f) {
        class Impl final : public TriangleAdder {
        public:
            explicit Impl(Func && f_): m_f(std::move(f_)) {}

            void operator () (const TriangleSegment & tri) const final
                { m_f(tri); }
        private:
            Func m_f;
        };
        return Impl{std::move(f)};
    }
};

using TriangleLinks = std::vector<SharedPtr<TriangleLink>>;
#if 0
template <typename Func>
    Tuple
        <TriangleLinks, Grid<View<TriangleLinks::const_iterator>>>
    add_triangles_and_link_
    (int width, int height, Func && on_add_tile)
{
    using TriangleVec = std::vector<TriangleSegment>;
    using std::get;
    Grid<std::pair<std::size_t, std::size_t>> links_grid;
    links_grid.set_size(width, height);
    TriangleVec vec;
    auto adder = TriangleAdder::make([&vec] (const TriangleSegment & triangle)
        { vec.push_back(triangle); });
    for (Vector2I r; r != links_grid.end_position(); r = links_grid.next(r)) {
        links_grid(r).first = vec.size();
        on_add_tile(r, adder);
        links_grid(r).second = vec.size();
    }

    TriangleLinks rv1;
    rv1.reserve(vec.size());
    for (auto & tri : vec)
        { rv1.emplace_back(std::make_shared<TriangleLink>(tri)); }

    Grid<View<TriangleLinks::iterator>> link_grid;
    link_grid.set_size(links_grid.width(), links_grid.height(),
                       View<TriangleLinks::iterator>{ rv1.end(), rv1.end() });
    {
    auto beg = rv1.begin();
    for (Vector2I r; r != links_grid.end_position(); r = links_grid.next(r)) {
        link_grid(r) = View<TriangleLinks::iterator>{
            beg + links_grid(r).first, beg + links_grid(r).second};
    }
    }

    // now link them together
    for (Vector2I r; r != link_grid.end_position(); r = link_grid.next(r)) {
    for (auto & this_tri : link_grid(r)) {
        assert(this_tri);
        for (Vector2I v : { r, Vector2I{1, 0} + r, Vector2I{-1,  0} + r,
/*                          */ Vector2I{0, 1} + r, Vector2I{ 0, -1} + r}) {
            if (!links_grid.has_position(v)) continue;
            for (auto & other_tri : link_grid(v)) {
                assert(other_tri);

                if (this_tri == other_tri) continue;
                this_tri->attempt_attachment_to(other_tri);
        }}
    }}
    Grid<View<TriangleLinks::const_iterator>> rv2;
    rv2.set_size(links_grid.width(), links_grid.height(),
                 View<TriangleLinks::const_iterator>{ rv1.end(), rv1.end() });
    for (Vector2I r; r != rv2.end_position(); r = rv2.next(r)) {
        rv2(r) = View<TriangleLinks::const_iterator>
            { link_grid(r).begin(), link_grid(r).end() };
    }

    return make_tuple(std::move(rv1), std::move(rv2));
}
#endif
#if 0
template <typename Func>
    Tuple
        <TriangleLinks, Grid<View<TriangleLinks::const_iterator>>>
    add_triangles_and_link
    (const cul::Rectangle<int> & range, Func && on_add_tile)
{
    using TriangleVec = std::vector<TriangleSegment>;
    using std::get;
    Grid<std::pair<std::size_t, std::size_t>> links_grid;
    links_grid.set_size(range.width, range.height);
    TriangleVec vec;
    auto adder = TriangleAdder::make([&vec] (const TriangleSegment & triangle)
        { vec.push_back(triangle); });
    for (Vector2I r; r != links_grid.end_position(); r = links_grid.next(r)) {
        links_grid(r).first = vec.size();
        on_add_tile(r + cul::top_left_of(range), adder);
        links_grid(r).second = vec.size();
    }

    TriangleLinks rv1;
    rv1.reserve(vec.size());
    for (auto & tri : vec)
        { rv1.emplace_back(std::make_shared<TriangleLink>(tri)); }

    Grid<View<TriangleLinks::iterator>> link_grid;
    link_grid.set_size(links_grid.width(), links_grid.height(),
                       View<TriangleLinks::iterator>{ rv1.end(), rv1.end() });
    {
    auto beg = rv1.begin();
    for (Vector2I r; r != links_grid.end_position(); r = links_grid.next(r)) {
        link_grid(r) = View<TriangleLinks::iterator>{
            beg + links_grid(r).first, beg + links_grid(r).second};
    }
    }

    // now link them together
    for (Vector2I r; r != link_grid.end_position(); r = link_grid.next(r)) {
    for (auto & this_tri : link_grid(r)) {
        assert(this_tri);
        for (Vector2I v : { r, Vector2I{1, 0} + r, Vector2I{-1,  0} + r,
/*                          */ Vector2I{0, 1} + r, Vector2I{ 0, -1} + r}) {
            if (!links_grid.has_position(v)) continue;
            for (auto & other_tri : link_grid(v)) {
                assert(other_tri);

                if (this_tri == other_tri) continue;
                this_tri->attempt_attachment_to(other_tri);
        }}
    }}
    Grid<View<TriangleLinks::const_iterator>> rv2;
    rv2.set_size(links_grid.width(), links_grid.height(),
                 View<TriangleLinks::const_iterator>{ rv1.end(), rv1.end() });
    for (Vector2I r; r != rv2.end_position(); r = rv2.next(r)) {
        rv2(r) = View<TriangleLinks::const_iterator>
            { link_grid(r).begin(), link_grid(r).end() };
    }

    return make_tuple(std::move(rv1), std::move(rv2));
}
#endif
inline void link_triangles
    (const Grid<View<std::vector<SharedPtr<TriangleLink>>::const_iterator>> & link_grid)
{
    // now link them together
    for (Vector2I r; r != link_grid.end_position(); r = link_grid.next(r)) {
    for (auto & this_tri : link_grid(r)) {
        assert(this_tri);
        for (Vector2I v : { r, Vector2I{1, 0} + r, Vector2I{-1,  0} + r,
/*                          */ Vector2I{0, 1} + r, Vector2I{ 0, -1} + r}) {
            if (!link_grid.has_position(v)) continue;
            for (auto & other_tri : link_grid(v)) {
                assert(other_tri);

                if (this_tri == other_tri) continue;
                this_tri->attempt_attachment_to(other_tri);
        }}
    }}
}
