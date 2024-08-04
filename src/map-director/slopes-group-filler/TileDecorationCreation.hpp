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
    static void create_tile_decoration_with(ProducableTileCallbacks &);

    TileDecorationCreation(ProducableTileCallbacks &);

    void created_tile_decoration();

private:
    void make_grass();

    void make_tree();

    Real random_roll() const;

    ProducableTileCallbacks & m_callbacks;
};
