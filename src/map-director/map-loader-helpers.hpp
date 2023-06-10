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

#pragma once

#include "map-loader.hpp"
#include "ViewGrid.hpp"

#include "../Components.hpp"

#include <ariajanke/cul/SubGrid.hpp>
#if 0
class TeardownTask final : public LoaderTask {
public:
    // though... these are shared pointers to links
    // how do I not have a super long, however descriptive name?
    using ViewGridTriangle = ViewGrid<SharedPtr<TriangleLink>>;
    using ViewGridTriangleInserter = ViewGridTriangle::Inserter;
    using TriangleLinkView = ViewGridTriangle::ElementView;

    TeardownTask() {}

    TeardownTask
        (std::vector<Entity> && entities,
         const TriangleLinkView & triangles);

    void operator () (Callbacks &) const final;

private:
    std::vector<Entity> m_entities;
    std::vector<SharedPtr<TriangleLink>> m_triangles;
};
#endif

struct Vector2IHasher final {
    std::size_t operator () (const Vector2I & r) const {
        using IntHash = std::hash<int>;
        return IntHash{}(r.x) ^ IntHash{}(r.y);
    }
};
