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

namespace tiled_map_loading {

class BaseState;
class FileContentsWaitState;
class InitialDocumentReadState;
class ProducableLoadState;
class TileSetWaitState;
class TiledMapStrategyState;

class ExpiredState;

class BaseState {
public:
    using MapLoadResult = OptionalEither<MapLoadingError, MapLoadingSuccess>;
    using StateSwitcher = RestrictedStateSwitcher
        <BaseState,
         FileContentsWaitState, InitialDocumentReadState, TileSetWaitState,
         TiledMapStrategyState, ProducableLoadState, ExpiredState>;

    // how can I make delays happen?
    // If I do this, it makes a lot of stuff around here way easier
    virtual MapLoadResult update_progress(StateSwitcher &) = 0;

    virtual ~BaseState() {}

    void copy_platform_and_warnings(const BaseState &);

protected:
    BaseState() {}

    explicit BaseState(Platform & platform_):
        m_platform(&platform_) {}

    Platform & platform() const { return *m_platform; }

    MapLoadingWarningsAdder & warnings_adder() { return m_unfinished_warnings; }

private:
    Platform * m_platform = nullptr;
    UnfinishedMapLoadingWarnings m_unfinished_warnings;
};

class FileContentsWaitState final : public BaseState {
public:
    FileContentsWaitState(FutureStringPtr && future_, Platform & platform):
        BaseState(platform),
        m_future_contents(std::move(future_)) {}

    MapLoadResult update_progress(StateSwitcher &) final;

private:
    FutureStringPtr m_future_contents;
};

class TileSetContent;

class DocumentOwningNode final {
public:
    static Either<MapLoadingError, DocumentOwningNode>
        load_root(std::string && file_contents);

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

class InitialDocumentReadState final : public BaseState {
public:
    static std::vector<Grid<int>> load_layers
        (const TiXmlElement & document_root, MapLoadingWarningsAdder &);

    static std::vector<UnfinishedTileSetContent> load_unfinished_tilesets
        (const DocumentOwningNode & document_root,
         MapLoadingWarningsAdder &,
         Platform &);

    explicit InitialDocumentReadState
        (DocumentOwningNode && root_node):
        m_document_root(std::move(root_node)) {}

    InitialDocumentReadState(const InitialDocumentReadState &) = delete;

    InitialDocumentReadState(InitialDocumentReadState &&) = default;

    MapLoadResult update_progress(StateSwitcher &) final;

private:
    DocumentOwningNode m_document_root;
};

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

class ExpiredState final : public BaseState {
public:
    MapLoadResult update_progress(StateSwitcher &) final
        { return {}; }
};

class MapLoadStateMachine final {
public:
    using MapLoadResult = BaseState::MapLoadResult;

    MapLoadStateMachine() {}

    MapLoadStateMachine(Platform &, const char * filename);

    void initialize_starting_state(Platform &, const char * filename);

    MapLoadResult update_progress();

private:
    using StateSwitcher = BaseState::StateSwitcher;
    using CompleteStateSwitcher =
        BaseState::StateSwitcher::StateSwitcherComplete;
    using StateDriver = StateSwitcher::StatesDriver;

    StateDriver m_state_driver;
};

} // end of tiled_map_loading namespace
