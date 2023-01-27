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

#include "../Components.hpp"
#include "TileFactory.hpp"
#include "TileSet.hpp"
#include "ViewGrid.hpp"

#include <ariajanke/cul/SubGrid.hpp>

class TeardownTask final : public LoaderTask {
public:
    TeardownTask() {}

    explicit TeardownTask
        (std::vector<Entity> && entities,
         std::vector<SharedPtr<TriangleLink>> && triangles):
        m_entities (std::move(entities )),
        m_triangles(std::move(triangles)) {}

    void operator () (Callbacks &) const final;

private:
    std::vector<Entity> m_entities;
    std::vector<SharedPtr<TriangleLink>> m_triangles;
};

/// container of triangle links, used to glue segment triangles together
class InterTriangleLinkContainer final {
public:
    using Iterator = std::vector<SharedPtr<TriangleLink>>::iterator;
    using GridOfViews = Grid<View<TriangleLinks::const_iterator>>;

    InterTriangleLinkContainer() {}

    explicit InterTriangleLinkContainer(const GridOfViews & views);

    void glue_to(InterTriangleLinkContainer & rhs);

private:
    static bool is_edge_tile(const GridOfViews & grid, const Vector2I & r);

    static bool is_not_edge_tile(const GridOfViews & grid, const Vector2I & r);

    template <bool (*meets_pred)(const GridOfViews &, const Vector2I &)>
    static void append_links_by_predicate
        (const GridOfViews & views, std::vector<SharedPtr<TriangleLink>> & links);

    Iterator edge_begin() { return m_edge_begin; }

    Iterator edge_end() { return m_links.end(); }

    std::vector<SharedPtr<TriangleLink>> m_links;
    Iterator m_edge_begin;
};

struct Vector2IHasher final {
    std::size_t operator () (const Vector2I & r) const {
        using IntHash = std::hash<int>;
        return IntHash{}(r.x) ^ IntHash{}(r.y);
    }
};
