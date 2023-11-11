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

#pragma once

#include "TileMapIdToSetMapping.hpp"
#include "MapLoadingError.hpp"
#include "StateMachineDriver.hpp"
#include "FutureTileSet.hpp"

#include "../ParseHelpers.hpp"
#include "../ProducableGrid.hpp"
#include "../MapRegion.hpp"

#include "../../platform.hpp"

#include <ariajanke/cul/RectangleUtils.hpp>
#include <ariajanke/cul/OptionalEither.hpp>

#include <ariajanke/cul/TypeSet.hpp>
#include <ariajanke/cul/TypeList.hpp>

struct MapLoadingSuccess final {
    UniquePtr<TiledMapRegion> loaded_region;
    MapLoadingWarnings warnings;
};

// still have "unfinished" types
// provide an interface to the outside world for anything loading
// map related content
struct MapContentLoader { // am I a visitor?
    virtual ~MapContentLoader() {}

    /// @returns true if any promised file contents is not immediately ready
    virtual bool delay_required() const = 0;

    virtual FutureStringPtr promise_file_contents(const char *) = 0;

    virtual void add_warning(MapLoadingWarningEnum) = 0;

    virtual SharedPtr<Texture> make_texture() const = 0;

    virtual void wait_on(const SharedPtr<BackgroundTask> &) = 0;
};

namespace tiled_map_loading {

class BaseState;
class FileContentsWaitState;
class InitialDocumentReadState;
class ProducableLoadState;
class TileSetWaitState;
class TiledMapStrategyState;
class TileSetLoadState;
class MapElementCollectorState;

class ExpiredState;

class BaseState {
public:
    using MapLoadResult = OptionalEither<MapLoadingError, MapLoadingSuccess>;
    using StateSwitcher = RestrictedStateSwitcher
        <BaseState,
         FileContentsWaitState, InitialDocumentReadState, TileSetLoadState,
         MapElementCollectorState, ExpiredState>;

    virtual MapLoadResult update_progress
        (StateSwitcher &, MapContentLoader &) = 0;

    virtual ~BaseState() {}
};

class FileContentsWaitState final : public BaseState {
public:
    explicit FileContentsWaitState
        (FutureStringPtr && future_):
        m_future_contents(std::move(future_)) {}

    MapLoadResult update_progress(StateSwitcher &, MapContentLoader &) final;

private:
    FutureStringPtr m_future_contents;
};

class TileSetContent;

class InitialDocumentReadState final : public BaseState {
public:
    struct TileSetLoadSplit final {
        std::vector<TileSetProviderWithStartGid> future_tilesets;
        std::vector<TileSetWithStartGid> ready_tilesets;
    };

    static std::vector<Grid<int>> load_layers
        (const TiXmlElement & document_root, MapContentLoader &);

    static std::vector<TileSetLoadersWithStartGid>
        load_future_tilesets
        (const DocumentOwningNode & document_root, MapContentLoader &);

    static TileSetLoadSplit
        split_tileset_load(std::vector<TileSetLoadersWithStartGid> &&,
                           MapContentLoader & content_loader);

    explicit InitialDocumentReadState
        (DocumentOwningNode && root_node):
        m_document_root(std::move(root_node)) {}

    InitialDocumentReadState(const InitialDocumentReadState &) = delete;

    InitialDocumentReadState(InitialDocumentReadState &&) = default;

    MapLoadResult update_progress(StateSwitcher &, MapContentLoader &) final;

private:
    DocumentOwningNode m_document_root;
};

class TileSetLoadState final : public BaseState {
public:
    using StartGidWithTileSet = StartGidWith<SharedPtr<TileSetBase>>;
    using StartGidWithTileSetContainer = std::vector<StartGidWithTileSet>;

    static void check_for_finished
        (MapContentLoader &,
         TileSetLoadersWithStartGid &,
         StartGidWithTileSetContainer &);

    static std::vector<TileSetWithStartGid> finish_tilesets
        (std::vector<TileSetProviderWithStartGid> && future_tilesets,
         std::vector<TileSetWithStartGid> && ready_tilesets);

    TileSetLoadState
        (DocumentOwningNode && document_root_,
         std::vector<Grid<int>> && layers_,
         std::vector<TileSetProviderWithStartGid> && future_tilesets_,
         std::vector<TileSetWithStartGid> && ready_tilesets_);

    MapLoadResult update_progress(StateSwitcher &, MapContentLoader &) final;

private:
    DocumentOwningNode m_document_root;
    std::vector<Grid<int>> m_layers;
    std::vector<TileSetProviderWithStartGid> m_future_tilesets;
    std::vector<TileSetWithStartGid> m_ready_tilesets;
};

class MapElementCollectorState final : public BaseState {
public:
    MapElementCollectorState
        (DocumentOwningNode &&,
         TileMapIdToSetMapping_New &&,
         std::vector<Grid<int>> &&);

    MapLoadResult update_progress(StateSwitcher &, MapContentLoader &) final;

private:
    ScaleComputation map_scale() const;

    DocumentOwningNode m_document_root;
    TileMapIdToSetMapping_New m_id_mapping_set;
    std::vector<Grid<int>> m_layers;
};

class ExpiredState final : public BaseState {
public:
    MapLoadResult update_progress(StateSwitcher &, MapContentLoader &) final
        { return {}; }
};

class MapLoadStateMachine final {
public:
    using MapLoadResult = BaseState::MapLoadResult;

    static MapLoadStateMachine make_with_starting_state
        (MapContentLoader &, const char * filename);

    MapLoadStateMachine() {}

    void initialize_starting_state(MapContentLoader &, const char * filename);

    MapLoadResult update_progress(MapContentLoader &);

private:
    using StateSwitcher = BaseState::StateSwitcher;
    using CompleteStateSwitcher =
        BaseState::StateSwitcher::StateSwitcherComplete;
    using StateDriver = StateSwitcher::StatesDriver;

    StateDriver m_state_driver;
};

} // end of tiled_map_loading namespace
