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

#include "ProducableGrid.hpp"
#include "ProducableGroupFiller.hpp"

#include "../TriangleSegment.hpp"

// TEMP
TriangleSegment TriangleSegmentTransformation::operator ()
    (const TriangleSegment & triangle) const
{ return m_scale(triangle).move(translation()); }

void ProducableTileCallbacks::add_collidable
    (const Vector & triangle_point_a,
     const Vector & triangle_point_b,
     const Vector & triangle_point_c)
{
    add_collidable_
        (TriangleSegment{triangle_point_a, triangle_point_b, triangle_point_c});
}

ModelTranslation TriangleSegmentTransformation::model_translation() const
    { return ModelTranslation{translation()}; }

ModelScale TriangleSegmentTransformation::model_scale() const
    { return m_scale.to_model_scale(); }

/* private */ Vector TriangleSegmentTransformation::translation() const {
    return Vector
        {Real(m_on_field_position.x), 0, Real(-m_on_field_position.y)};
}

// ----------------------------------------------------------------------------

ProducableTileViewGrid::ProducableTileViewGrid
    (ViewGrid<ProducableTile *> && factory_view_grid,
     std::vector<SharedPtr<ProducableGroup_>> && groups,
     std::vector<SharedPtr<const ProducableGroupFiller>> && fillers):
    m_factories(std::move(factory_view_grid)),
    m_groups(std::move(groups)),
    m_fillers(std::move(fillers))
{}

// ----------------------------------------------------------------------------

void UnfinishedProducableTileViewGrid::add_layer
    (Grid<ProducableTile *> && target,
     const std::vector<SharedPtr<ProducableGroup_>> & groups)
{
    m_groups.insert(m_groups.end(), groups.begin(), groups.end());
    m_targets.emplace_back(std::move(target));
}

ProducableTileViewGrid UnfinishedProducableTileViewGrid::
    finish(ProducableGroupFillerExtraction & extraction)
{
    auto [producables, groups] = move_out_producables_and_groups();
    auto map_fillers = extraction.move_out_fillers();
    return ProducableTileViewGrid
        {std::move(producables), std::move(groups), std::move(map_fillers)};
}

/* private */ Tuple
    <ViewGrid<ProducableTile *>,
     std::vector<SharedPtr<ProducableGroup_>>>
    UnfinishedProducableTileViewGrid::move_out_producables_and_groups()
{
    ViewGridInserter<ProducableTile *> producables_inserter{m_targets.front().size2()};
    for (Vector2I r; r != m_targets.front().end_position(); r = m_targets.front().next(r)) {
        for (auto & target : m_targets) {
            if (!target(r)) continue;
            producables_inserter.push(target(r));
        }
        producables_inserter.advance();
    }
    m_targets.clear();
    return make_tuple(producables_inserter.finish(), std::move(m_groups));
}

// ----------------------------------------------------------------------------

void ProducableGroupTileLayer::set_size(const Size2I & sz) {
    using namespace cul::exceptions_abbr;
    if (sz == Size2I{}) {
        throw InvArg{"ProducableGroupTileLayer::set_size: given size must be "
                     "non zero"};
    } else if (m_target.size2() != Size2I{}) {
        throw RtError{"ProducableGroupTileLayer::set_size: size may only be "
                      "set once"};
    }
    m_groups.clear();
    m_target.clear();
    m_target.set_size(sz, nullptr);
}

UnfinishedProducableTileViewGrid
    ProducableGroupTileLayer::move_self_to
    (UnfinishedProducableTileViewGrid && unfinished_grid_view)
{
    unfinished_grid_view.add_layer(std::move(m_target), m_groups);
    m_target.clear();
    m_groups.clear();
    return std::move(unfinished_grid_view);
}
