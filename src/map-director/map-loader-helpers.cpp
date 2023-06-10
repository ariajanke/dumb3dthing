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

#include "map-loader-helpers.hpp"

#include "RegionEdgeConnectionsContainer.hpp"
#if 0
namespace {

using ViewGridTriangle = RegionEdgeLinksContainer::ViewGridTriangle;

} // end of <anonymous> namespace

TeardownTask::TeardownTask
    (std::vector<Entity> && entities,
     const TriangleLinkView & triangles):
    m_entities (std::move(entities))
{
    m_triangles.
        insert(m_triangles.begin(), triangles.begin(), triangles.end());
}

void TeardownTask::operator () (Callbacks & callbacks) const {
    for (auto ent : m_entities)
        { ent.request_deletion(); }
    for (auto & triptr : m_triangles)
        { callbacks.remove(triptr); }
}
#endif
