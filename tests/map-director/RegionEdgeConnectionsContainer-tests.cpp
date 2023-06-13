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
#include "../../src/TriangleLink.hpp"

#include "../test-helpers.hpp"

struct Samp final {
    SharedPtr<TriangleLink> w;
    SharedPtr<TriangleLink> e;
    ViewGrid<SharedPtr<TriangleLink>> view_grid;
};

Samp make_view_grid_for_tile(const Vector2I & r) {
    auto link_w = make_shared<TriangleLink>
        (Vector{Real(r.x), 0., Real(r.y)},
         Vector{Real(r.x + 1), 0., Real(r.y)},
         Vector{Real(r.x), 0., Real(r.y + 1)});
    auto link_e = make_shared<TriangleLink>
        (Vector{Real(r.x + 1), 0., Real(r.y)},
         Vector{Real(r.x + 1), 0., Real(r.y + 1)},
         Vector{Real(r.x), 0., Real(r.y + 1)});
    ViewGridInserter<SharedPtr<TriangleLink>> grid_inserter{Size2I{1, 1}};
    grid_inserter.push(link_w);
    grid_inserter.push(link_e);
    grid_inserter.advance();
    Samp rv;
    rv.w = link_w;
    rv.e = link_e;
    rv.view_grid = grid_inserter.finish();
    return rv;
}

[[maybe_unused]] static auto s_add_describes = [] {

using namespace cul::tree_ts;

describe<RegionEdgeLinksContainer>("RegionEdgeLinksContainer")([] {
    auto a = make_view_grid_for_tile(Vector2I{0, 0});
    auto b = make_view_grid_for_tile(Vector2I{1, 0});
    RegionEdgeLinksContainer a_cont{a.view_grid};
    RegionEdgeLinksContainer b_cont{b.view_grid};
    a_cont.glue_to(b_cont);
    mark_it("glue together triangle links on connecting edge", [&] {
        auto ae_target = a.e->transfers_to(TriangleSegment::k_side_ab).target;
        return test_that(ae_target == b.w);
    }).
    mark_it("does not reglue together links if already attached", [&] {
        // do I really want to support this behavior?
        auto c = make_view_grid_for_tile(Vector2I{1, 0});
        RegionEdgeLinksContainer c_cont{c.view_grid};
        a_cont.glue_to(c_cont);
        auto ae_target = a.e->transfers_to(TriangleSegment::k_side_ab).target;
        return test_that(ae_target != c.w);
    }).
    mark_it("does not glue links, even if they can when not on opposite edges", [] {
        return test_that(false);
    });
});

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
    RegionEdgeConnectionEntry::Container entries;    
    entries.emplace_back(RegionSideAddress{RegionSide::left , 0}, nullptr);
    entries.emplace_back(RegionSideAddress{RegionSide::top  , 3}, nullptr);
    entries.emplace_back(RegionSideAddress{RegionSide::right, 1}, nullptr);
    mark_it("throws on unsorted container", [&] {
        return expect_exception<std::invalid_argument>([&] {
            RegionEdgeConnectionEntry::verify_sorted("test", std::move(entries));
        });
    }).
    next([&] {
        std::sort(entries.begin(), entries.end(), RegionEdgeConnectionEntry::less_than);
    }).
    mark_it("expects a sorted container", [&] {
        // unsorted container -> sort
        RegionEdgeConnectionEntry::verify_sorted("test", std::move(entries));
        return test_that(true);
    });
});

describe<RegionEdgeConnectionEntry>("RegionEdgeConnectionEntry::verify_no_bubbles")([] {
    RegionEdgeConnectionEntry::Container entries;
    entries.emplace_back(RegionSideAddress{RegionSide::left , 0}, make_shared<RegionEdgeLinksContainer>());
    entries.emplace_back(RegionSideAddress{RegionSide::top  , 3}, nullptr);
    entries.emplace_back(RegionSideAddress{RegionSide::right, 1}, make_shared<RegionEdgeLinksContainer>());
    mark_it("throws on bubbled container", [&] {
        return expect_exception<std::invalid_argument>([&] {
            RegionEdgeConnectionEntry::verify_no_bubbles("test", std::move(entries));
        });
    }).
    next([&] {
        auto has_no_link_container = []
            (const RegionEdgeConnectionEntry & entry)
            { return !!entry.link_container(); };
        auto new_end = std::remove_if
            (entries.begin(), entries.end(), has_no_link_container);
        entries.erase(new_end, entries.end());
    }).
    mark_it("expects a bubbleless container", [&] {
        RegionEdgeConnectionEntry::verify_no_bubbles("test", std::move(entries));
        return test_that(true);
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
