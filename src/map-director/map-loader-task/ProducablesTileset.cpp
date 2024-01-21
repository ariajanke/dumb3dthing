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

#include "ProducablesTileset.hpp"
#include "../MapTileset.hpp"

#include "../slopes-group-filler.hpp"
#include "../twist-loop-filler.hpp"

#include "TileMapIdToSetMapping.hpp"

namespace {

using FillerFactory = ProducablesTileset::FillerFactory;
using FillerFactoryMap = ProducablesTileset::FillerFactoryMap;
using Continuation = BackgroundTask::Continuation;

struct MakeFillerGridRt final {
    Grid<SharedPtr<ProducableGroupFiller>> grid;
    std::vector<SharedPtr<const ProducableGroupFiller>> unique_fillers;
};

std::vector<Tuple<Vector2I, FillerFactory>>
    make_factory_grid_positions
    (const MapTileset & map_tileset, const FillerFactoryMap & filler_factories);

std::vector<Tuple<FillerFactory, std::vector<Vector2I>>>
    find_unique_factories_and_positions
    (std::vector<Tuple<Vector2I, FillerFactory>> && factory_grid_positions);

MakeFillerGridRt make_filler_grid
    (const std::vector<Tuple<FillerFactory, std::vector<Vector2I>>> & factory_and_locations,
     const MapTileset &,
     MapContentLoader & platform);

} // end of <anonymous> namespace

/* static */ const ProducablesTileset::FillerFactoryMap &
    ProducablesTileset::builtin_fillers()
{
    static auto s_map = [] {
        FillerFactoryMap s_map;
        for (auto type : slopes_group_filler_type_names::k_ramp_group_type_list) {
            s_map[type] = SlopeGroupFiller_::make;
        }
#       if 0 // disabled, feature not implemented
        for (auto type : twist_loop_filler_names::k_name_list) {
            s_map[type] = TwistLoopGroupFiller_::make;
        }
#       endif
        return s_map;
    } ();
    return s_map;
}

// four params, ouch!
Continuation & ProducablesTileset::load
    (const DocumentOwningXmlElement & tileset_el, MapContentLoader & content_loader)
{
    MapTileset map_tileset;
    map_tileset.load(tileset_el);
    auto factory_and_locations = find_unique_factories_and_positions
        (make_factory_grid_positions(map_tileset, content_loader.map_fillers()));
    auto filler_things = make_filler_grid
        (factory_and_locations, map_tileset, content_loader);
    m_filler_grid = std::move(filler_things.grid);
    m_unique_fillers = std::move(filler_things.unique_fillers);

    return content_loader.task_continuation();
}

/* private */
std::map<
    SharedPtr<ProducableGroupFiller>,
    std::vector<TileLocation>>
    ProducablesTileset::make_fillers_and_locations
    (const TilesetLayerWrapper & tile_layer_wrapper) const
{
    std::map<
        SharedPtr<ProducableGroupFiller>,
        std::vector<TileLocation>> rv;
    for (auto & location : tile_layer_wrapper) {
        // more cleaning out can be done here I think
        if (!m_filler_grid(location.on_tile_set())) {
            continue;
        }

        rv[m_filler_grid(location.on_tile_set())].
            push_back(location.to_tile_location());
    }
    return rv;
}

void ProducablesTileset::add_map_elements
    (TilesetMapElementCollector & collector,
     const TilesetLayerWrapper & mapping_view) const
{
    using CallbackWithCreator = ProducableGroupFiller::CallbackWithCreator;
    using ProducableGroupCreation = ProducableGroupFiller::ProducableGroupCreation;
    auto fillers_to_locs = make_fillers_and_locations(mapping_view);
    Grid<ProducableTile *> producables;
    std::vector<SharedPtr<ProducableGroupOwner>> owners;
    producables.set_size(mapping_view.grid_size(), nullptr);
    for (auto & pair : fillers_to_locs) {
        auto & filler = pair.first;
#       if MACRO_DEBUG
        assert(filler);
#       endif
        auto & locs = pair.second;

        auto creator = CallbackWithCreator::make([&] (ProducableGroupCreation & creation) {
            creation.reserve(locs.size(), mapping_view.grid_size());
            for (auto & loc : locs) {
                producables(loc.on_map) = &creation.add_member(loc);
            }
            owners.emplace_back(creation.finish());
        });
        filler->make_group(creator);
    }

    collector.add(std::move(producables), std::move(owners));
}

namespace {


std::vector<Tuple<Vector2I, FillerFactory>>
    make_factory_grid_positions
    (const MapTileset & map_tileset, const FillerFactoryMap & filler_factories)
{
    std::vector<Tuple<Vector2I, FillerFactory>> factory_grid_positions;
    factory_grid_positions.reserve(map_tileset.tile_count());
    for (Vector2I r; r != map_tileset.end_position(); r = map_tileset.next(r)) {
        const auto * el = map_tileset.tile_at(r);
        if (!el)
            { continue; }
        const auto * type = el->type();
        auto itr = filler_factories.find(type);
        if (itr == filler_factories.end()) {
            // warn maybe, but don't error
            continue;
        }
        if (!itr->second) {
            throw InvalidArgument
                {"TileSet::load: no filler factory maybe nullptr"};
        }
        factory_grid_positions.emplace_back(r, itr->second);
    }
    return factory_grid_positions;
}

std::vector<Tuple<FillerFactory, std::vector<Vector2I>>>
    find_unique_factories_and_positions
    (std::vector<Tuple<Vector2I, FillerFactory>> && factory_grid_positions)
{
    using Tup = Tuple<Vector2I, FillerFactory>;
    std::vector<Tuple<FillerFactory, std::vector<Vector2I>>> factory_and_locations;
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
    return factory_and_locations;
}

MakeFillerGridRt make_filler_grid
    (const std::vector<Tuple<FillerFactory, std::vector<Vector2I>>> & factory_and_locations,
     const MapTileset & map_tileset,
     MapContentLoader & platform)
{
    MakeFillerGridRt rv;
    auto & filler_grid = rv.grid;
    auto & filler_instances = rv.unique_fillers;
    filler_grid.set_size(map_tileset.size2(), nullptr);
    for (auto & [factory, locations] : factory_and_locations) {
        auto filler = (*factory)(map_tileset, platform);
        filler_instances.emplace_back(filler);
        for (auto r : locations) {
            filler_grid(r) = filler;
        }
    }
    return rv;
}

} // end of <anonymous> namespace
