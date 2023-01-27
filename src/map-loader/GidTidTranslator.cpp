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

#include "GidTidTranslator.hpp"
#include "TileSet.hpp"

namespace {

using namespace cul::exceptions_abbr;
using ConstTileSetPtr = GidTidTranslator::ConstTileSetPtr;

} // end of <anonymous> namespace

GidTidTranslator::GidTidTranslator
    (const std::vector<TileSetPtr> & tilesets, const std::vector<int> & startgids)
{
    if (tilesets.size() != startgids.size()) {
        throw InvArg{"GidTidTranslator::GidTidTranslator: GidTidTranslator "
                     "constructor expects both passed containers to be equal "
                     "in size."};
    }
    m_gid_map.reserve(tilesets.size());
    for (std::size_t i = 0; i != tilesets.size(); ++i) {
        m_gid_map.emplace_back(startgids[i], tilesets[i]);
    }
    // this is what happens when you don't test your shit
    if (!startgids.empty()) {
        m_gid_end = startgids.back() + tilesets.back()->total_tile_count();
    }
    std::sort(m_gid_map.begin(), m_gid_map.end(), order_by_gids);
}

std::vector<SharedPtr<const TileSet>> GidTidTranslator::move_out_tilesets() {
    std::vector<SharedPtr<const TileSet>> rv;
    rv.reserve(m_gid_map.size());
    for (auto & tl : m_gid_map) {
        rv.emplace_back(std::move(tl.tileset));
    }
    m_gid_map.clear();
    m_gid_end = 0;
    return rv;
}

Tuple<int, ConstTileSetPtr> GidTidTranslator::gid_to_tid(int gid) const {
    if (gid == 0) {
        return std::make_pair(0, nullptr);
    }
    if (gid < 1 || gid >= m_gid_end) {
        throw InvArg{"GidTidTranslator::gid_to_tid: Given gid is either the "
                     "empty tile or not contained in this map; translatable "
                     "gids: [1 " + std::to_string(m_gid_end) + ")."};
    }
    GidAndTileSetPtr samp;
    samp.starting_id = gid;
    auto itr = std::upper_bound(m_gid_map.begin(), m_gid_map.end(), samp, order_by_gids);
    assert(itr != m_gid_map.begin());
#   if 0
    if (itr == m_gid_map.begin()) {
        throw RtError{"GidTidTranslator::gid_to_tid: Given gid is "};
    }
#   endif
    --itr;
    assert(gid >= itr->starting_id);
    return std::make_pair(gid - itr->starting_id, itr->tileset);
}

void GidTidTranslator::swap(GidTidTranslator & rhs) {
    m_gid_map.swap(rhs.m_gid_map);
    std::swap(m_gid_end, rhs.m_gid_end);
}

/* private static */ bool GidTidTranslator::order_by_gids
    (const GidAndTileSetPtr & lhs, const GidAndTileSetPtr & rhs)
{ return lhs.starting_id < rhs.starting_id; }
