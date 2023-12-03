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
#include "MapRegion.hpp"

#include "../TriangleSegment.hpp"

void ProducableTileCallbacks::add_collidable
    (const Vector & triangle_point_a,
     const Vector & triangle_point_b,
     const Vector & triangle_point_c)
{
    add_collidable_
        (TriangleSegment{triangle_point_a, triangle_point_b, triangle_point_c});
}

// ----------------------------------------------------------------------------

ProducableTileViewGrid::ProducableTileViewGrid
    (ViewGrid<ProducableTile *> && factory_view_grid,
     std::vector<SharedPtr<ProducableGroupOwner>> && groups):
    m_factories(std::move(factory_view_grid)),
    m_groups(std::move(groups)) {}
