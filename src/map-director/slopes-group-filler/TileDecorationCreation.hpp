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

#include "../ProducableGrid.hpp"

class TileDecorationCreation final {
public:
    static Entity create_tile_decoration_with(ProducableTileCallbacks &);

    TileDecorationCreation(ProducableTileCallbacks &);

    Entity created_tile_decoration();

private:
    Entity make_grass();

    Entity make_tree();

    Real random_position() const;

    Real random_roll() const;

    Entity adjust_translation(Entity &&);

    ProducableTileCallbacks & m_callbacks;
    AssetsRetrieval & m_assets_retrieval;
    Optional<Vector> m_random_pt_in_tile;
};
