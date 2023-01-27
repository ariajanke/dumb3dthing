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

#include "ParseHelpers.hpp"
#include "ProducableTileFiller.hpp"

#include <map>

/// Tilesets map tileset ids to tile group fillers.
///
/// maybe a loader thing
class TileSet final {
public:
    using FillerFactory = SharedPtr<ProducableTileFiller>(*)(const TileSetXmlGrid &, Platform &);
    using FillerFactoryMap = std::map<std::string, FillerFactory>;
    using TileLocation = ProducableTileFiller::TileLocation;

    static const FillerFactoryMap & builtin_fillers();

    static Vector2I tid_to_tileset_location(const Size2I &, int tid);

    template <typename T>
    static Vector2I tid_to_tileset_location(const Grid<T> & grid, int tid)
        { return tid_to_tileset_location(grid.size2(), tid); }

    void load(Platform &, const TiXmlElement &,
              const FillerFactoryMap & = builtin_fillers());

    SharedPtr<ProducableTileFiller> find_filler(int tid) const;

    Vector2I tile_id_to_tileset_location(int tid) const;

    auto total_tile_count() const
        { return m_filler_grid.size(); }

private:
    SharedPtr<ProducableTileFiller> find_filler(Vector2I) const;

    Grid<SharedPtr<ProducableTileFiller>> m_filler_grid;
};
