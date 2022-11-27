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

#include "map-loader-helpers.hpp"

namespace {

using GridOfViews = InterTriangleLinkContainer::GridOfViews;

} // end of <anonymous> namespace

void TeardownTask::on_occasion(Callbacks &) {
    for (auto & ent : m_entities)
        { ent.request_deletion(); }
}

// ----------------------------------------------------------------------------

TileFactorySubGrid TileFactoryGrid::make_subgrid
    (const Rectangle & range) const
{
    return TileFactorySubGrid{m_factories, cul::top_left_of(range),
                              range.width, range.height};
}

// ----------------------------------------------------------------------------

InterTriangleLinkContainer::InterTriangleLinkContainer
    (const GridOfViews & views)
{
    append_links_by_predicate<is_not_edge_tile>(views, m_links);
    auto idx_for_edge = m_links.size();
    append_links_by_predicate<is_edge_tile>(views, m_links);
    m_edge_begin = m_links.begin() + idx_for_edge;
}

void InterTriangleLinkContainer::glue_to
    (InterTriangleLinkContainer & rhs)
{
    for (auto itr = edge_begin(); itr != edge_end(); ++itr) {
        for (auto jtr = rhs.edge_begin(); jtr != rhs.edge_end(); ++jtr) {
            (**itr).attempt_attachment_to(*jtr);
            (**jtr).attempt_attachment_to(*itr);
        }
    }
}

/* private static */ bool InterTriangleLinkContainer::is_edge_tile
    (const GridOfViews & grid, const Vector2I & r)
{
    return std::any_of
        (k_neighbor_offsets.begin(), k_neighbor_offsets.end(),
         [&] (const Vector2I & offset)
         { return !grid.has_position(offset + r); });
}

/* private static */ bool InterTriangleLinkContainer::is_not_edge_tile
    (const GridOfViews & grid, const Vector2I & r)
    { return !is_edge_tile(grid, r); }

template <bool (*meets_pred)(const GridOfViews &, const Vector2I &)>
/* private static */ void InterTriangleLinkContainer::append_links_by_predicate
    (const GridOfViews & views, std::vector<SharedPtr<TriangleLink>> & links)
{
    for (Vector2I r; r != views.end_position(); r = views.next(r)) {
        if (!meets_pred(views, r)) continue;
        for (auto & link : views(r)) {
            links.push_back(link);
        }
    }
}
