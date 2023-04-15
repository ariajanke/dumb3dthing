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

#include "../../Defs.hpp"
#include "../ProducableGrid.hpp"

class TileSet;
class ProducableGroupFiller;

/// Translates global ids to tileset ids, along with their tilesets
///
/// Can also be used as an owner for tilesets (needs to for translation to
/// work). The tilesets maybe moved out, however this empties the translator.
///   TileMapIdToSetMapping
///   MapToSetIdMapping
class TileMapIdToSetMapping final :
    public UnfinishedProducableTileViewGrid::ProducableGroupFillerExtraction
{
public:
    using ConstTileSetPtr = SharedPtr<const TileSet>;
    using TileSetPtr      = SharedPtr<TileSet>;

    TileMapIdToSetMapping() {}

    TileMapIdToSetMapping(const std::vector<TileSetPtr> & tilesets,
                          const std::vector<int> & startgids);

    Tuple<int, ConstTileSetPtr> map_id_to_set(int map_wide_id) const;

    [[nodiscard]] std::vector<SharedPtr<TileSet>> move_out_tilesets();

    [[nodiscard]] std::vector<SharedPtr<const ProducableGroupFiller>>
        move_out_fillers() final;

    void swap(TileMapIdToSetMapping &);

private:
    struct GidAndTileSetPtr final {
        GidAndTileSetPtr() {}

        GidAndTileSetPtr(int sid, const TileSetPtr & ptr):
            starting_id(sid), tileset(ptr) {}

        int starting_id = 0;
        TileSetPtr tileset;
    };

    static bool order_by_gids(const GidAndTileSetPtr &, const GidAndTileSetPtr &);

    std::vector<GidAndTileSetPtr> m_gid_map;
    int m_gid_end = 0;
};
