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

#include "../TriangleLink.hpp"
#include "SpatialPartitionMap.hpp"

class FrameTimeLinkContainer final {
public:
    using Iterator = ProjectedSpatialMap::Iterator;

    void defer_addition_of(const SharedPtr<const TriangleLink> &);

    void defer_removal_of(const SharedPtr<const TriangleLink> &);

    void update();

    View<Iterator> view_for(const Vector &, const Vector &) const;

    void clear();

private:
    bool is_dirty() const noexcept
        { return m_add_dirty || !m_to_remove_links.empty(); }

    std::vector<SharedPtr<const TriangleLink>> m_to_add_links;
    std::vector<SharedPtr<const TriangleLink>> m_to_remove_links;
    bool m_add_dirty = false;
    ProjectedSpatialMap m_spm;
};
