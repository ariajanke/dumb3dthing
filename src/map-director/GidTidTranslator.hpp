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

#include "../Defs.hpp"

class TileSet;

/// Translates global ids to tileset ids, along with their tilesets
///
/// Can also be used as an owner for tilesets (needs to for translation to
/// work). The tilesets maybe moved out, however this empties the translator.
class GidTidTranslator final {
public:
    using ConstTileSetPtr = SharedPtr<const TileSet>;
    using TileSetPtr      = SharedPtr<TileSet>;

    GidTidTranslator() {}

    GidTidTranslator(const std::vector<TileSetPtr> & tilesets,
                     const std::vector<int> & startgids);

    Tuple<int, ConstTileSetPtr> gid_to_tid(int gid) const;

    std::vector<SharedPtr<const TileSet>> move_out_tilesets();

    void swap(GidTidTranslator &);

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
