/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "../../Definitions.hpp"
#include "../MapElementValuesMap.hpp"

class GlobalIdTileLayer final {
public:
    GlobalIdTileLayer(Grid<int> && gids, MapElementProperties && elprops):
        m_gids(std::move(gids)),
        m_layer_elements(std::move(elprops)) {}

    auto size2() const { return m_gids.size2(); }

    int gid_at(const Vector2I &r) const { return m_gids(r); }

    [[nodiscard]] MapElementProperties move_out_layer_properties()
        { return std::move(m_layer_elements); }

private:
    Grid<int> m_gids;
    MapElementProperties m_layer_elements;
};
