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

#include "SpatialPartitionMap.hpp"

namespace {

using Triangle = ProjectionLine::Triangle;
using Interval = ProjectionLine::Interval;

using namespace cul::exceptions_abbr;

} // end of <anonymous> namespace ---------------------------------------------

ProjectionLine::ProjectionLine(const Vector & a_, const Vector & b_):
    m_a(a_), m_b(b_)
{
    if (!are_very_close(a_, b_)) return;
    throw InvArg{"ProjectionLine::ProjectionLine: points a and b must be "
                 "two different points to form a line."};
}

Interval ProjectionLine::interval_for(const Triangle & triangle) const {
    std::array pts
        { triangle.point_a(), triangle.point_b(), triangle.point_c() };
    return interval_for(&pts[0], &pts[0] + pts.size());
}

Interval ProjectionLine::interval_for
    (const Vector & a, const Vector & b) const
{
    std::array pts{a, b};
    return interval_for(&pts[0], &pts[0] + pts.size());
}

Real ProjectionLine::point_for(const Vector & r) const {
    auto pt_on_line = find_closest_point_to_line(m_a, m_b, r);
    auto mag = magnitude(pt_on_line - m_a);
    auto dir = dot(pt_on_line - m_a, m_b - m_a) < 0 ? -1 : 1;
    return mag*dir;
}

/* private */ Interval ProjectionLine::interval_for
    (const Vector * beg, const Vector * end) const
{
    assert(beg != end);
    auto [min, max] = std::minmax_element
        (beg, end,
         [this](const Vector & lhs, const Vector & rhs)
         { return point_for(lhs) < point_for(rhs); });
    return Interval{point_for(*min), point_for(*max)};
}

// ----------------------------------------------------------------------------

SpatialPartitionMap::Iterator & SpatialPartitionMap::Iterator::operator ++ () {
    ++m_itr;
    return *this;
}

SpatialPartitionMap::SpatialPartitionMap(const EntryContainer & sorted_entries)
    { populate(sorted_entries); }

void SpatialPartitionMap::populate(const EntryContainer & sorted_entries) {
    if (!Helpers::is_sorted(sorted_entries))
        { throw InvArg{"entries must be sorted"}; }

    m_container.clear();

    // sorted_entries is our temporary
    auto divisions = Helpers::compute_divisions(sorted_entries);

    // indicies represent would be positions in the destination container
    DivisionsPopulator<std::size_t> index_divisions;
    Helpers::make_indexed_divisions
        (sorted_entries, divisions, index_divisions, m_container);

    auto idx_to_itr = [this](std::size_t idx) {
        static constexpr const auto k_idx_oob_msg =
            "SpatialPartitionMap::populate: index is out of bounds for entries "
            "container; something is broken";
        if (idx > m_container.size())
            { throw RtError{k_idx_oob_msg}; }
        return m_container.begin() + idx;
    };

    // after all entries are in, convert indicies into iterators
    m_divisions = Divisions<EntryIterator>
        {Divisions<std::size_t>{std::move(index_divisions)}, idx_to_itr};
}

View<SpatialPartitionMap::Iterator>
    SpatialPartitionMap::view_for(const Interval & interval) const
{
    if (m_divisions.count() == 0) {
        return View{Iterator{m_container.end()}, Iterator{m_container.end()}};
    }
    auto [beg, end] = m_divisions.pair_for(interval);
    return View{Iterator{beg}, Iterator{end}};
}

// ----------------------------------------------------------------------------

ProjectedSpatialMap::ProjectedSpatialMap(const TriangleLinks & links)
    { populate(links); }

void ProjectedSpatialMap::populate(const TriangleLinks & links) {
    using EntryContainer = SpatialPartitionMap::EntryContainer;

    m_projection_line = make_line_for(links);

    EntryContainer entries;
    entries.reserve(links.size());
    for (auto & link : links) {
        entries.emplace_back
            (m_projection_line.interval_for(link->segment()), link);
    }

    Helpers::sort_entries_container(entries);
    m_spatial_map.populate(entries);
}

View<ProjectedSpatialMap::Iterator>
    ProjectedSpatialMap::view_for
    (const Vector & a, const Vector & b) const
{
    auto points_interval = m_projection_line.interval_for(a, b);
    return m_spatial_map.view_for(points_interval);
}

/* private static */ ProjectionLine
    ProjectedSpatialMap::make_line_for
    (const TriangleLinks & links)
{
    Vector low { k_inf,  k_inf,  k_inf};
    Vector high{-k_inf, -k_inf, -k_inf};
    for (auto & link : links) {
        const auto & triangle = link->segment();
        for (auto pt : { triangle.point_a(), triangle.point_b(), triangle.point_c() }) {
            auto low_list = {
                make_tuple(&pt.x, &low.x),
                make_tuple(&pt.y, &low.y),
                make_tuple(&pt.z, &low.z),
            };
            auto high_list = {
                make_tuple(&pt.x, &high.x),
                make_tuple(&pt.y, &high.y),
                make_tuple(&pt.z, &high.z),
            };
            for (auto [i, low_i] : low_list) {
                *low_i = std::min(*low_i, *i);
            }
            for (auto [i, high_i] : high_list) {
                *high_i = std::max(*high_i, *i);
            }
        }
    }
    auto line_options = {
        make_tuple(high.x - low.x, Vector{high.x, 0, 0}, Vector{low.x, 0, 0}),
        make_tuple(high.y - low.y, Vector{0, high.y, 0}, Vector{0, low.y, 0}),
        make_tuple(high.z - low.z, Vector{0, 0, high.z}, Vector{0, 0, low.z})
    };
    auto choice = std::max_element(line_options.begin(), line_options.end(),
                     [] (auto & lhs, auto & rhs)
    { return std::get<0>(lhs) < std::get<0>(rhs); });
    return ProjectionLine{std::get<1>(*choice), std::get<2>(*choice)};
}
