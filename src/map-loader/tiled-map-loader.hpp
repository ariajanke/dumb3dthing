/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "../platform.hpp"
#include "../Components.hpp"
#include "TileSet.hpp"
#include "ParseHelpers.hpp"

#include <ariajanke/cul/StringUtil.hpp>

class MapEdgeLinks final {
public:

    class TileRange final {
    public:
        constexpr TileRange () {}

        constexpr TileRange(Vector2I beg_, Vector2I end_):
            m_begin(beg_), m_end(end_) {}

        constexpr Vector2I begin_location() const { return m_begin; }

        constexpr Vector2I end_location() const { return m_end; }

        constexpr TileRange displace(Vector2I r) const
            { return TileRange{m_begin + r, m_end + r}; }

        constexpr bool operator == (const TileRange & rhs) const
            { return is_same_as(rhs); }

        constexpr bool operator != (const TileRange & rhs) const
            { return is_same_as(rhs); }

    private:
        constexpr bool is_same_as(const TileRange & rhs) const
            { return rhs.m_begin == m_begin && rhs.m_end == m_end; }

        Vector2I m_begin, m_end;
    };

    struct MapLinks final {
        std::string filename;
        TileRange range;
    };

    using LinksView = View<const MapLinks *>;
    enum class Side { north, south, east, west };

    MapEdgeLinks() { m_views.fill(make_tuple(k_uninit, k_uninit)); }

    LinksView north_links() const { return links(Side::north); }

    LinksView south_links() const { return links(Side::south); }

    LinksView east_links() const { return links(Side::east); }

    LinksView west_links() const { return links(Side::west); }

    void set_side(Side side, const std::vector<MapLinks> & links) {
        auto idx = static_cast<std::size_t>(side);
        auto & [b, e] = m_views[idx]; {}
        b = m_links.size();
        m_links.insert(m_links.end(), links.begin(), links.end());
        e = m_links.size();
    }

private:
    static constexpr const std::size_t k_uninit = -1;

    LinksView links(Side side) const {
        auto [b, e] = m_views[static_cast<std::size_t>(side)]; {}
        if (b == k_uninit || m_links.empty()) {
            throw std::runtime_error{"Side was not set."};
        }
        auto fptr = &m_links.front();
        return LinksView{fptr + b, fptr + e};
    }

    std::vector<MapLinks> m_links;
    std::array<Tuple<std::size_t, std::size_t>, 4> m_views;
};

class MapLoader final {
public:
    class TeardownTask final : public OccasionalTask {
    public:
        TeardownTask() {}

        explicit TeardownTask(std::vector<Entity> && entities):
            m_entities(std::move(entities)) {}

        void on_occasion(Callbacks &) final {
            for (auto & ent : m_entities)
                { ent.request_deletion(); }
        }

    private:
        std::vector<Entity> m_entities;
    };

    explicit MapLoader(Platform & platform):
        m_platform(&platform) {}

    Tuple<SharedPtr<LoaderTask>, SharedPtr<TeardownTask>> operator ()
        (const Vector2I & offset);

    void start_preparing(const char * filename)
        { m_file_contents = m_platform->promise_file_contents(filename); }

    // whooo accessors and further methods

    int width() const noexcept // not in tiles, but ues, implementation and interface happen to match for now
        { return m_layer.width(); }

    int height() const noexcept
        { return m_layer.height(); }

    auto northern_maps() const { return m_links.north_links(); }

    // names?

    auto southern_maps() const { return m_links.south_links(); }

    auto eastern_maps() const { return m_links.east_links(); }

    auto western_maps() const { return m_links.west_links(); }

private:
    void add_tileset(const TiXmlElement & tileset);

    bool check_has_remaining_pending_tilesets();

    void do_content_load(std::string && contents);

    FutureStringPtr m_file_contents;

    Grid<int> m_layer;
    Platform * m_platform;

    std::vector<SharedPtr<TileSet>> m_tilesets;
    std::vector<int> m_startgids;

    std::vector<Tuple<std::size_t, FutureStringPtr>> m_pending_tilesets;
    GidTidTranslator m_tidgid_translator;

    MapEdgeLinks m_links;
};

inline MapEdgeLinks::TileRange operator + (const MapEdgeLinks::TileRange & lhs, Vector2I rhs)
    { return lhs.displace(rhs); }

inline MapEdgeLinks::TileRange operator + (Vector2I rhs, const MapEdgeLinks::TileRange & lhs)
    { return lhs.displace(rhs); }

// ----------------------------------------------------------------------------

class MapSegmentContainer;

class MapLoaderN final {
public:
    using Rectangle = cul::Rectangle<int>;

    class TeardownTask final : public OccasionalTask {
    public:
        TeardownTask() {}

        explicit TeardownTask(std::vector<Entity> && entities):
            m_entities(std::move(entities)) {}

        void on_occasion(Callbacks &) final {
            for (auto & ent : m_entities)
                { ent.request_deletion(); }
        }

    private:
        std::vector<Entity> m_entities;
    };

    class StateHolder;

    class State {
    public:
        virtual ~State() {}

        virtual SharedPtr<LoaderTask> update_progress(StateHolder &)
            { return nullptr; }

        State & set_others_stuff(State & lhs) const {
            lhs.m_platform = m_platform;
            lhs.m_target_container = m_target_container;
            lhs.m_offset = m_offset;
            lhs.m_tiles_to_load = m_tiles_to_load;
            return lhs;
        }

    protected:
        struct TileSetsContainer final {
            std::vector<int> startgids;
            std::vector<SharedPtr<TileSet>> tilesets;
            std::vector<Tuple<std::size_t, FutureStringPtr>> pending_tilesets;
        };

        State() {}

        State
            (Platform & platform, MapSegmentContainer & target_container,
             const Vector2I & offset, const Rectangle & tiles_to_load):
            m_platform(&platform),
            m_target_container(&target_container),
            m_offset(offset),
            m_tiles_to_load(tiles_to_load) {}

        Platform & platform() const {
            verify_shared_set();
            return *m_platform;
        }

        MapSegmentContainer & target() {
            verify_shared_set();
            return *m_target_container;
        }

        Vector2I map_offset() const {
            verify_shared_set();
            return m_offset;
        }

        Rectangle target_tile_range() const {
            verify_shared_set();
            return m_tiles_to_load;
        }

    private:
        void verify_shared_set() const {
            using namespace cul::exceptions_abbr;
            if (m_platform) return;
            throw RtError{"Unset stuff"};
        }

        Platform * m_platform = nullptr;
        MapSegmentContainer * m_target_container = nullptr;
        Vector2I m_offset;
        Rectangle m_tiles_to_load;
    };

    class WaitingForFileContents final : public State {
    public:
        WaitingForFileContents
            (Platform & platform, MapSegmentContainer & target_container,
             const char * filename, const Vector2I & offset,
             const Rectangle & tiles_to_load):
             State(platform, target_container, offset, tiles_to_load)
        { m_file_contents = platform.promise_file_contents(filename); }

        SharedPtr<LoaderTask> update_progress(StateHolder & next_state) final;

    private:
        void add_tileset(const TiXmlElement & tileset, TileSetsContainer &);

        FutureStringPtr m_file_contents;
    };

    class WaitingForTileSets final : public State {
    public:
        WaitingForTileSets
            (TileSetsContainer && cont_, Grid<int> && layer_):
            m_tilesets_container(std::move(cont_)),
            m_layer(std::move(layer_)) {}

        SharedPtr<LoaderTask> update_progress(StateHolder & next_state) final;

    private:
        TileSetsContainer m_tilesets_container;
        Grid<int> m_layer;
        GidTidTranslator m_tidgid_translator;
    };

    class Ready final : public State {
    public:
        Ready(GidTidTranslator && idtrans_, Grid<int> && layer_):
            m_tidgid_translator(std::move(idtrans_)),
            m_layer(std::move(layer_)) {}

        SharedPtr<LoaderTask> update_progress(StateHolder & next_state) final;

    private:
        GidTidTranslator m_tidgid_translator;
        Grid<int> m_layer;
    };

    class Expired final : public State {
    public:
        Expired() {}
    };

    using StateSpace = Variant
        <WaitingForFileContents, WaitingForTileSets, Ready, Expired>;

    class StateHolder final {
    public:
        template <typename NextState, typename ... Types>
        NextState & set_next_state(Types && ...args) {
            m_space = NextState{std::forward<Types>(args)...};
            m_get_state = [](StateSpace & space) -> State *
                { return &std::get<NextState>(space); };
            return std::get<NextState>(m_space);
        }

        bool has_next_state() const noexcept
            { return m_get_state; }

        void move_state(State *& state_ptr, StateSpace & space) {
            using namespace cul::exceptions_abbr;
            if (!m_get_state) {
                throw RtError{"No state to move"};
            }

            space = std::move(m_space);
            state_ptr = m_get_state(space);
            m_get_state = nullptr;
        }

    private:
        using StatePtrGetter = State * (*)(StateSpace &);
        StatePtrGetter m_get_state = nullptr;
        StateSpace m_space = Expired{};
    };

    template <typename ... Types>
    MapLoaderN
        (Types && ... args):
         m_state_space(WaitingForFileContents{ std::forward<Types>(args)... }),
         m_state(&std::get<WaitingForFileContents>(m_state_space))
    {}

    SharedPtr<LoaderTask> update_progress() {
        StateHolder next;
        SharedPtr<LoaderTask> rv;
        while (true) {
            rv = m_state->update_progress(next);
            if (!next.has_next_state()) break;

            next.move_state(m_state, m_state_space);
            if (rv) break;
        }
        return rv;
    }

    bool is_expired() const
        { return std::holds_alternative<Expired>(m_state_space); }

private:
    StateSpace m_state_space;
    State * m_state = nullptr;
};


class MapLinkContainer final {
public:
    using Iterator = std::vector<SharedPtr<TriangleLink>>::iterator;
    using GridOfViews = Grid<View<TriangleLinks::const_iterator>>;

    static constexpr const std::array k_neighbor_offsets = {
        Vector2I{ 1, 0}, Vector2I{0,  1},
        Vector2I{-1, 0}, Vector2I{0, -1},
    };

    explicit MapLinkContainer(const GridOfViews & views) {
        append_links_by_predicate<is_not_edge_tile>(views, m_links);
        auto idx_for_edge = m_links.size();
        append_links_by_predicate<is_edge_tile>(views, m_links);
        m_edge_begin = m_links.begin() + idx_for_edge;
    }

    Iterator edge_begin() { return m_edge_begin; }

    Iterator edge_end() { return m_links.end(); }

    Iterator all_begin() { return m_links.begin(); }

    Iterator all_end() { return m_links.end(); }

private:
    static bool is_edge_tile(const GridOfViews & grid, const Vector2I & r) {
        return std::any_of
            (k_neighbor_offsets.begin(), k_neighbor_offsets.end(),
             [&] (const Vector2I & offset)
             { return !grid.has_position(offset + r); });
    }

    static bool is_not_edge_tile(const GridOfViews & grid, const Vector2I & r)
        { return !is_edge_tile(grid, r); }

    template <bool (*meets_pred)(const GridOfViews &, const Vector2I &)>
    static void append_links_by_predicate
        (const GridOfViews & views, std::vector<SharedPtr<TriangleLink>> & links)
    {
        for (Vector2I r; r != views.end_position(); r = views.next(r)) {
            if (!meets_pred(views, r)) continue;
            for (auto & link : views(r)) {
                links.push_back(link);
            }
        }
    }

    std::vector<SharedPtr<TriangleLink>> m_links;
    Iterator m_edge_begin;
};

class MapSegment final {
public:
    using TeardownTask = MapLoaderN::TeardownTask;

    MapSegment
        (const SharedPtr<TeardownTask> & teardown, MapLinkContainer && link_container):
        m_teardown(teardown),
        m_links(std::move(link_container)) {}

    void glue_to(MapSegment & other_segment) {
#       if 0
        for (auto itr = m_links.edge_begin(); itr != m_links.edge_end(); ++itr) {

        }
#       endif
    }

    void trigger_teardown(TaskCallbacks & callbacks) {
        callbacks.add(m_teardown);
        m_teardown = nullptr; // <- prevent accidental dbl sending
    }

    auto begin() { return m_links.all_begin(); }

    auto end() { return m_links.all_end(); }

private:
    SharedPtr<TeardownTask> m_teardown;
    MapLinkContainer m_links;
};

namespace point_and_plane {
    class Driver;
} // end of point_and_plane namespace

// target object
class MapSegmentContainer final {
public:
    using Size2I = cul::Size2<int>;
    using Rectangle = cul::Rectangle<int>;

    MapSegmentContainer() {}

    void set_chunk_size(const Size2I & chunk_size)
        { m_chunk_size = chunk_size; }

    void emplace_segment(const Vector2I & r, MapSegment && segment) {
        using namespace cul::exceptions_abbr;
        auto pair = m_segments.insert({ r, segment });
        if (pair.second) {
            m_has_changed = true;
            m_unglued_segments.push_back(r);
            return;
        }
        throw InvArg{"MapSegmentContainer::emplace_segment: conflict map "
                     "segments at location: " + std::to_string(r.x) +
                     ", " + std::to_string(r.y)};
    }

    bool has_changed() const { return m_has_changed; }

    void add_all_triangles(point_and_plane::Driver &);

    template <typename RemovePred>
    void remove_segments_if(TaskCallbacks & callbacks, RemovePred && pred) {
        for (auto itr = m_segments.begin(); itr != m_segments.end(); ) {
            auto & [location, segment] = *itr;
            const auto & csegment = segment;
            if (!pred(csegment, Rectangle{location, m_chunk_size})) {
                ++itr;
                continue;
            }
            segment.trigger_teardown(callbacks);
            m_has_changed = true;
            itr = m_segments.erase(itr);
        }
    }

    // at some point, I'm going to have to handle new segment triangles and
    // push them to the pp driver
    //
    // I may have to take them *all* out, and then add them back in (yuck)

private:
    struct Vector2IHasher final {
        std::size_t operator () (const Vector2I & r) const {
            using IntHash = std::hash<int>;
            return IntHash{}(r.x) ^ IntHash{}(r.y);
        }
    };
    static constexpr const auto k_neighbor_offsets =
        MapLinkContainer::k_neighbor_offsets;

    void verify_chunk_size_set() const {
        using namespace cul::exceptions_abbr;
        if (m_chunk_size.width > 0 && m_chunk_size.height > 0) return;
        throw RtError{"MapSegmentContainer: chunk width and height must be positive integers"};
    }

    void glue_together_new_segments() {
        for (auto r : m_unglued_segments) {
            auto itr = m_segments.find(r);
            assert(itr != m_segments.end());
            for (auto offset : k_neighbor_offsets) {
                auto jtr = m_segments.find(r + offset);
                if (jtr == m_segments.end()) continue;
                itr->second.glue_to(jtr->second);
            }
        }

        m_unglued_segments.clear();
    }

    std::unordered_map<Vector2I, MapSegment, Vector2IHasher> m_segments;
    Size2I m_chunk_size;
    bool m_has_changed = false;
    std::vector<Vector2I> m_unglued_segments;
};

/** @brief The MapLoadingDirector is responsible for loading map segments.
 *
 *  Map segments are loaded depending on the player's state.
 */
class MapLoadingDirector final {
public:
    using Size2I = cul::Size2<int>;
    using Rectangle = cul::Rectangle<int>;
    using PpDriver = point_and_plane::Driver;

    MapLoadingDirector(PpDriver * ppdriver, Size2I chunk_size):
        m_ppdriver(ppdriver),
        m_chunk_size(chunk_size)
    {}

    void load_map(const char * initial_map, Platform & platform) {
        m_active_loaders.emplace_back
            (platform, m_segment_container, initial_map, Vector2I{},
             Rectangle{Vector2I{2, 2}, m_chunk_size});
    }

    void on_every_frame(TaskCallbacks & callbacks, const Entity & physics_ent);

private:
    void check_for_other_map_segments
        (TaskCallbacks & callbacks, const Entity & physics_ent);

    // there's only one per game and it never changes
    PpDriver * m_ppdriver = nullptr;
    Size2I m_chunk_size;
    MapSegmentContainer m_segment_container;
    std::vector<MapLoaderN> m_active_loaders;
};

class PlayerUpdateTask final : public EveryFrameTask {
public:
    PlayerUpdateTask
        (MapLoadingDirector && map_director, const EntityRef & physics_ent):
        m_map_director(std::move(map_director)),
        m_physics_ent(physics_ent)
    {}

    void load_initial_map(const char * initial_map, Platform & platform) {
        m_map_director.load_map(initial_map, platform);
    }

    void on_every_frame(Callbacks & callbacks, Real) final {
        using namespace cul::exceptions_abbr;
        if (!m_physics_ent) {
            throw RtError{"Player entity deleted before its update task"};
        }
        Entity physics_ent{m_physics_ent};
        m_map_director.on_every_frame(callbacks, physics_ent);
        check_fall_below(physics_ent);
    }

private:
    static void check_fall_below(Entity & ent);

    MapLoadingDirector m_map_director;
    // | extremely important that the task is *not* owning
    // v the reason entity refs exists
    EntityRef m_physics_ent;
};
