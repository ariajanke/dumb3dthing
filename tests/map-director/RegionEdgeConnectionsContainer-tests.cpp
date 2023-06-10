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

#include "../../src/map-director/RegionEdgeConnectionsContainer.hpp"

#include "../test-helpers.hpp"


[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe<RegionSideAddress>("RegionSideAddress::compare")([] {
    mark_it("compares less than", [] {
        return test_that(false);
    }).
    mark_it("compares greater than", [] {
        return test_that(false);
    }).
    mark_it("compares equal to", [] {
        return test_that(false);
    });
});

describe<RegionEdgeConnectionEntry>("RegionEdgeConnectionEntry::seek")([] {
    mark_it("finds nothing in not in sequence", [] {
        return test_that(false);
    }).
    mark_it("finds target entry if in sequence", [] {
        return test_that(false);
    });
});

describe<RegionEdgeConnectionEntry>("RegionEdgeConnectionEntry::verify_sorted")([] {
    mark_it("throws on unsorted container", [] {
        return test_that(false);
    }).
    next([] {
        ;
    }).
    mark_it("expects a sorted container", [] {
        // unsorted container -> sort
        return test_that(false);
    });
});

describe<RegionEdgeConnectionEntry>("RegionEdgeConnectionEntry::verify_no_bubbles")([] {
    mark_it("throws on bubbled container", [] {
        return test_that(false);
    }).
    next([] {
        ;
    }).
    mark_it("expects a bubbleless container", [] {
        return test_that(false);
    });
});

describe<RegionEdgeConnectionEntry>("RegionEdgeConnectionEntry::less_than") ([] {
    mark_it("sorted container with less_than works with seek", [] {
        // unsorted container -> sort -> seek
        return test_that(false);
    });
});

describe<RegionEdgeConnectionsAdder>("RegionEdgeConnectionsAdder")([] {
    mark_it("sorts out of order entries", [] {
        return test_that(false);
    });
});


describe<RegionEdgeConnectionsAdder>("RegionEdgeConnectionsRemover")([] {
    mark_it("removes bubbles in container", [] {
        return test_that(false);
    });
});

return [] {};

} ();
