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

#include "platform/platform.hpp"
#include "map-loader.hpp"
#include "ParseHelpers.hpp"

#include <map>

class TileFactory;

class TileSet final {
public:
    using ConstTileSetPtr = std::shared_ptr<const TileSet>;
    using TileSetPtr      = std::shared_ptr<TileSet>;
    struct TileTexture final {
        Vector2 nw, sw, se, ne;
    };

    // there may, or may not be a factory for a particular id
    TileFactory * operator () (int tid) const;

    TileFactory & insert_factory(UniquePtr<TileFactory> uptr, int tid);

    void load_information(Platform::ForLoaders &, const TiXmlElement & tileset);

    void set_texture_information
        (const SharedPtr<const Texture> & texture, const Size2 & tile_size,
         const Size2 & texture_size);

    int total_tile_count() const { return m_tile_count; }

private:
    using TileTextureMap = std::map<std::string, TileTexture>;
    struct TileParams final {
        TileParams(Size2 tile_size_,
                   Platform::ForLoaders & platform_):
            tile_size(tile_size_),
            platform(platform_) {}
        Size2 tile_size;
        Platform::ForLoaders & platform;
    };
    // still really airy
    using LoadTileTypeFunc = void (TileSet::*)
        (const TiXmlElement &, int, Vector2I, TileParams &);
    using TileTypeFuncMap = std::map<std::string, LoadTileTypeFunc>;

    static const TileTypeFuncMap & tiletype_handlers();

    void load_pure_texture(const TiXmlElement &, int, Vector2I, TileParams &);

    void load_wall_factory(const TiXmlElement &, int, Vector2I, TileParams &);

    void load_factory
        (const TiXmlElement &, UniquePtr<TileFactory> factory, int, Vector2I,
         Platform::ForLoaders &);

    template <typename T>
    void load_usual_factory
        (const TiXmlElement & el, int id, Vector2I r, TileParams & tp)
    {
        static_assert(std::is_base_of_v<TileFactory, T>);
        load_factory(el, make_unique<T>(), id, r, tp.platform);
    }

    std::map<int, UniquePtr<TileFactory>> m_factory_map;
    TileTextureMap m_tile_texture_map;

    SharedPtr<const Texture> m_texture;
    Size2 m_texture_size;
    Size2 m_tile_size;
    int m_tile_count = 0;
};

// straight up yoinked from tmap
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
        using TsPtrType = typename std::conditional_t<kt_is_const, ConstTileSetPtr, TileSetPtr>;
        GidAndTileSetPtrImpl() {}
        GidAndTileSetPtrImpl(int sid, const TsPtrType & ptr): starting_id(sid), tileset(ptr) {}
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
