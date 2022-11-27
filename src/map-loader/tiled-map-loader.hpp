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

// ----------------------------------------------------------------------------

class MapSegmentContainer;

using TileFactorySubGrid = cul::ConstSubGrid
    <TileFactory *,
     cul::SubGridParentAccess::allow_access_to_parent_elements>;

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
             const Vector2I & offset, const Rectangle & tiles_to_load):
            m_platform(&platform),
            m_offset(offset),
            m_tiles_to_load(tiles_to_load) {}

        Platform & platform() const {
            verify_shared_set();
            return *m_platform;
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
        Vector2I m_offset;
        Rectangle m_tiles_to_load;
    };

    class WaitingForFileContents final : public State {
    public:
        WaitingForFileContents
            (Platform & platform,
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

    void glue_to(InterTriangleLinkContainer & rhs) {
        for (auto itr = edge_begin(); itr != edge_end(); ++itr) {
            for (auto jtr = rhs.edge_begin(); jtr != rhs.edge_end(); ++jtr) {
                (**itr).attempt_attachment_to(*jtr);
                (**jtr).attempt_attachment_to(*itr);
            }
        }
    }

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

    Iterator edge_begin() { return m_edge_begin; }

    Iterator edge_end() { return m_links.end(); }

    std::vector<SharedPtr<TriangleLink>> m_links;
    Iterator m_edge_begin;
};

class MapRegionPreparer;


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
         TaskCallbacks & callbacks) final;

private:
    Size2I m_region_size;
    TileFactoryGrid m_factory_grid;
};

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

struct LoadedMapRegion final {
    InterTriangleLinkContainer link_edge_container;
    SharedPtr<TeardownTask> teardown;
};

class MapRegionPreparer final : public LoaderTask {
public:
    class SlopesGridFromTileFactories final : public SlopesGridInterface {
    public:
        SlopesGridFromTileFactories(const TileFactorySubGrid & factory_subgrid):
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

    class EntityAndLinkInsertingAdder final : public EntityAndTrianglesAdder {
    public:
        EntityAndLinkInsertingAdder
            (GridViewInserter<SharedPtr<TriangleLink>> & link_inserter_):
            m_link_inserter(link_inserter_) {}

        void add_triangle(const TriangleSegment & triangle) final
            { m_link_inserter.push(make_shared<TriangleLink>(triangle)); }

        void add_entity(const Entity & ent) final
            { m_entities.push_back(ent); }

        auto move_out_entities() { return std::move(m_entities); }

    private:
        GridViewInserter<SharedPtr<TriangleLink>> & m_link_inserter;
        std::vector<Entity> m_entities;
    };
    using NeighborRegions = std::array<LoadedMapRegion *, 4>;

    MapRegionPreparer(LoadedMapRegion & target, Vector2I tile_offset,
                      const NeighborRegions & neighbors):
        m_target_region(target),
        m_tile_offset(tile_offset),
        m_neighbor_regions(neighbors)
    {}

    void operator () (LoaderTask::Callbacks & callbacks) const final
        { finish_map(m_tile_factory_grid, m_target_region, callbacks); }

    void set_tile_factory_subgrid(TileFactorySubGrid && tile_factory_grid)
        { m_tile_factory_grid = std::move(tile_factory_grid); }

protected:
    void finish_map
        (const TileFactorySubGrid & factory_grid, LoadedMapRegion & target,
         Callbacks & callbacks) const
    {
        GridViewInserter<SharedPtr<TriangleLink>> link_inserter{factory_grid.size2()};

        SlopesGridFromTileFactories grid_intf{factory_grid};
        EntityAndLinkInsertingAdder triangle_entities_adder{link_inserter};
        for (; !link_inserter.filled(); link_inserter.advance()) {
            TileFactory::NeighborInfo neighbor_stuff
                {grid_intf, link_inserter.position(), m_tile_offset};
            (*factory_grid(link_inserter.position()))
                (triangle_entities_adder, neighbor_stuff, callbacks.platform());
        }
        auto [link_container, link_view_grid] =
            link_inserter.move_out_container_and_grid_view();
        link_triangles(link_view_grid);
        target.link_edge_container =
            InterTriangleLinkContainer{link_view_grid};
        auto ents = triangle_entities_adder.move_out_entities();
        for (auto & link : link_container) {
            callbacks.add(link);
        }
        for (auto & ent : ents) {
            callbacks.add(ent);
        }
        target.teardown = make_shared<TeardownTask>(std::move(ents));
        for (auto * region_ptr : m_neighbor_regions) {
            if (!region_ptr) continue;
            region_ptr->link_edge_container.glue_to(target.link_edge_container);
        }
    }


private:
    TileFactorySubGrid m_tile_factory_grid;
    LoadedMapRegion & m_target_region;
    Vector2I m_tile_offset;
    NeighborRegions m_neighbor_regions;
};

/// keeps track of already loaded map regions
///
/// regions are treated as one flat collection by this class through a root
/// region
class MapRegionTracker final {
public:
    using Size2I = cul::Size2<int>;
    using NeighborRegions = MapRegionPreparer::NeighborRegions;

    MapRegionTracker() {}

    MapRegionTracker
        (UniquePtr<MapRegion> && root_region,
         const Size2I & region_size_in_tiles):
        m_root_region(std::move(root_region)),
        m_region_size_in_tiles(region_size_in_tiles) {}

    void frame_refresh() { /* lets just get them loading for now */ }

    void frame_hit(const Vector2I & global_region_location, TaskCallbacks & callbacks) {
        if (!m_root_region) return;
        auto itr = m_loaded_regions.find(global_region_location);
        if (itr == m_loaded_regions.end()) {
            // I've a task that creates the triangles, entities, and links the
            // edges. This task is for a fully loaded region.

            // In order to do this I need a target map region.

            auto & loaded_region = m_loaded_regions[global_region_location];
            Vector2I region_tile_offset
                {global_region_location.x*m_region_size_in_tiles.width,
                 global_region_location.y*m_region_size_in_tiles.height};
            auto region_preparer = make_shared<MapRegionPreparer>
                (loaded_region, region_tile_offset,
                 get_neighbors_for(global_region_location));
            // A call into region, may do two things:
            // 1.) generate a task itself (which can then do 2)
            // 2.) immediately launch the aforementioned creation task
            m_root_region->request_region_load(global_region_location, region_preparer, callbacks);
        } else {
            // move to top of LRU or whatever
        }
    }

private:
    using LoadedRegionMap = std::unordered_map<Vector2I, LoadedMapRegion, Vector2IHasher>;

    NeighborRegions get_neighbors_for(const Vector2I & r) {
        return NeighborRegions{
            location_to_pointer(r + Vector2I{ 0,  1}),
            location_to_pointer(r + Vector2I{ 0, -1}),
            location_to_pointer(r + Vector2I{ 1,  0}),
            location_to_pointer(r + Vector2I{-1,  0})
        };
    }

    LoadedMapRegion * location_to_pointer(const Vector2I & r) {
        auto itr = m_loaded_regions.find(r);
        if (itr == m_loaded_regions.end()) return nullptr;
        return &itr->second;
    }

    LoadedRegionMap m_loaded_regions;
    UniquePtr<MapRegion> m_root_region;
    Size2I m_region_size_in_tiles;
};

namespace point_and_plane {
    class Driver;
} // end of point_and_plane namespace

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

    SharedPtr<BackgroundTask> begin_initial_map_loading
        (const char * initial_map, Platform & platform, const Entity & player_physics)
    {
        TiledMapLoader map_loader
            {platform, initial_map, Vector2I{},
             Rectangle{Vector2I{}, m_chunk_size}};
        // presently: task will be lost without completing
        return BackgroundTask::make(
            [this, map_loader = std::move(map_loader),
             player_physics_ = player_physics]
            (TaskCallbacks &) mutable {
                auto grid = map_loader.update_progress();
                if (grid.is_empty()) return BackgroundCompletion::in_progress;
                player_physics_.ensure<Velocity>();
                m_region_tracker = MapRegionTracker
                    {make_unique<TiledMapRegion>
                        (std::move(grid), m_chunk_size),
                     m_chunk_size};
                return BackgroundCompletion::finished;
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
    std::vector<TiledMapLoader> m_active_loaders;
    MapRegionTracker m_region_tracker;
};

class PlayerUpdateTask final : public EveryFrameTask {
public:
    PlayerUpdateTask
        (MapLoadingDirector && map_director, const EntityRef & physics_ent):
        m_map_director(std::move(map_director)),
        m_physics_ent(physics_ent)
    {}

    SharedPtr<BackgroundTask> load_initial_map(const char * initial_map, Platform & platform) {
        return m_map_director.begin_initial_map_loading(initial_map, platform, Entity{m_physics_ent});
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
