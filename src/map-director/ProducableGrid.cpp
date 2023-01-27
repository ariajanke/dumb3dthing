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
#include "GidTidTranslator.hpp"

void ProducableTileViewGrid::set_layers
    (UnfinishedProducableTileViewGrid && unfinished_grid,
     GidTidTranslator && gidtid_translator)
{
    std::tie(m_factories, m_groups)
        = unfinished_grid.move_out_producables_and_groups();
    m_tilesets = gidtid_translator.move_out_tilesets();
}

// ----------------------------------------------------------------------------

void UnfinishedProducableTileViewGrid::add_layer
    (Grid<ProducableTile *> && target,
     const std::vector<SharedPtr<ProducableGroup_>> & groups)
{
    m_groups.insert(m_groups.end(), groups.begin(), groups.end());
    m_targets.emplace_back(std::move(target));
}

Tuple
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
    return make_tuple(ViewGrid{std::move(producables_inserter)}, std::move(m_groups));
}
