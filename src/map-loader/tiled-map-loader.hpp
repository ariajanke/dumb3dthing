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
#include <ariajanke/cul/SubGrid.hpp>

#include <unordered_map>

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
#   if 0
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
#   endif
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

using TileFactorySubGrid = cul::ConstSubGrid<TileFactory *>;

class TileFactoryGrid final {
public:
    using Rectangle = cul::Rectangle<int>;

    void load_layer(const Grid<int> & gids, const GidTidTranslator &);

    auto height() const { return m_factories.height(); }

    auto width() const { return m_factories.width(); }

    auto is_empty() const { return m_factories.is_empty(); }

    /** this object must live at least as long as the return value */
    TileFactorySubGrid make_subgrid(const Rectangle & range) const {
        return TileFactorySubGrid{m_factories, cul::top_left_of(range),
                                  range.width, range.height};
    }

private:
    std::vector<SharedPtr<const TileSet>> m_tilesets;
    Grid<TileFactory *> m_factories;
};

/// loads a TiledMap asset file
class TiledMapLoader final {
public:
    using Rectangle = cul::Rectangle<int>;

    class StateHolder;

    class State {
    public:
        virtual ~State() {}

        virtual TileFactoryGrid update_progress(StateHolder &)
            { return TileFactoryGrid{}; }

        State & set_others_stuff(State & lhs) const {
            lhs.m_platform = m_platform;
#           if 0
            lhs.m_target_container = m_target_container;
#           endif
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
            (Platform & platform,
#           if 0
             MapSegmentContainer & target_container,
#           endif
             const Vector2I & offset, const Rectangle & tiles_to_load):
            m_platform(&platform),
#           if 0
            m_target_container(&target_container),
#           endif
            m_offset(offset),
            m_tiles_to_load(tiles_to_load) {}

        Platform & platform() const {
            verify_shared_set();
            return *m_platform;
        }
#       if 0
        MapSegmentContainer & target() {
            verify_shared_set();
            return *m_target_container;
        }
#       endif
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
#       if 0
        MapSegmentContainer * m_target_container = nullptr;
#       endif
        Vector2I m_offset;
        Rectangle m_tiles_to_load;
    };

    class WaitingForFileContents final : public State {
    public:
        WaitingForFileContents
            (Platform & platform,
#            if 0
             MapSegmentContainer & target_container,
#            endif
             const char * filename, const Vector2I & offset,
             const Rectangle & tiles_to_load):
             State(platform, /*target_container, */offset, tiles_to_load)
        { m_file_contents = platform.promise_file_contents(filename); }

        TileFactoryGrid update_progress(StateHolder & next_state) final;

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

        TileFactoryGrid update_progress(StateHolder & next_state) final;

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

        TileFactoryGrid update_progress(StateHolder & next_state) final;

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
        using StatePtrGetter = State * (*)(StateSpace &);

        template <typename NextState, typename ... Types>
        NextState & set_next_state(Types && ...args) {
            m_space = NextState{std::forward<Types>(args)...};
            m_get_state = [](StateSpace & space) -> State *
                { return &std::get<NextState>(space); };
            return std::get<NextState>(m_space);
        }

        bool has_next_state() const noexcept
            { return m_get_state; }

        void move_state(StatePtrGetter & state_getter_ptr, StateSpace & space) {
            using namespace cul::exceptions_abbr;
            if (!m_get_state) {
                throw RtError{"No state to move"};
            }

            space = std::move(m_space);
            state_getter_ptr = m_get_state;
            m_get_state = nullptr;
        }

    private:
        StatePtrGetter m_get_state = nullptr;
        StateSpace m_space = Expired{};
    };

    template <typename ... Types>
    TiledMapLoader
        (Types && ... args):
         m_state_space(WaitingForFileContents{ std::forward<Types>(args)... }),
         m_get_state([] (StateSpace & space) -> State * { return &std::get<WaitingForFileContents>(space); })
    {}

    // return instead a grid of tile factories
    // (note: that grid must own tilesets)
    TileFactoryGrid update_progress() {
        StateHolder next;
        TileFactoryGrid rv;
        while (true) {
            rv = m_get_state(m_state_space)->update_progress(next);
            if (!next.has_next_state()) break;

            next.move_state(m_get_state, m_state_space);
            if (!rv.is_empty()) break;
        }
        return rv;
    }

    bool is_expired() const
        { return std::holds_alternative<Expired>(m_state_space); }

private:
    using StatePtrGetter = State * (*)(StateSpace &);
    StateSpace m_state_space;
    StatePtrGetter m_get_state = nullptr;
};
#if 0
// Triangle links, link to three triangles
// Map links, link to four map "segments"
class MapLink final {

};

class InterMapLinkContainer final {

};
#endif

/// container of triangle links, used to glue segment triangles together
class InterTriangleLinkContainer final {
public:
    using Iterator = std::vector<SharedPtr<TriangleLink>>::iterator;
    using GridOfViews = Grid<View<TriangleLinks::const_iterator>>;

    static constexpr const std::array k_neighbor_offsets = {
        Vector2I{ 1, 0}, Vector2I{0,  1},
        Vector2I{-1, 0}, Vector2I{0, -1},
    };

    InterTriangleLinkContainer() {}

    explicit InterTriangleLinkContainer(const GridOfViews & views) {
        append_links_by_predicate<is_not_edge_tile>(views, m_links);
        auto idx_for_edge = m_links.size();
        append_links_by_predicate<is_edge_tile>(views, m_links);
        m_edge_begin = m_links.begin() + idx_for_edge;
    }
#   if 0
    Iterator edge_begin() { return m_edge_begin; }

    Iterator edge_end() { return m_links.end(); }

    Iterator all_begin() { return m_links.begin(); }

    Iterator all_end() { return m_links.end(); }
#   endif

    void glue_to(InterTriangleLinkContainer &);

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


struct LoadedMapRegion final {
    InterTriangleLinkContainer link_edge_container;
    SharedPtr<TeardownTask> teardown;
};
#if 1
class MapRegionPreparer : public OccasionalTask {
public:
    template <typename Func>
    static SharedPtr<MapRegionPreparer> make(LoadedMapRegion & target, Func && f_) {
        class Impl final : public MapRegionPreparer {
        public:
            Impl(LoadedMapRegion & target, Func && f_):
                MapRegionPreparer(target),
                m_f(std::move(f_)) {}

            void finish_map
                (const TileFactorySubGrid & factory_grid, LoadedMapRegion & target,
                 Callbacks & callbacks) final
                { m_f(factory_grid, target, callbacks); }

        private:
            Func m_f;
        };
        return make_shared<Impl>(target, std::move(f_));
    }

    void set_tile_factory_subgrid(TileFactorySubGrid && tile_factory_grid)
        { m_tile_factory_grid = std::move(tile_factory_grid); }

protected:
    MapRegionPreparer(LoadedMapRegion & target):
        m_target_region(target)
    {}

    virtual void finish_map(const TileFactorySubGrid &, LoadedMapRegion & target, Callbacks & callbacks) = 0;

    void on_occasion(Callbacks & callbacks) final
        { finish_map(m_tile_factory_grid, m_target_region, callbacks); }

#   if 0
    void set_edge_container(InterTriangleLinkContainer && container)
        { m_target_region.link_edge_container = std::move(container); }

    void set_teardown_task(const SharedPtr<TeardownTask> & teardown)
        { m_target_region.teardown = teardown; }
#   endif
private:
    TileFactorySubGrid m_tile_factory_grid;
    LoadedMapRegion & m_target_region;
};
#endif
/// a map region is a grid of task pairs, one to load, one to teardown
class MapRegion {
public:
    static UniquePtr<MapRegion> make_null_instance() {
        class Impl final : public MapRegion {
            void request_region_load
                (const Vector2I &, const SharedPtr<MapRegionPreparer> &,
                 TaskCallbacks &) final
            {}
        };
        return make_unique<Impl>();
    }
#   if 0
    struct TaskPair final {
        SharedPtr<LoaderTask> loader;
        SharedPtr<TeardownTask> teardown;
    };
#   endif
    virtual ~MapRegion() {}

    virtual void request_region_load
        (const Vector2I & local_position,
         const SharedPtr<MapRegionPreparer> & region_preparer, TaskCallbacks &) = 0;
};

/// is a map region, built off a TiledMap asset
class TiledMapRegion final : public MapRegion {
public:
    using Size2I = cul::Size2<int>;
    using Rectangle = cul::Rectangle<int>;

    TiledMapRegion
        (TileFactoryGrid && full_factory_grid,
         const Size2I & region_size_in_tiles):
         m_region_size(region_size_in_tiles),
         m_factory_grid(std::move(full_factory_grid)) {}

    void request_region_load
        (const Vector2I & local_region_position,
         const SharedPtr<MapRegionPreparer> & region_preparer,
         TaskCallbacks & callbacks) final
    {
        auto region_left   = std::max
            (local_region_position.x*m_region_size.width, 0);
        auto region_top    = std::max
            (local_region_position.y*m_region_size.height, 0);
        auto region_right  = std::min
            (region_left + m_region_size.width, m_factory_grid.width());
        auto region_bottom = std::min
            (region_top + m_region_size.height, m_factory_grid.height());
        if (region_left == region_right || region_top == region_bottom) {
            return;
        }
        auto factory_subgrid = m_factory_grid.make_subgrid(Rectangle
            {region_left, region_top,
             region_right - region_left, region_bottom - region_top});
        region_preparer->set_tile_factory_subgrid(std::move(factory_subgrid));
        callbacks.add(region_preparer);
    }

private:
    Size2I m_region_size;
    TileFactoryGrid m_factory_grid;
};

// I still need to link triangles together


struct Vector2IHasher final {
    std::size_t operator () (const Vector2I & r) const {
        using IntHash = std::hash<int>;
        return IntHash{}(r.x) ^ IntHash{}(r.y);
    }
};

template <typename T>
class GridViewInserter final {
public:
    using ElementContainer = std::vector<T>;
    using ElementIterator  = typename std::vector<T>::const_iterator;
    using ElementView      = View<ElementIterator>;
    using Size2I           = cul::Size2<int>;

    GridViewInserter(int width, int height)
        { m_index_pairs.set_size(width, height, make_tuple(0, 0)); }

    explicit GridViewInserter(Size2I size):
        GridViewInserter(size.width, size.height) {}

    void advance() {
        using namespace cul::exceptions_abbr;
        if (filled()) {
            throw RtError{""};
        }
        auto el_count = m_elements.size();
        std::get<1>(m_index_pairs(m_position)) = el_count;
        auto next_position = m_index_pairs.next(m_position);
        if (next_position != m_index_pairs.end_position())
            m_index_pairs(next_position) = make_tuple(el_count, el_count);
        m_position = next_position;
    }

    void push(const T & obj)
        { m_elements.push_back(obj); }

    Vector2I position() const noexcept
        { return m_position; }

    bool filled() const noexcept
        { return m_position == m_index_pairs.end_position(); }

    Tuple<ElementContainer, Grid<ElementView>>
        move_out_container_and_grid_view()
    {
        Grid<View<ElementIterator>> views;
        views.set_size
            (m_index_pairs.width(), m_index_pairs.height(),
             ElementView{m_elements.end(), m_elements.end()});
        for (Vector2I r; r != m_index_pairs.end_position(); r = m_index_pairs.next(r)) {
            auto [beg_idx, end_idx] = m_index_pairs(r);
            views(r) = ElementView
                {m_elements.begin() + beg_idx, m_elements.begin() + end_idx};
        }
        return make_tuple(std::move(m_elements), std::move(views));
    }

private:
    Vector2I m_position;
    std::vector<T> m_elements;
    Grid<Tuple<std::size_t, std::size_t>> m_index_pairs;
};

/// keeps track of already loaded map regions
///
/// regions are treated as one flat collection by this class through a root
/// region
class MapRegionTracker final {
public:
    MapRegionTracker() {}

    explicit MapRegionTracker(UniquePtr<MapRegion> && root_region):
        m_root_region(std::move(root_region)) {}

    void frame_refresh() { /* lets just get them loading for now */ }

    void frame_hit(const Vector2I & global_region_location, TaskCallbacks & callbacks) {
        if (!m_root_region) return;
        auto itr = m_loaded_regions.find(global_region_location);
        if (itr == m_loaded_regions.end()) {
            // I've a task that creates the triangles, entities, and links the
            // edges. This task is for a fully loaded region.

            // In order to do this I need a target map region.

            auto & loaded_region = m_loaded_regions[global_region_location];
            auto region_preparer = MapRegionPreparer::make(loaded_region,
                [global_region_location]
                // I need everything I need to build my tiles
                (const TileFactorySubGrid & factory_grid,
                 LoadedMapRegion & target, TaskCallbacks & callbacks)
            {
                class GridIntfImpl final : public SlopesGridInterface {
                public:
                    GridIntfImpl(const TileFactorySubGrid & factory_subgrid):
                        m_grid(factory_subgrid) {}

                    Slopes operator () (Vector2I r) const {
                        static const Slopes k_all_inf{k_inf, k_inf, k_inf, k_inf};
                        if (!m_grid.has_position(r)) return k_all_inf;
                        if (!m_grid(r)) return k_all_inf;
                        return m_grid(r)->tile_elevations();
                    }
                private:
                    const TileFactorySubGrid & m_grid;
                };
                class Impl final : public EntityAndTrianglesAdder {
                public:
                    Impl(GridViewInserter<SharedPtr<TriangleLink>> & link_inserter_):
                        m_link_inserter(link_inserter_) {}

                    void add_triangle(const TriangleSegment & triangle) final {
                        m_link_inserter.push(make_shared<TriangleLink>(triangle));
                    }

                    void add_entity(const Entity & ent) final
                        { m_entities.push_back(ent); }

                    auto move_out_entities() { return std::move(m_entities); }

                private:
                    GridViewInserter<SharedPtr<TriangleLink>> & m_link_inserter;
                    std::vector<Entity> m_entities;
                };
                GridViewInserter<SharedPtr<TriangleLink>> link_inserter{factory_grid.size2()};

                GridIntfImpl grid_intf{factory_grid};
                Impl triangle_entities_adder{link_inserter};
                for (; !link_inserter.filled(); link_inserter.advance()) {
                    TileFactory::NeighborInfo neighbor_stuff
                        {grid_intf, link_inserter.position(), global_region_location};
                    (*factory_grid(link_inserter.position()))
                        (triangle_entities_adder, neighbor_stuff, callbacks.platform());
                }
                auto [link_container, link_view_grid] =
                    link_inserter.move_out_container_and_grid_view();

                target.link_edge_container =
                    InterTriangleLinkContainer{link_view_grid};
                auto ents = triangle_entities_adder.move_out_entities();
                auto links_ent = Entity::make_sceneless_entity();
                links_ent.add<TriangleLinks>() = std::move(link_container);
                callbacks.add(links_ent);
                for (auto & ent : ents) {
                    callbacks.add(ent);
                }
                target.teardown = make_shared<TeardownTask>(std::move(ents));
            });

            // A call into region, may do two things:
            // 1.) generate a task itself (which can then do 2)
            // 2.) immediately launch the aforementioned creation task
            m_root_region->request_region_load(global_region_location, region_preparer, callbacks);

            // start loading the map

            //callbacks.add();
        } else {
            // move to top of LRU or whatever
        }
    }

private:

    using LoadedRegionMap = std::unordered_map<Vector2I, LoadedMapRegion, Vector2IHasher>;

    LoadedRegionMap m_loaded_regions;
    UniquePtr<MapRegion> m_root_region;
};

#if 0 // later
struct MapPreview final {
    using Size2I = cul::Size2<int>;
    Size2I size;
    // something describing the image
};

MapPreview make_map_preview(const TiXmlDocument & tilemap);
#endif
// next I'd need to pack them (shouldn't be too hard with libraries)

#if 0
// a segment of map presently loaded in game
class MapSegment final {
public:
    MapSegment
        (const SharedPtr<TeardownTask> & teardown, InterTriangleLinkContainer && link_container):
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
    InterTriangleLinkContainer m_links;
};
#endif
namespace point_and_plane {
    class Driver;
} // end of point_and_plane namespace
#if 0
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

    bool segment_loaded_at(const Vector2I & r) const
        { return m_segments.find(r) != m_segments.end(); }

private:
    struct Vector2IHasher final {
        std::size_t operator () (const Vector2I & r) const {
            using IntHash = std::hash<int>;
            return IntHash{}(r.x) ^ IntHash{}(r.y);
        }
    };
    static constexpr const auto k_neighbor_offsets =
        InterTriangleLinkContainer::k_neighbor_offsets;

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
#endif
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
#   if 0
    void load_map(const char * initial_map, Platform & platform) {
        m_active_loaders.emplace_back
            (platform, m_segment_container, initial_map, Vector2I{},
             Rectangle{Vector2I{}, m_chunk_size});
    }
#   endif
    SharedPtr<EveryFrameTask> begin_initial_map_loading
        (const char * initial_map, Platform & platform, const Entity & player_physics)
    {
#       if 0
        TiledMapLoader map_loader
            {platform, m_segment_container, initial_map, Vector2I{},
             Rectangle{Vector2I{}, m_chunk_size}};
#       endif
        TiledMapLoader map_loader
            {platform, initial_map, Vector2I{},
             Rectangle{Vector2I{}, m_chunk_size}};
        // presently: task will be lost without completing
        return EveryFrameTask::make(
            [this, map_loader = std::move(map_loader),
             player_physics_ = player_physics]
            (TaskCallbacks &, Real) mutable {
                auto grid = map_loader.update_progress();
                if (!grid.is_empty()) {
                    player_physics_.ensure<Velocity>();
                    m_region_tracker = MapRegionTracker
                        {make_unique<TiledMapRegion>
                            (std::move(grid), m_chunk_size)};
                }
            });
    }

    void on_every_frame(TaskCallbacks & callbacks, const Entity & physics_ent);

private:
    void check_for_other_map_segments
        (TaskCallbacks & callbacks, const Entity & physics_ent);

    static Vector2I to_segment_location
        (const Vector & location, const Size2I & segment_size);

    // there's only one per game and it never changes
    PpDriver * m_ppdriver = nullptr;
    Size2I m_chunk_size;
#   if 0
    //MapSegmentContainer m_segment_container;
#   endif
    std::vector<TiledMapLoader> m_active_loaders;
#   if 0
    UniquePtr<MapRegion> m_root_region = MapRegion::make_null_instance();
#   endif
    MapRegionTracker m_region_tracker;
};

class PlayerUpdateTask final : public EveryFrameTask {
public:
    PlayerUpdateTask
        (MapLoadingDirector && map_director, const EntityRef & physics_ent):
        m_map_director(std::move(map_director)),
        m_physics_ent(physics_ent)
    {}

    void load_initial_map(Callbacks & callbacks, const char * initial_map, Platform & platform) {
         callbacks.add(m_map_director.begin_initial_map_loading(initial_map, platform, Entity{m_physics_ent}));
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
