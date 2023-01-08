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

class UnfinishedTileGroupGrid final {
public:
    using Size2I = cul::Size2<int>;

    // may only be set once
    void set_size(const Size2I &);

    // may only be called once per tile location
    void set(Vector2I, TileFactory *, SharedPtr<TileGroup>);

    bool filled(Vector2I) const;

    Vector2I next(Vector2I) const noexcept;

    Vector2I end_position() const noexcept;

    // may only be called once
    auto finish();

private:
    Grid<Tuple<bool, TileFactory *, SharedPtr<TileGroup>>> m_target_grid;
};

class TileGroupFiller {
public:
    struct ProcessedLayer final {
        int remaining_ids = 0;
        Grid<int> tile_ids;
        Grid<Tuple<TileFactory *, SharedPtr<TileGroup>>> factory_group_tuples;
    };

    static TileGroupFiller * ramp_group_factory();

    virtual ~TileGroupFiller() {}

    virtual UnfinishedTileGroupGrid operator ()
        (const Grid<TileGroupFiller *> &, UnfinishedTileGroupGrid &&) const = 0;
};


class TileSet final {
public:
    using ProcessedLayer  = TileGroupFiller::ProcessedLayer;
    using ConstTileSetPtr = SharedPtr<const TileSet>;
    using TileSetPtr      = SharedPtr<TileSet>;

    // there may, or may not be a factory for a particular id
    Tuple<TileFactory *, TileGroupFiller *> operator () (int tid) const;

    TileGroupFiller & group_filler_for_id(int tid) const;

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
    using TileGroupFuncMap = std::map<std::string, TileGroupFiller>;

    static const TileTypeFuncMap & tiletype_handlers();

    static const TileGroupFuncMap & tilegroup_handlers();

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
    std::map<int, TileGroupFiller *> m_group_factory_map;
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

struct TileLocation final {
    Vector2I on_map;
    Vector2I on_field;
};

// straight up yoinked from tmap
// this is just used wrong I think...
class GidTidTranslator final {
public:
    using ConstTileSetPtr = TileSet::ConstTileSetPtr;
    using TileSetPtr      = TileSet::TileSetPtr;

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
