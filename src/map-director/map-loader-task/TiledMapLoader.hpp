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
#include "StateMachineDriver.hpp"
#include "GlobalIdTileLayer.hpp"

#include "../MapObjectCollection.hpp"
#include "../MapRegion.hpp"

class TilesetXmlGrid;

struct MapLoadingSuccess final {
    UniquePtr<MapRegion> loaded_region;
    MapObjectCollection object_collection;
    MapObjectFraming object_framing;
    MapLoadingWarnings warnings;
};

namespace tiled_map_loading {

class BaseState;
class FileContentsWaitState;
class InitialDocumentReadState;
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
        std::vector<TilesetProviderWithStartGid> future_tilesets;
        std::vector<TilesetWithStartGid> ready_tilesets;
    };

    static std::vector<GlobalIdTileLayer> load_layers
        (const DocumentOwningXmlElement & document_root, MapContentLoader &);

    static std::vector<TilesetLoadersWithStartGid>
        load_future_tilesets
        (const DocumentOwningXmlElement & document_root, MapContentLoader &);

    static TileSetLoadSplit
        split_tileset_load(std::vector<TilesetLoadersWithStartGid> &&,
                           MapContentLoader & content_loader);

    explicit InitialDocumentReadState
        (DocumentOwningXmlElement && root_node):
        m_document_root(std::move(root_node)) {}

    InitialDocumentReadState(const InitialDocumentReadState &) = delete;

    InitialDocumentReadState(InitialDocumentReadState &&) = default;

    MapLoadResult update_progress(StateSwitcher &, MapContentLoader &) final;

private:
    DocumentOwningXmlElement m_document_root;
};

class TileSetLoadState final : public BaseState {
public:
    using StartGidWithTileSet = StartGidWith<SharedPtr<TilesetBase>>;
    using StartGidWithTileSetContainer = std::vector<StartGidWithTileSet>;

    static std::vector<TilesetWithStartGid> finish_tilesets
        (std::vector<TilesetProviderWithStartGid> && future_tilesets,
         std::vector<TilesetWithStartGid> && ready_tilesets);

    TileSetLoadState
        (DocumentOwningXmlElement && document_root_,
         std::vector<GlobalIdTileLayer> && layers_,
         std::vector<TilesetProviderWithStartGid> && future_tilesets_,
         std::vector<TilesetWithStartGid> && ready_tilesets_);

    MapLoadResult update_progress(StateSwitcher &, MapContentLoader &) final;

private:
    DocumentOwningXmlElement m_document_root;
    std::vector<GlobalIdTileLayer> m_layers;
    std::vector<TilesetProviderWithStartGid> m_future_tilesets;
    std::vector<TilesetWithStartGid> m_ready_tilesets;
};

class MapElementCollectorState final : public BaseState {
public:
    MapElementCollectorState
        (DocumentOwningXmlElement &&,
         TileMapIdToSetMapping &&,
         std::vector<GlobalIdTileLayer> &&);

    MapLoadResult update_progress(StateSwitcher &, MapContentLoader &) final;

private:
    ScaleComputation map_scale() const;

    DocumentOwningXmlElement m_document_root;
    TileMapIdToSetMapping m_id_mapping_set;
    std::vector<GlobalIdTileLayer> m_layers;
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
