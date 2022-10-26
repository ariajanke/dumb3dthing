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

class TileTextureN final {
public:
    TileTextureN();

    TileTextureN(Vector2 nw, Vector2 se);

    TileTextureN(Vector2I tileset_loc, const Size2 & tile_size);

    Vector2 texture_position_for(const Vector2 & tile_normalized_location) const;

private:
    Vector2 m_nw, m_se;
};

inline TileTextureN::TileTextureN() {}

inline TileTextureN::TileTextureN
    (Vector2 nw, Vector2 se):
    m_nw(nw),
    m_se(se)
{}

inline TileTextureN::TileTextureN(Vector2I tileset_loc, const Size2 & tile_size) {
    using cul::convert_to;
    Vector2 offset{tileset_loc.x*tile_size.width, tileset_loc.y*tile_size.height};
    m_nw = offset;
    m_se = offset + convert_to<Vector2>(tile_size);
}

inline Vector2 TileTextureN::texture_position_for
    (const Vector2 & tile_normalized_location) const
{
    auto r = tile_normalized_location;
    return Vector2{r.x*m_se.x + m_nw.x*(1 - r.x),
                   r.y*m_se.y + m_nw.y*(1 - r.y)};
}

class TileSet final {
public:
    using ConstTileSetPtr = std::shared_ptr<const TileSet>;
    using TileSetPtr      = std::shared_ptr<TileSet>;
#   if 0
    struct TileTexture final {

        Vector2 texture_position_for(const Vector2 & tile_normalized_position) const {
            auto r = tile_normalized_position + Vector2{0.5, 0.5};
            return Vector2{r.x*se.x + nw.x*(1 - r.x),
                           r.y*se.y + nw.y*(1 - r.y)};
        }

        Vector2 nw, sw, se, ne;
    };
#   endif
    // there may, or may not be a factory for a particular id
    TileFactory * operator () (int tid) const;

    void load_information(Platform::ForLoaders &, const TiXmlElement & tileset);

    int total_tile_count() const { return m_tile_count; }

private:
    using TileTextureMap = std::map<std::string, TileTextureN>;
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

    TileFactory & insert_factory(UniquePtr<TileFactory> uptr, int tid);

    void load_pure_texture(const TiXmlElement &, int, Vector2I, TileParams &);

    void set_texture_information
        (const SharedPtr<const Texture> & texture, const Size2 & tile_size,
         const Size2 & texture_size);

#   if 0
    void load_wall_factory(const TiXmlElement &, int, Vector2I, TileParams &);
#   endif
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
