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
         MapElementCollectorState,
         ExpiredState
#        if 0
         , TileSetWaitState,
         TiledMapStrategyState, ProducableLoadState
#        endif
         >;

    // how can I make delays happen?
    // If I do this, it makes a lot of stuff around here way easier
    virtual MapLoadResult update_progress
        (StateSwitcher &, MapContentLoader &) = 0;

    virtual ~BaseState() {}
#   if 0
    void copy_platform_and_warnings(const BaseState &);
#   endif
protected:
    BaseState() {}
#   if 0
    explicit BaseState(FileContentProvider & provider_):
        m_content_provider(&provider_) {}

    FileContentProvider & content_provider() const { return *m_content_provider; }

    MapLoadingWarningsAdder & warnings_adder() { return m_unfinished_warnings; }

private:
    FileContentProvider * m_content_provider = nullptr;
    UnfinishedMapLoadingWarnings m_unfinished_warnings;
#   endif
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
#if 0
class DocumentOwningNode final {
public:
    static Either<MapLoadingError, DocumentOwningNode>
        load_root(std::string && file_contents);

    static OptionalEither<MapLoadingError, DocumentOwningNode>
        optionally_load_root(std::string && file_contents);

    DocumentOwningNode() {}

    DocumentOwningNode make_with_same_owner
        (const TiXmlElement & same_document_element) const;

    const TiXmlElement * operator -> () const { return &element(); }

    const TiXmlElement & operator * () const { return element(); }

    const TiXmlElement & element() const;

    explicit operator bool() const { return m_element; }

private:
    struct Owner {
        virtual ~Owner() {}
    };

    DocumentOwningNode
        (const SharedPtr<Owner> & owner, const TiXmlElement & element_):
        m_owner(owner), m_element(&element_) {}

    SharedPtr<Owner> m_owner;
    const TiXmlElement * m_element = nullptr;
};
#endif
#if 0
class UnfinishedTileSetContent final {
public:
    using Lost = Future<std::string>::Lost;

    static Optional<UnfinishedTileSetContent> load
        (const DocumentOwningNode & tileset,
         Platform & platform,
         MapLoadingWarningsAdder &);

    static bool finishable(const UnfinishedTileSetContent & content)
        { return content.is_finished(); }

    UnfinishedTileSetContent() {}

    Either<Lost, Optional<TileSetContent>> update();

    bool is_finished() const;

private:
    UnfinishedTileSetContent(int first_gid, FutureStringPtr &&);

    UnfinishedTileSetContent(int first_gid, const DocumentOwningNode &);

    Optional<TileSetContent> finish();

    Optional<TileSetContent> finish(DocumentOwningNode &&);

    int m_first_gid = 0;
    FutureStringPtr m_future_string;
    DocumentOwningNode m_tileset_element;
};

class TileSetContent final {
public:
    TileSetContent(int first_gid_, DocumentOwningNode && contents_):
        m_first_gid(first_gid_), m_tileset_element(std::move(contents_)) {}

    int first_gid() const { return m_first_gid; }

    const TiXmlElement & as_element() const { return *m_tileset_element; }

private:
    int m_first_gid;
    DocumentOwningNode m_tileset_element;
};
#endif
class InitialDocumentReadState final : public BaseState {
public:
    struct TileSetLoadSplit final {
        std::vector<TileSetProviderWithStartGid> future_tilesets;
        std::vector<TileSetWithStartGid> ready_tilesets;
    };

    static std::vector<Grid<int>> load_layers
        (const TiXmlElement & document_root, MapContentLoader &);
#   if 0
    static std::vector<UnfinishedTileSetContent> load_unfinished_tilesets
        (const DocumentOwningNode & document_root,
         MapLoadingWarningsAdder &,
         Platform &);
#   endif
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
#   if 0
    static std::vector<FutureTileSetWithStartGid>
        remove_done_futures(std::vector<FutureTileSetWithStartGid> &&);
#   endif

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
#   if 0
    void check_for_finished(MapContentLoader &);
#   endif

    DocumentOwningNode m_document_root;
    std::vector<Grid<int>> m_layers;
    std::vector<TileSetProviderWithStartGid> m_future_tilesets;
    std::vector<TileSetWithStartGid> m_ready_tilesets;
#   if 0
    std::vector<TileSetLoadersWithStartGid> m_future_tilesets;
    StartGidWithTileSetContainer m_tilesets;
#   endif
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

#if 0
class TileSetWaitState final : public BaseState {
public:
    class UpdatedContainers final {
    public:
        static UpdatedContainers update
            (std::vector<UnfinishedTileSetContent> && unfinished,
             std::vector<TileSetContent> && finished,
             MapLoadingWarningsAdder &);

        UpdatedContainers
            (std::vector<UnfinishedTileSetContent> && unfinished,
             std::vector<TileSetContent> && finished);

        std::vector<UnfinishedTileSetContent> move_out_unfinished();

        std::vector<TileSetContent> move_out_finished();

    private:
        std::vector<UnfinishedTileSetContent> m_unfinished;
        std::vector<TileSetContent> m_finished;
    };

    TileSetWaitState
        (DocumentOwningNode && document_root,
         std::vector<Grid<int>> && layers,
         std::vector<UnfinishedTileSetContent> &&);

    TileSetWaitState(const TileSetWaitState &) = delete;

    TileSetWaitState(TileSetWaitState &&) = default;

    MapLoadResult update_progress(StateSwitcher &) final;

private:
    DocumentOwningNode m_document_root;
    std::vector<Grid<int>> m_layers;
    std::vector<UnfinishedTileSetContent> m_unfinished_contents;
    std::vector<TileSetContent> m_finished_contents;
};

// preverbial fork in the states road
// State class names should tell me roughly what it does
// and not "where in progress" it is
// is this a strategy? a factory? a what?
class TiledMapStrategyState final : public BaseState {
public:
    TiledMapStrategyState
        (DocumentOwningNode && document_root,
         std::vector<Grid<int>> && layers,
         std::vector<TileSetContent> && finished_tilesets);

    MapLoadResult update_progress(StateSwitcher &) final;

private:
    DocumentOwningNode m_document_root;
    std::vector<Grid<int>> m_layers;
    std::vector<TileSetContent> m_finished_contents;
};

class ProducableLoadState final : public BaseState {
public:
    using TileSetAndStartGid = TileMapIdToSetMapping_New::TileSetAndStartGid;

    static TileSetAndStartGid
        contents_to_producables_with_start_gid
        (TileSetContent &&,
         Platform & platform);

    static std::vector<TileSetAndStartGid>
        convert_to_tileset_and_start_gids
        (std::vector<TileSetContent> &&,
         Platform & platform);

    ProducableLoadState
        (DocumentOwningNode && document_root,
         std::vector<Grid<int>> && layers,
         std::vector<TileSetContent> && finished_tilesets);

    MapLoadResult update_progress(StateSwitcher &) final;

private:
    ScaleComputation map_scale() const;

    DocumentOwningNode m_document_root;
    std::vector<Grid<int>> m_layers;
    std::vector<TileSetContent> m_finished_contents;
};
#endif
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
