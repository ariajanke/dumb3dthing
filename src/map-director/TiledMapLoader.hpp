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

#include "MapRegion.hpp"
#include "GidTidTranslator.hpp"

#include <ariajanke/cul/RectangleUtils.hpp>

#include <optional>

class MapLoadingStateHolder;
class MapLoadingState;
class MapLoadingWaitingForFileContents;
class MapLoadingWaitingForTileSets;
class MapLoadingReady;
class MapLoadingExpired;

template <typename T>
using Optional = std::optional<T>;

class MapLoadingContext {
public:
    using State = MapLoadingState;
    using WaitingForFileContents = MapLoadingWaitingForFileContents;
    using WaitingForTileSets = MapLoadingWaitingForTileSets;
    using Ready = MapLoadingReady;
    using Expired = MapLoadingExpired;
    using StateHolder = MapLoadingStateHolder;
    using OptionalTileViewGrid = Optional<ProducableTileViewGrid>;

protected:
    MapLoadingContext() {}
};

class MapLoadingState {
public:
    using OptionalTileViewGrid = MapLoadingContext::OptionalTileViewGrid;

    virtual ~MapLoadingState() {}

    virtual OptionalTileViewGrid
        update_progress(MapLoadingStateHolder &)
        { return {}; }

    MapLoadingState & set_others_stuff(MapLoadingState & lhs) const {
        lhs.m_platform = m_platform;
        lhs.m_offset = m_offset;
#       if 0
        lhs.m_tiles_to_load = m_tiles_to_load;
#       endif
        return lhs;
    }

protected:
    struct TileSetsContainer final {
        std::vector<int> startgids;
        std::vector<SharedPtr<TileSet>> tilesets;
        std::vector<Tuple<std::size_t, FutureStringPtr>> pending_tilesets;
    };

    MapLoadingState() {}

    MapLoadingState
        (Platform & platform, const Vector2I & offset):
        m_platform(&platform),
        m_offset(offset) {}

    Platform & platform() const;

    Vector2I map_offset() const;

private:
    void verify_shared_set() const;

    Platform * m_platform = nullptr;
    Vector2I m_offset;
};

class MapLoadingWaitingForFileContents final : public MapLoadingState {
public:
    MapLoadingWaitingForFileContents
        (Platform & platform,
         const char * filename, const Vector2I & offset):
         MapLoadingState(platform, offset)
    { m_file_contents = platform.promise_file_contents(filename); }

    OptionalTileViewGrid
        update_progress(MapLoadingStateHolder & next_state) final;

private:
    void add_tileset(const TiXmlElement & tileset, TileSetsContainer &);

    FutureStringPtr m_file_contents;
};

class MapLoadingWaitingForTileSets final : public MapLoadingState {
public:
    MapLoadingWaitingForTileSets
        (TileSetsContainer && cont_, std::vector<Grid<int>> && layers_):
        m_tilesets_container(std::move(cont_)),
        m_layers(std::move(layers_)) {}

    OptionalTileViewGrid update_progress(MapLoadingStateHolder & next_state) final;

private:
    TileSetsContainer m_tilesets_container;
    std::vector<Grid<int>> m_layers;
    GidTidTranslator m_tidgid_translator;
};

class MapLoadingReady final : public MapLoadingState {
public:
    MapLoadingReady
        (
        GidTidTranslator && idtrans_,
        std::vector<Grid<int>> && layers_):
        m_tidgid_translator(std::move(idtrans_)),
        m_layers(std::move(layers_)) {}

    OptionalTileViewGrid update_progress(MapLoadingStateHolder & next_state) final;

private:
    GidTidTranslator m_tidgid_translator;
    std::vector<Grid<int>> m_layers;
};

class MapLoadingExpired final : public MapLoadingState {
public:
    MapLoadingExpired() {}
};

class MapLoadingStateHolder final : public MapLoadingContext {
public:
    using StateSpace = Variant<
        WaitingForFileContents,
        WaitingForTileSets,
        Ready, Expired>;

    using StatePtrGetter = State * (*)(StateSpace &);

    template <typename NextState, typename ... Types>
    NextState & set_next_state(Types && ...args) {
        m_space = NextState{std::forward<Types>(args)...};
        m_get_state = [](StateSpace & space) -> State *
            { return &std::get<NextState>(space); };
        return std::get<NextState>(m_space);
    }

    bool has_next_state() const noexcept;

    void move_state(StatePtrGetter & state_getter_ptr, StateSpace & space);

private:
    StatePtrGetter m_get_state = nullptr;
    StateSpace m_space = MapLoadingExpired{};
};

/// loads a TiledMap asset file
class TiledMapLoader final : public MapLoadingContext {
public:
    using StateHolder = MapLoadingStateHolder;
    using StateSpace = Variant
        <WaitingForFileContents, WaitingForTileSets, Ready, Expired>;

    template <typename ... Types>
    TiledMapLoader
        (Types && ... args):
         m_state_space(WaitingForFileContents{ std::forward<Types>(args)... }),
         m_get_state([] (StateSpace & space) -> State * { return &std::get<WaitingForFileContents>(space); })
    {}

    // return instead a grid of tile factories
    // (note: that grid must own tilesets)
    OptionalTileViewGrid update_progress();

    bool is_expired() const
        { return std::holds_alternative<Expired>(m_state_space); }

private:
    using StatePtrGetter = State * (*)(StateSpace &);
    StateSpace m_state_space;
    StatePtrGetter m_get_state = nullptr;
};
