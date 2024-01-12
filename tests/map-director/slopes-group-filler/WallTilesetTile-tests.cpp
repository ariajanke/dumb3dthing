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

#include <tinyxml2.h>

namespace {

using namespace cul::tree_ts;

class TestGeometryGenerationStrategy final :
    public SplitWallGeometry::GeometryGenerationStrategy
{
public:
    using WithSplitWallGeometry = SplitWallGeometry::WithSplitWallGeometry;

    static GeometryGenerationStrategy & instance_for(CardinalDirection dir) {
        s_dir = dir;
        return instance();
    }

    static CardinalDirection direction_received_for_strategy() {
        return s_dir;
    }

    static GeometryGenerationStrategy & instance() {
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
        (TileCornerElevations) const final
    {
        ;
    }

private:
    static CardinalDirection s_dir;
};

/* static */ CardinalDirection TestGeometryGenerationStrategy::s_dir;

} // end of <anonymous> namespace

auto x = [] {

describe("WallTilesetTile#load")([] {
    TestGeometryGenerationStrategy::instance() = TestGeometryGenerationStrategy{};
    WallTilesetTile wtt{TestGeometryGenerationStrategy::instance_for};
    MapTilesetTile mtt;
    // mtt.
    // wtt.load()

});

return [] {};
} ();
