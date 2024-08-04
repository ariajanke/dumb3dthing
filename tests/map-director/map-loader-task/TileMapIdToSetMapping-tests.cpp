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

#include "../../../src/map-director/map-loader-task/TileMapIdToSetMapping.hpp"
#include "../../../src/map-director/map-loader-task/TilesetBase.hpp"

#include "../../test-helpers.hpp"

#include <tinyxml2.h>

#include <set>

namespace {

using namespace cul::tree_ts;

class TestTileset final : public TilesetBase {
public:
    Continuation & load
        (const DocumentOwningXmlElement &, MapContentLoader &) final
        { throw ""; }

    void add_map_elements
        (TilesetMapElementCollector &, const TilesetLayerWrapper &) const final
        { throw ""; }

private:
    Size2I size2() const final { return Size2I{1, 1}; }
};

using StartGidWithTileset = TileMapIdToSetMapping::StartGidWithTileset;

struct Info final {
    TilesetBase * tileset = nullptr;
    std::vector<TilesetMappingTile> mapping_tiles;
};

} // end of <anonymous> namespace

[[maybe_unused]] static auto s_add_describes = [] {

describe("TileMapIdToSetMapping")([] {
    auto a = make_shared<TestTileset>();
    auto b = make_shared<TestTileset>();
    std::vector tsids = { StartGidWithTileset{1, a}, StartGidWithTileset{2, b} };
    TileMapIdToSetMapping mapping{std::move(tsids)};
    Grid<int> g{ { 1, 2 }, { 1, 0 } };
    auto view = mapping.make_mapping_from_layer(GlobalIdTileLayer{std::move(g), MapElementProperties{}});
    std::vector<Info> infos;
    for (const TilesetLayerWrapper & layer : view) {
        Info info;
        info.tileset = TilesetMappingLayer::tileset_of(layer.as_view());
        for (const TilesetMappingTile & mapping_tile : layer.as_view()) {
            info.mapping_tiles.push_back(mapping_tile);
        }
        infos.push_back(info);
    }
    mark_it("makes n tileset mapping layers per n tilesets", [&] {
        return test_that(infos.size() == 2);
    }).
    mark_it("covers entire map", [&] {
        Grid<bool> coverage{ { false, false }, { false, false } };
        for (auto & info : infos) {
            for (auto & tile : info.mapping_tiles) {
                coverage(tile.on_map()) = true;
            }
        }
        static constexpr const Vector2I k_hole{1, 1};
        coverage(k_hole) = !coverage(k_hole);
        return test_that(std::all_of(coverage.begin(), coverage.end(), [](bool b) { return b; }));
    }).
    mark_it("tiles map to their respective tilesets", [&] {
        Grid<TilesetBase *> tilesets;
        tilesets.set_size(2, 2, nullptr);
        for (auto & info : infos) {
            for (auto & tile : info.mapping_tiles) {
                tilesets(tile.on_map()) = info.tileset;
            }
        }
        Grid<TilesetBase *> expected {
            { a.get(), b.get() },
            { a.get(), nullptr }
        };
        return test_that(std::equal(
            tilesets.begin(), tilesets.end(),
            expected.begin(), expected.end()
        ));
    });
});

return [] {};

} ();
