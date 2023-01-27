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

#include "TileSet.hpp"
#include "TileSetPropertiesGrid.hpp"
#include "slopes-group-filler.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::exceptions_abbr;

} // end of <anonymous> namespace

/* static */ const TileSet::FillerFactoryMap & TileSet::builtin_fillers() {
    static auto s_map = [] {
        FillerFactoryMap s_map;
        for (auto type : slopes_group_filler_type_names::k_ramp_group_type_list) {
            s_map[type] = SlopeGroupFiller_::make;
        }
        return s_map;
    } ();
    return s_map;
}

/* static */ Vector2I TileSet::tid_to_tileset_location
    (const Size2I & sz, int tid)
    { return Vector2I{tid % sz.width, tid / sz.width}; }

void TileSet::load
    (Platform & platform, const TiXmlElement & tileset,
     const FillerFactoryMap & filler_factories)
{
    // tileset loc,

    std::vector<Tuple<Vector2I, FillerFactory>> factory_grid_positions;

    TileSetXmlGrid xml_grid;
    xml_grid.load(platform, tileset);
    {
    factory_grid_positions.reserve(xml_grid.size());
    for (Vector2I r; r != xml_grid.end_position(); r = xml_grid.next(r)) {
        const auto & el = xml_grid(r);
        if (el.is_empty()) continue;
        auto & type = el.type();
        auto itr = filler_factories.find(type);
        if (itr == filler_factories.end()) {
            // warn maybe, but don't error
            continue;
        }
        if (!itr->second) {
            throw InvArg{"TileSetN::load: no filler factory maybe nullptr"};
        }
        factory_grid_positions.emplace_back(r, itr->second);
    }
    }

    std::vector<Tuple<FillerFactory, std::vector<Vector2I>>> factory_and_locations;
    {
    using Tup = Tuple<Vector2I, FillerFactory>;
    auto comp = [](const Tup & lhs, const Tup & rhs) {
        return std::less<FillerFactory>{}
            ( std::get<FillerFactory>(lhs), std::get<FillerFactory>(rhs) );
    };
    std::sort
        (factory_grid_positions.begin(), factory_grid_positions.end(),
         comp);
    FillerFactory filler_factory = nullptr;
    for (auto [r, factory] : factory_grid_positions) {
        if (filler_factory != factory) {
            factory_and_locations.emplace_back(factory, std::vector<Vector2I>{});
            filler_factory = factory;
        }
        std::get<std::vector<Vector2I>>(factory_and_locations.back()).push_back(r);
    }
    }

    Grid<SharedPtr<ProducableTileFiller>> filler_grid;
    filler_grid.set_size(xml_grid.size2(), nullptr);
    for (auto [factory, locations] : factory_and_locations) {
        auto filler = (*factory)(xml_grid, platform);
        for (auto r : locations) {
            filler_grid(r) = filler;
        }
    }
    m_filler_grid = std::move(filler_grid);
}

SharedPtr<ProducableTileFiller> TileSet::find_filler(int tid) const
    { return find_filler( tile_id_to_tileset_location(tid) ); }

/* private */ SharedPtr<ProducableTileFiller> TileSet::find_filler
    (Vector2I r) const
    { return m_filler_grid(r); }

Vector2I TileSet::tile_id_to_tileset_location(int tid) const
    { return tid_to_tileset_location(m_filler_grid, tid); }
