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

#include <unordered_set>

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

struct ForEachTileOnEdge final {};

describe<ForEachTileOnEdge>("for_each_tile_on_edge")([] {
    using Vector2ISet = std::unordered_set<Vector2I, Vector2IHasher>;
    static auto make_remover = [] (Vector2ISet & set) {
        return [&set](int x, int y) { set.erase(Vector2I{x, y}); };
    };
    mark_it("runs left side of a rectangle", [] {
        Vector2ISet expected_pos
            { Vector2I{1, 2}, Vector2I{1, 3}, Vector2I{1, 4} };
        for_each_tile_on_edge
            (RectangleI{1, 2, 3, 3}, RegionSide::left, make_remover(expected_pos));
        return test_that(expected_pos.empty());
    }).
    mark_it("runs right side of a rectangle", [] {
        Vector2ISet expected_pos
            { Vector2I{4, 1}, Vector2I{4, 2}, Vector2I{4, 3} };
        for_each_tile_on_edge
            (RectangleI{2, 1, 3, 3}, RegionSide::right, make_remover(expected_pos));
        return test_that(expected_pos.empty());
    }).
    mark_it("runs top side of a rectangle", [] {
        Vector2ISet expected_pos
            { Vector2I{1, 2}, Vector2I{2, 2}, Vector2I{3, 2} };
        for_each_tile_on_edge
            (RectangleI{1, 2, 3, 3}, RegionSide::top, make_remover(expected_pos));
        return test_that(expected_pos.empty());
    }).
    mark_it("runs bottom side of a rectangle", [] {
        Vector2ISet expected_pos
            { Vector2I{0, 3}, Vector2I{1, 3}, Vector2I{2, 3} };
        for_each_tile_on_edge
            (RectangleI{0, 1, 3, 3}, RegionSide::bottom, make_remover(expected_pos));
        return test_that(expected_pos.empty());
    });
});

describe<RegionAxisLinkEntry>("RegionAxisLinkEntry::computed_bounds")([] {
    auto sample_link = make_shared<TriangleLink>
        (Vector{0, 0, 0}, Vector{-10, 100, 9.76}, Vector{5, 87.6, -400});
    mark_it("computes correct bounds on x-axis", [sample_link] {
        auto entry = RegionAxisLinkEntry::computed_bounds
            (sample_link, RegionAxis::x_ways);
        return test_that(are_very_close(entry.low_bounds (), -10) &&
                         are_very_close(entry.high_bounds(),   5)   );
    }).
    mark_it("computes correct bounds on z-axis", [sample_link] {
        auto entry = RegionAxisLinkEntry::computed_bounds
            (sample_link, RegionAxis::z_ways);
        return test_that(are_very_close(entry.low_bounds (), -400   ) &&
                         are_very_close(entry.high_bounds(),    9.76)   );
    });
});

describe<RegionAxisLinksAdder>("RegionAxisLinksAdder::dedupelicate").
    depends_on<RegionAxisLinkEntry>()([]
{
    mark_it("removes dupelicate links from the container", [] {
        auto a = make_shared<TriangleLink>();
        auto b = make_shared<TriangleLink>();
        std::vector<RegionAxisLinkEntry> entries;
        entries.emplace_back(a);
        entries.emplace_back(b);
        entries.emplace_back(a);
        assert(a.use_count() == 3);
        entries = RegionAxisLinksAdder::dedupelicate(std::move(entries));
        return test_that(a.use_count() == 2);
    });
});

describe<RegionAxisLinksAdder>("RegionAxisLinksAdder::sort_and_sweep").
    depends_on<RegionAxisLinkEntry>()([]
{
    // how to get certain overlaps?
    auto a = make_view_grid_for_tile(Vector2I{0, 0});
    auto b = make_view_grid_for_tile(Vector2I{0, 1});
    std::vector<RegionAxisLinkEntry> entries;
    for (auto link : { a.e, a.w, b.e, b.w }) {
        entries.emplace_back(RegionAxisLinkEntry::computed_bounds
            (link, RegionAxis::x_ways));
    }
    entries = RegionAxisLinksAdder::sort_and_sweep(std::move(entries));
    mark_it("links relavant triangles together", [&] {
        return test_that(a.e->transfers_to(TriangleSide::k_side_bc).target() == b.w);
    });
});

describe<RegionAxisAddressAndSide>("RegionAxisAddressAndSide::for_")([]
{
    using AddrAndSide = RegionAxisAddressAndSide;
    using Axis        = RegionAxis;
    using Side        = RegionSide;
    mark_it("contains correct north side", [] {
        auto res = AddrAndSide::for_(Vector2I{1, 1}, Size2I{1, 1});
        AddrAndSide expected{Axis::x_ways, 1, Side::top};
        return test_that(std::find(res.begin(), res.end(), expected));
    }).
    mark_it("contains correct south side", [] {
        auto res = AddrAndSide::for_(Vector2I{1, 1}, Size2I{1, 2});
        AddrAndSide expected{Axis::x_ways, 3, Side::bottom};
        return test_that(std::find(res.begin(), res.end(), expected));
    }).
    mark_it("contains correct west side", [] {
        auto res = AddrAndSide::for_(Vector2I{2, 1}, Size2I{1, 1});
        AddrAndSide expected{Axis::z_ways, 2, Side::left};
        return test_that(std::find(res.begin(), res.end(), expected));
    }).
    mark_it("contains correct east side", [] {
        auto res = AddrAndSide::for_(Vector2I{2, 1}, Size2I{3, 1});
        AddrAndSide expected{Axis::z_ways, 5, Side::right};
        return test_that(std::find(res.begin(), res.end(), expected));
    });
});

describe<RegionAxisLinksRemover>("RegionAxisLinksRemover::null_out_dupelicates").
    depends_on<RegionAxisLinkEntry>()([]
{
    using Entry = RegionAxisLinkEntry;
    using LinksRemover = RegionAxisLinksRemover;

    std::vector<Entry> entries;

    SharedPtr<TriangleLink> low_link, high_link;
    std::tie(low_link, high_link) = [] {
        auto link_a = make_shared<TriangleLink>();
        auto link_b = make_shared<TriangleLink>();
        auto link_low  = link_a < link_b ? link_a : link_b;
        auto link_high = link_a < link_b ? link_b : link_a;
        return make_tuple(link_low, link_high);
    } ();

    auto null_out_dupelicates = [&entries]
        { entries = LinksRemover::null_out_dupelicates(std::move(entries)); };

    mark_it("clears out dupelicate link", [&] {
        entries.push_back(Entry{high_link});
        entries.push_back(Entry{ low_link});
        entries.push_back(Entry{high_link});
        null_out_dupelicates();
        return test_that(high_link.use_count() == 1);
    }).
    mark_it("retains unique link", [&] {
        entries.push_back(Entry{high_link});
        entries.push_back(Entry{ low_link});
        entries.push_back(Entry{high_link});
        null_out_dupelicates();
        return test_that(low_link.use_count() == 2);
    }).
    mark_it("clears dupelicates at the beginning of container", [&] {
        entries.push_back(Entry{ low_link});
        entries.push_back(Entry{ low_link});
        entries.push_back(Entry{high_link});
        null_out_dupelicates();
        return test_that(low_link.use_count() == 1);
    });
});

describe<RegionAxisLinksRemover>("RegionAxisLinksRemover::remove_nulls").
    depends_on<RegionAxisLinkEntry>()([]
{
    using Entry = RegionAxisLinkEntry;
    using LinksRemover = RegionAxisLinksRemover;

    std::vector<Entry> entries;
    entries.emplace_back(make_shared<TriangleLink>());
    entries.emplace_back(nullptr);
    entries = LinksRemover::remove_nulls(std::move(entries));
    mark_it("reduces container to appropriate size", [&] {
        return test_that(entries.size() == 1);
    }).
    mark_it("remaining entries are not null", [&] {
        return test_that(!!entries[0].link());
    });
});

struct Whatevs final {};

describe<Whatevs>("RegionEdgeConnectionsContainer")([] {
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    auto samp_0_0_old = make_view_grid_for_tile(Vector2I{});
    auto samp_1_0     = make_view_grid_for_tile(Vector2I{1, 0});
    auto samp_0_0_new = make_view_grid_for_tile(Vector2I{});

    auto samp_0_0_old_ptr = make_shared<ViewGridTriangle>(samp_0_0_old.view_grid);
    auto samp_1_0_ptr     = make_shared<ViewGridTriangle>(samp_1_0    .view_grid);
    auto samp_0_0_new_ptr = make_shared<ViewGridTriangle>(samp_0_0_new.view_grid);

    auto adder_first = RegionEdgeConnectionsContainer{}.make_adder();
    RegionEdgeConnectionsContainer cont_first;
    mark_it("adding links to container, container owns the given links", [&] {
        auto usec00 = samp_0_0_old.e.use_count();
        auto usec10 = samp_1_0.e.use_count();
        adder_first.add(Vector2I{    }, samp_0_0_old_ptr);
        adder_first.add(Vector2I{1, 0}, samp_1_0_ptr    );
        cont_first = adder_first.finish();
        // triangles neighbor four axises with 1x1 grids
        return test_that(samp_0_0_old.e.use_count() - usec00 == 4 &&
                         samp_1_0    .e.use_count() - usec10 == 4   );
    });

});
#if 0
// can't depend_on multiple :c

describe<RegionEdgeConnectionsAdder>("RegionEdgeConnectionsAdder::ensure_entries_for_changes")([] {

});

describe<RegionEdgeConnectionsAdder>("RegionEdgeConnectionsAdder::apply_additions")([] {

});

describe<RegionEdgeConnectionsAdder>("RegionEdgeConnectionsAdder::finish_adders")([] {

});

describe<RegionEdgeConnectionsRemover>("RegionEdgeConnectionsRemover::apply_removals")([] {

});

describe<RegionEdgeConnectionsRemover>("RegionEdgeConnectionsRemover::finish_removers")([] {

});
#endif
return [] {};

} ();
