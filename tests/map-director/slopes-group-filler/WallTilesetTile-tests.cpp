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

#include "../../../src/map-director/slopes-group-filler/WallTilesetTile.hpp"
#include "../../../src/map-director/MapTileset.hpp"
#include "../../test-helpers.hpp"
#include "../../RenderModel.hpp"

#include <tinyxml2.h>

namespace {

using namespace cul::tree_ts;

class TestGeometryGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    using WithSplitWallGeometry = SplitWallGeometry::WithSplitWallGeometry;

    static GeometryGenerationStrategy & instance_for(CardinalDirection dir) {
        if (s_choosen_direction.has_value()) {
            throw RuntimeError{"needs to be reset"};
        }
        s_choosen_direction = dir;
        return instance();
    }

    static void reset() {
        s_choosen_direction = {};
    }

    static Optional<CardinalDirection> choosen_direction() {
        return s_choosen_direction;
    }

    static CardinalDirection direction_received_for_strategy() {
        return s_dir;
    }

    static TestGeometryGenerationStrategy & instance() {
        static TestGeometryGenerationStrategy impl;
        return impl;
    }

    void with_splitter_do
        (const TileCornerElevations & elevations,
         Real division_z,
         const WithSplitWallGeometry &) const final
    {
        ;
    }

    TileCornerElevations filter_to_known_corners
        (TileCornerElevations elvs) const final
    {
        m_filtered_elevations.push_back(elvs);
        return elvs;
    }

    TileCornerElevations filtered_elevations_at(int n) const {
        return m_filtered_elevations.at(std::size_t(n));
    }

private:
    static CardinalDirection s_dir;
    static Optional<CardinalDirection> s_choosen_direction;

    mutable std::vector<TileCornerElevations> m_filtered_elevations;
};

/* static */ CardinalDirection TestGeometryGenerationStrategy::s_dir;
/* static */ Optional<CardinalDirection> TestGeometryGenerationStrategy::
    s_choosen_direction = {};

constexpr const auto k_something =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<properties>"
        "<property name=\"direction\" value=\"nw\" />"
        "<property name=\"elevation\" value=\"2\" />"
    "</properties>";

// How do I setup abstract classes for tests?
// redefining them everytime seems tedious
// There's got to be a less painful way to do it.

// remember... always do things iteratively
// there's no way I'm going to be able to create some awesome sauce testing
// framework from scratch right away
//
// I need an application where it's used first. It's also a motivational thing
// too. Why bother trying to create something in a vacuum that'll just make me
// feel empty and null? No, practice in conjunction with what I want.
//
// Google testing framework looks promising. But do I really want to use it?
// Seems there's no real escape from these mega corpos. Even this project's
// hosting is Microsoft's crap. Though it doesn't cross the line, as there's
// a difference between hosting and something that contaminates project source
// code. An odd thing to say about a broken, ugly, poorly aging language like
// C++.

class SingleResponseAssetsStrategy final : public PlatformAssetsStrategy {
public:
    static SingleResponseAssetsStrategy & instance() {
        static SingleResponseAssetsStrategy inst;
        return inst;
    }

    SharedPtr<RenderModel> nth_made_render_model(int i) const {
        return m_render_models.at(std::size_t(i));
    }

    SharedPtr<Texture> make_texture() const final {
        throw "";
    }

    SharedPtr<RenderModel> make_render_model() const final {
        m_render_models.push_back(make_shared<TestRenderModel>());
        return m_render_models.back();
    }

    FutureStringPtr promise_file_contents(const char *) final {
        throw "";
    }

private:
    mutable std::vector<SharedPtr<RenderModel>> m_render_models;
};

} // end of <anonymous> namespace

auto x = [] {

describe("WallTilesetTile#load")([] {
    auto & geo_strat =
        TestGeometryGenerationStrategy::instance() =
        TestGeometryGenerationStrategy{};
    WallTilesetTile wtt{TestGeometryGenerationStrategy::instance_for};
    MapTileset mt;
    MapTilesetTile mtt;
    mtt.load(**DocumentOwningXmlElement::load_from_contents(k_something));
    TilesetTileTexture ttt;
    auto & assets_strat = SingleResponseAssetsStrategy::instance();
    wtt.load(mtt, ttt, assets_strat);
    mark_it("chooses a direction based on data", [&] {
        return test_that
            (TestGeometryGenerationStrategy::choosen_direction().
             value_or(CardinalDirection::north) ==
             CardinalDirection::north_west);
    }).
    mark_it("loads and filters elevation correctly", [&] {
        return test_that
            (geo_strat.filtered_elevations_at(0) ==
             TileCornerElevations{2, 2, 2, 2});
    }).
    mark_it("makes and loads a render model", [&] {
        return test_that(assets_strat.nth_made_render_model(0)->is_loaded());
    });
});

return [] {};
} ();
