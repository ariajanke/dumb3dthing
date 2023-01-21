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

class TileGroupGrid;

template <typename T>
class UnfinishedProducableGroup final {
public:
    UnfinishedProducableGroup & at_position(const Vector2I & r) {
        m_positions.push_back(r);
        return *this;
    }

    template <typename ... Types>
    void make_producable(Types && ... args) {
        verify_container_sizes("make_producable");
        m_producables.emplace_back(std::forward<Types>(args)...);
    }

    SharedPtr<ProducableGroup_> finish(Grid<ProducableTile *> & target) {
        verify_finishable("finish");
        class Impl final : public ProducableGroup_ {
        public:
            explicit Impl(std::vector<T> && producables_):
                producables(std::move(producables_)) {}

            std::vector<T> producables;
        };
        auto rv = make_shared<Impl>(std::move(m_producables));
        m_producables.clear();
        for (std::size_t i = 0; i != m_positions.size(); ++i) {
            target(m_positions[i]) = &rv->producables[i];
        }
        m_positions.clear();
        return rv;
    }

private:
    void verify_finishable(const char * caller) const {
        using namespace cul::exceptions_abbr;
        if (m_positions.size() == m_producables.size()) return;
        throw RtError{  "UnfinishedProducableGroup::" + std::string{caller}
                      + ": to finish a group, both every call to 'at_position' "
                        "must be followed by exactly one call to 'make_producable'"};
    }

    void verify_container_sizes(const char * caller) const {
        using namespace cul::exceptions_abbr;
        if (m_positions.size() == m_producables.size() + 1) return;
        throw RtError{  "UnfinishedProducableGroup::" + std::string{caller}
                      + ": at_position must be called exactly once before this "
                        "method is called"};
    }

    std::vector<T> m_producables;
    std::vector<Vector2I> m_positions;
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

    [[nodiscard]] Tuple
        <GridView<ProducableTile *>,
         std::vector<SharedPtr<ProducableGroup_>>>
        move_out_producables_and_groups();

private:
    std::vector<Grid<ProducableTile *>> m_targets;
    std::vector<SharedPtr<ProducableGroup_>> m_groups;
};

class UnfinishedTileGroupGrid final {
public:

    // may only be set once
    void set_size(const Size2I & sz) {
        m_target.set_size(sz, nullptr);
        m_filleds.set_size(sz, false);
    }

    template <typename T>
    void add_group(UnfinishedProducableGroup<T> && unfinished_pgroup) {
        m_groups.emplace_back(unfinished_pgroup.finish(m_target));
    }

    UnfinishedProducableTileGridView
        finish(UnfinishedProducableTileGridView && unfinished_grid_view)
    {
        m_filleds.clear();
        unfinished_grid_view.add_layer(std::move(m_target), m_groups);
        m_target.clear();
        m_groups.clear();
        return std::move(unfinished_grid_view);
    }

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

class TileProperties final {
public:
    using PropertiesMap = std::map<std::string, std::string>;

    TileProperties() {}

    explicit TileProperties(const TiXmlElement & tile_el)
        { load(tile_el); }

    void load(const TiXmlElement & tile_el);

    bool is_empty() const noexcept { return m_id == k_no_id; }

    int id() const { return m_id; }

    const std::string & type() const { return m_type; }

    template <typename KeyType>
    const std::string * find_value(const KeyType & key) const {
        auto itr = m_properties.find(key);
        if (itr == m_properties.end()) return nullptr;
        return &itr->second;
    }

private:
    static constexpr const int k_no_id = -1;

    int m_id = k_no_id;
    std::string m_type;

    PropertiesMap m_properties;
};

// Grid of xml elements, plus info on tileset
class TileSetXmlGrid final {
public:
    void load(Platform &, const TiXmlElement &);

    const TileProperties * operator() (const Vector2I & r) const {
        const auto & props = m_elements(r);
        if (props.is_empty()) return nullptr;
        return &props;
    }

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

    // there's something weird going on with ownership of TiXmlElements
    Grid<TileProperties> m_elements;
    SharedPtr<const Texture> m_texture;
    Size2 m_tile_size;
    Size2 m_texture_size;
};

class TileSet final {
public:
    using FillerFactory = SharedPtr<TileProducableFiller>(*)(const TileSetXmlGrid &, Platform &);
    using FillerFactoryMap = std::map<std::string, FillerFactory>;
    using TileLocation = TileProducableFiller::TileLocation;

    static const FillerFactoryMap & builtin_fillers();

    void load(Platform &, const TiXmlElement &, const FillerFactoryMap & = builtin_fillers());

    SharedPtr<TileProducableFiller> find_filler(int tid) const;

    Vector2I tile_id_to_tileset_location(int tid) const;

    std::size_t total_tile_count() const
        { return m_filler_grid.size(); }

private:
    SharedPtr<TileProducableFiller> find_filler(Vector2I) const;

    Grid<SharedPtr<TileProducableFiller>> m_filler_grid;
};

class SlopedTileFactory;

struct TileLocation final {
    Vector2I on_map;
    Vector2I on_field;
};

// straight up yoinked from tmap
// this is just used wrong I think...
class GidTidTranslator final {
public:
    using ConstTileSetPtr = SharedPtr<const TileSet>;
    using TileSetPtr      = SharedPtr<TileSet>;

    GidTidTranslator() {}

    GidTidTranslator(const std::vector<TileSetPtr> & tilesets, const std::vector<int> & startgids);

    std::pair<int, ConstTileSetPtr> gid_to_tid(int gid) const;

    void swap(GidTidTranslator &);

    std::vector<SharedPtr<const TileSet>> move_out_tilesets() {
        std::vector<SharedPtr<const TileSet>> rv;
        rv.reserve(m_gid_map.size());
        for (auto & tl : m_gid_map) {
            rv.emplace_back(std::move(tl.tileset));
        }
        m_gid_map.clear();
        m_gid_end = 0;
        return rv;
    }

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

    std::vector<GidAndTileSetPtr> m_gid_map;
    int m_gid_end = 0;
};
