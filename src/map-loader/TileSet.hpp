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
#include "map-loader.hpp"
#include "ParseHelpers.hpp"
#include "TileTexture.hpp"
#include "WallTileFactory.hpp"

#include <map>

class TileFactory;
class TileGroup;
class SlopedTileGroup;

// Shit name, but "TileTileFactory" and "TileSetTileFactory" kinda pattern will
// have to come later
// This is a something that produces a tile instance at a specific location
// somewhere in the game.
class ProducableTile {
public:
    virtual ~ProducableTile() {}

    virtual void operator () (const Vector2I & maps_offset,
                              EntityAndTrianglesAdder &, Platform &) const = 0;
};

class ProducableGroup_ {
public:
    virtual ~ProducableGroup_() {}
};

template <typename T>
class ProducableGroup : public ProducableGroup_ {
public:
    static_assert(std::is_base_of_v<ProducableTile, T>);

    ProducableGroup & at_position(const Vector2I &);

    template <typename ... Types>
    void make_producable(Types && ... args) {
        m_producables.emplace_back(std::forward<Types>(args)...);
        set_producable(m_current_position, &m_producables.back());
    }

protected:
    virtual void set_producable(Vector2I, ProducableTile *) = 0;

private:
    Vector2I m_current_position;
    std::vector<T> m_producables;
};

class UnfinishedProducableViewGrid final {
public:
    void add_layer
        (Grid<ProducableTile *> && target,
         const std::vector<SharedPtr<ProducableGroup_>> & groups);

    // the grid view object will need to own groups as well...
private:
    std::vector<Grid<ProducableTile *>> m_layers;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
};

template <typename T>
class GridView;

class TileProducableViewGrid;
class UnfinishedProducableTileGridView final {
public:
    void add_layer
        (Grid<ProducableTile *> && target,
         const std::vector<SharedPtr<ProducableGroup_>> & groups)
    {
        m_groups.insert(m_groups.end(), groups.begin(), groups.end());
        m_targets.emplace_back(std::move(target));
    }

    Tuple
        <GridView<ProducableTile *>,
         std::vector<SharedPtr<ProducableGroup_>>>
        move_out_producables_and_groups();

private:
    std::vector<Grid<ProducableTile *>> m_targets;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
};

class TileGroupGrid final {
public:
    TileGroupGrid
        (Grid<ProducableTile *> && target,
         std::vector<SharedPtr<ProducableGroup_>> && groups):
        m_target(std::move(target)),
        m_groups(std::move(groups))
    {}
#   if 0
    // but this will not be used...
    void operator ()
        (const Vector2I & map_position, const Vector2I & maps_offset,
         EntityAndTrianglesAdder & adder, Platform & platform)
    {
        verify_tile("operator()", m_target(map_position))
            (maps_offset, adder, platform);
    }
#   endif
    // as of Jan 15, 2023:
    // merging multiple instances into a view grid, for
    // use with region preparers:
    // who's going to own the groups? (the special grid type)
    // yeah, I don't have a good solution for vertical maps...

    UnfinishedProducableTileGridView
        add_producables_to(UnfinishedProducableTileGridView && unfinished_grid_view);


private:
#   if 0
    static ProducableTile & verify_tile(const char * caller, ProducableTile *);
#   endif
    Grid<ProducableTile *> m_target;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
};

class UnfinishedTileGroupGrid final {
public:
    using Size2I = cul::Size2<int>;

    // may only be set once
    void set_size(const Size2I &);

    template <typename T>
    SharedPtr<ProducableGroup<T>> start_group_for_type() {
        class Impl final : public ProducableGroup<T> {
        public:
            Impl(Grid<ProducableTile *> & target_, Grid<bool> & filleds_):
                m_target(target_),
                m_filleds(filleds_)
            {}

            void set_producable(Vector2I r, ProducableTile * producable) final {
                using namespace cul::exceptions_abbr;
                if (m_filleds(r)) {
                    throw RtError{"cannot fill tile twice"};
                }
                m_target(r) = producable;
                m_filleds(r) = true;
            }

            Grid<ProducableTile *> & m_target;
            Grid<bool> & m_filleds;
        };
        auto rv = make_shared<Impl>(m_target, m_filleds);
        m_groups.push_back(rv);
        return rv;
    }
    // may only be called once
    TileGroupGrid finish();
#   if 0
    std::vector<Vector2I> filter_filleds(std::vector<Vector2I> &&) const;
#   endif
private:
    Grid<ProducableTile *> m_target;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
    // catch myself if I set a producable twice
    Grid<bool> m_filleds;
};

class TileSetXmlGrid;

// a subset of a tile set, focused on groups, which converts
class TileProducableFiller {
public:
    static SharedPtr<TileProducableFiller> make_ramp_group_filler
        (const TileSetXmlGrid & xml_grid, Platform &);

    struct TileLocation final {
        Vector2I location_on_map;
        Vector2I location_on_tileset;
    };

    virtual ~TileProducableFiller() {}

    virtual UnfinishedTileGroupGrid operator ()
        (const std::vector<TileLocation> &, UnfinishedTileGroupGrid &&) const = 0;
};

// Grid of xml elements, plus info on tileset
class TileSetXmlGrid final {
public:
    using Size2I = cul::Size2<int>;

    void load(Platform &, const TiXmlElement &);

    const TiXmlElement * operator() (const Vector2I & r) const
        { return m_elements(r); }

    Size2 tile_size() const
        { return m_tile_size; }

    Size2 texture_size() const
        { return m_texture_size; }

    SharedPtr<const Texture> texture() const
        { return m_texture; }

    Vector2I next(const Vector2I & r) const
        { return m_elements.next(r); }

    Vector2I end_position() const
        { return m_elements.end_position(); }

    Size2I size2() const
        { return m_elements.size2(); }

    auto size() const { return m_elements.size(); }

private:
    static Tuple<SharedPtr<const Texture>, Size2>
        load_texture(Platform &, const TiXmlElement &);

    Grid<const TiXmlElement *> m_elements;
    SharedPtr<const Texture> m_texture;
    Size2 m_tile_size;
    Size2 m_texture_size;
};

class TileSetN final {
public:
    using FillerFactory = SharedPtr<TileProducableFiller>(*)(const TileSetXmlGrid &, Platform &);
    using FillerFactoryMap = std::map<std::string, FillerFactory>;
    using TileLocation = TileProducableFiller::TileLocation;

    static const FillerFactoryMap & builtin_fillers();

    // groups shared between tilesets? <- yup this idea is stupid
    // shelve it fucker

    void load(Platform &, const TiXmlElement &, const FillerFactoryMap & = builtin_fillers());

    SharedPtr<TileProducableFiller> find_filler(int tid) const;

    SharedPtr<TileProducableFiller> find_filler(Vector2I) const;

    Vector2I tile_id_to_tileset_location(int tid) const;

    std::size_t total_tile_count() const
        { return m_filler_grid.size(); }

private:
    Grid<SharedPtr<TileProducableFiller>> m_filler_grid;
};

class TileSet final {
public:

#   if 0
    // there may, or may not be a factory for a particular id
    Tuple<TileFactory *, TileProducableFiller *> operator () (int tid) const;
#   endif
#   if 0
    TileProducableFiller & group_filler_for_id(int tid) const;
#   endif
    TileFactory * factory_for_id(int tid) const;

    void load(Platform &, const TiXmlElement & tileset);

    int total_tile_count() const { return m_tile_count; }
#   if 0 // this is a SRP violation from hell
    ProcessedLayer process_layer(ProcessedLayer && layer) const {
        auto & tile_ids = layer.tile_ids;
        auto & factory_group_tuples = layer.factory_group_tuples;
        for (Vector2I r; r != tile_ids.end_position(); r = tile_ids.next(r)) {
            auto itr = m_group_factory_map.find(tile_ids(r));
            if (itr == m_group_factory_map.end()) {
                --layer.remaining_ids;
                continue;
            }
            layer = (*itr->second)(  )
            ;
        }
    }

    ProcessedLayer process_layer(Grid<int> && layer) const {
        ProcessedLayer starting_layer;
        starting_layer.tile_ids = std::move(layer);
        starting_layer.remaining_ids = starting_layer.tile_ids.size();
        starting_layer.factory_group_tuples.set_size
            (starting_layer.tile_ids.size2(), make_tuple(nullptr, nullptr));
        return process_layer(std::move(starting_layer));
    }
#   endif
private:

    using TileTextureMap = std::map<std::string, TileTexture>;
    struct TileParams final {
        TileParams(Size2 tile_size_, Platform & platform_):
            tile_size(tile_size_),
            platform(platform_) {}

        Size2 tile_size;
        Platform & platform;
    };
    // still really airy
    using LoadTileTypeFunc = void (TileSet::*)
        (const TiXmlElement &, int, Vector2I, TileParams &);

    //using TileGroupFactory = Tuple<UniquePtr<TileGroup>, ProcessedLayer> (*)(ProcessedLayer &&);
    //using LoadTileGroupFactoryFunc = ProcessedLayer(*)(ProcessedLayer &&);

    using TileTypeFuncMap = std::map<std::string, LoadTileTypeFunc>;
#   if 0
    using TileGroupFuncMap = std::map<std::string, TileProducableFiller>;
#   endif
    static const TileTypeFuncMap & tiletype_handlers();
#   if 0
    static const TileGroupFuncMap & tilegroup_handlers();
#   endif
    TileFactory & insert_factory(UniquePtr<TileFactory> uptr, int tid);

    void load_pure_texture(const TiXmlElement &, int, Vector2I, TileParams &);

    void set_texture_information
        (const SharedPtr<const Texture> & texture, const Size2 & tile_size,
         const Size2 & texture_size);

    void load_factory
        (const TiXmlElement &, UniquePtr<TileFactory> factory, int, Vector2I,
         Platform &);

    template <typename T>
    void load_usual_factory
        (const TiXmlElement & el, int id, Vector2I r, TileParams & tp)
    {
        static_assert(std::is_base_of_v<TileFactory, T>);
        load_factory(el, make_unique<T>(), id, r, tp.platform);
    }

    template <typename T>
    void load_usual_wall_factory
        (const TiXmlElement & el, int id, Vector2I r, TileParams & tp)
    {
        static_assert(std::is_base_of_v<WallTileFactoryBase, T>);
        auto wall_factory = make_unique<T>();
        wall_factory->assign_wall_texture(m_tile_texture_map["wall"]);
        load_factory(el, std::move(wall_factory), id, r, tp.platform);
    }

    std::map<int, UniquePtr<TileFactory>> m_factory_map;
#   if 0
    std::map<int, TileProducableFiller *> m_group_factory_map;
#   endif
    TileTextureMap m_tile_texture_map;

    SharedPtr<const Texture> m_texture;
    Size2 m_texture_size;
    Size2 m_tile_size;
    int m_tile_count = 0;
};

class SlopedTileFactory;

class TileGroup {
public:
    // neighbors...
    // double dispatch?
    virtual ~TileGroup() {}

    virtual void on_sloped_tile_factory(const SlopedTileFactory &) const = 0;
};

class SlopedTileGroup final : public TileGroup {
public:
    using Size2I = cul::Size2<int>;

    void on_sloped_tile_factory(const SlopedTileFactory &) const final;

    Real neighbor_elevation(const Vector2I &, CardinalDirection) const;

    void set_size(const Size2I &);

    void set_factory_at(TileFactory *, const Vector2I &);

private:
    Grid<TileFactory *> m_factory_grid;
    //const SlopesGridInterface * m_grid = &SlopesGridInterface::null_instance();
};
#if 0
inline /* static */ TileGroupFiller * TileGroupFiller::ramp_group_factory() {
    class RampGroupFactory final : public TileGroupFiller {
    public:
        // fuck you Singleton
        static RampGroupFactory & instance() {
            static RampGroupFactory inst;
            return inst;
        }

        ProcessedLayer operator ()
            (const Grid<TileGroupFiller *> & group_layer,
             ProcessedLayer && inprogress_layer) const final
        {
            using namespace cul::exceptions_abbr;
            if (   group_layer.size2() != inprogress_layer.tile_ids.size2()
                || group_layer.size2() != inprogress_layer.factory_group_tuples.size2())
            {
                throw InvArg{"inconsistent layer sizes"};
            }
            SharedPtr<SlopedTileGroup> stg;
            for (Vector2I r; r != group_layer.end_position(); r = group_layer.next(r)) {
                if (inprogress_layer.tile_ids(r) == 0)
                    continue;
                // uncondtional for now; all existing types are ramps
                if (true || group_layer(r) == &RampGroupFactory::instance()) {
                    if (!stg) {
                        stg = make_shared<SlopedTileGroup>();
                        stg->set_size(group_layer.size2());
                    }
                    --inprogress_layer.remaining_ids;
                    inprogress_layer.tile_ids(r) = 0;
                    // this feels smelly?
                    stg->set_factory_at(std::get<TileFactory *>(/* downcasted */ inprogress_layer.factory_group_tuples(r)), r);
                    std::get<SharedPtr<TileGroup>>(inprogress_layer.factory_group_tuples(r)) = stg;
                }
            }
            return std::move(inprogress_layer);
        }

    private:
        RampGroupFactory() {}
    };
    return &RampGroupFactory::instance();
}
#endif
struct TileLocation final {
    Vector2I on_map;
    Vector2I on_field;
};

// straight up yoinked from tmap
// this is just used wrong I think...
class GidTidTranslator final {
public:
#   if MACRO_BIG_RED_BUTTON
    using ConstTileSetPtr = SharedPtr<const TileSetN>;
    using TileSetPtr      = SharedPtr<TileSetN>;
#   else
    using ConstTileSetPtr = SharedPtr<const TileSet>;
    using TileSetPtr      = SharedPtr<TileSet>;
#   endif

    GidTidTranslator() {}

    GidTidTranslator(const std::vector<TileSetPtr> & tilesets, const std::vector<int> & startgids);

    std::pair<int, ConstTileSetPtr> gid_to_tid(int gid) const;

    std::pair<int, TileSetPtr> gid_to_tid(int gid);

    int tid_to_gid(int tid, ConstTileSetPtr tileset) const;

    void swap(GidTidTranslator &);

private:
    template <bool kt_is_const>
    struct GidAndTileSetPtrImpl {
        using TsPtrType =
            typename std::conditional_t<kt_is_const, ConstTileSetPtr, TileSetPtr>;

        GidAndTileSetPtrImpl() {}

        GidAndTileSetPtrImpl(int sid, const TsPtrType & ptr):
            starting_id(sid), tileset(ptr) {}

        int starting_id = 0;
        TsPtrType tileset;
    };

    using GidAndTileSetPtr      = GidAndTileSetPtrImpl<false>;
    using GidAndConstTileSetPtr = GidAndTileSetPtrImpl<true>;

    static bool order_by_gids(const GidAndTileSetPtr &, const GidAndTileSetPtr &);
    static bool order_by_ptrs(const GidAndConstTileSetPtr &, const GidAndConstTileSetPtr &);

    std::vector<GidAndConstTileSetPtr> m_ptr_map;
    std::vector<GidAndTileSetPtr> m_gid_map;
    int m_gid_end = 0;
};
