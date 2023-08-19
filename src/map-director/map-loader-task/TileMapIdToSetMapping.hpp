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

#include "../../Definitions.hpp"
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

    struct TileSetAndStartGid final {
        TileSetAndStartGid() {}

        TileSetAndStartGid(TileSetPtr && tileset_, int start_gid_):
            tileset(std::move(tileset_)),
            start_gid(start_gid_)
        {}

        TileSetPtr tileset;
        int start_gid = 0;
    };

    TileMapIdToSetMapping() {}

    explicit TileMapIdToSetMapping(std::vector<TileSetAndStartGid> &&);

    Tuple<int, ConstTileSetPtr> map_id_to_set(int map_wide_id) const;

    [[nodiscard]] std::vector<SharedPtr<const ProducableGroupFiller>>
        move_out_fillers() final;

    void swap(TileMapIdToSetMapping &);

private:
    static bool order_by_gids(const TileSetAndStartGid &, const TileSetAndStartGid &);

    [[nodiscard]] std::vector<SharedPtr<TileSet>> move_out_tilesets();

    std::vector<TileSetAndStartGid> m_gid_map;
    int m_gid_end = 0;
};
