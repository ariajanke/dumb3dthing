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
#include "TileSetCollection.hpp"

#include "../ParseHelpers.hpp"
#include "../ProducableGrid.hpp"
#include "../MapRegion.hpp"

#include "../../platform.hpp"

#include <ariajanke/cul/RectangleUtils.hpp>
#include <ariajanke/cul/OptionalEither.hpp>

#include <ariajanke/cul/TypeSet.hpp>
#include <ariajanke/cul/TypeList.hpp>

// types are forwarded only

// I think I'm okay with losing constexpr
template <typename Base>
class ConstructorFunctions final {
public:
    using MoveFunction = Base * (*)(void * destination, void * source);

    using CopyFunction = Base * (*)(void * destination, const void * source);

    template <typename T>
    static const ConstructorFunctions & constructor_functions_for() {
        static_assert(std::is_base_of_v<Base, T>);
        static const ConstructorFunctions funcs
            {copy_function_for<T>(), move_function_for<T>()};
        return funcs;
    }

    ConstructorFunctions() {}

    ConstructorFunctions(CopyFunction copy_, MoveFunction move_):
        m_move(move_), m_copy(copy_) {}

    MoveFunction move_function() const { return m_move; }

    CopyFunction copy_function() const { return m_copy; }

private:
    template <typename T>
    static std::enable_if_t<std::is_copy_constructible_v<T>, CopyFunction>
        copy_function_for()
    {
        return [] (void * destination, const void * source) {
            // destination is assumed to be free
            const T & obj = *reinterpret_cast<const T *>(source);
            return static_cast<Base *>(new (destination) T{obj});
        };
    }

    template <typename T>
    static std::enable_if_t<!std::is_copy_constructible_v<T>, CopyFunction>
        copy_function_for()
    {
        return [] (void *, const void *) -> Base * {
            using namespace cul::exceptions_abbr;
            throw RtError{"Cannot copy this type"};
        };
    }

    template <typename T>
    static MoveFunction move_function_for() {
        return [] (void * destination, void * source) {
            // destination is assumed to be free
            T & obj = *reinterpret_cast<T *>(source);
            return static_cast<Base *>(new (destination) T{std::move(obj)});
        };
    }

    MoveFunction m_move = nullptr;
    CopyFunction m_copy = nullptr;
};


template <typename Base, typename ... Types>
class StateMachineDriver;

template <typename Base>
class RestrictedStateSwitcherBase {
public:
    virtual ~RestrictedStateSwitcherBase() {}

protected:
    using ConstructorFunctions_ = ConstructorFunctions<Base>;

    virtual void * prepare_next_state_space() = 0;

    virtual void assign_next_state_base
        (Base *, const ConstructorFunctions_ &) = 0;
};

template <typename Base, typename ... Types>
class RestrictedStateSwitcherComplete;

template <typename Base, typename ... Types>
class RestrictedStateSwitcher :
    public RestrictedStateSwitcherBase<Base>
{
public:
    using StatesTypeSet = cul::TypeSet<Types...>;

    // need complete types for most things in this class
    using StatesDriver = StateMachineDriver<Base, Types...>;

    using StateSwitcherComplete = RestrictedStateSwitcherComplete<Base, Types...>;

    template <typename T, typename ... ConstructorTypes>
    T & set_next_state(ConstructorTypes &&... constructor_args) {
        // only called when all types are complete
        static_assert(StatesTypeSet::template kt_contains<T>);
        static_assert(std::is_base_of_v<Base, T>);

        using Super = RestrictedStateSwitcherBase<Base>;
        using ConstructorFunctions_ = typename Super::ConstructorFunctions_;

        T * rv = new (this->prepare_next_state_space())
            T{std::forward<ConstructorTypes>(constructor_args)...};
        const auto & funcs =
            ConstructorFunctions_::template constructor_functions_for<T>();
        this->assign_next_state_base(rv, funcs);
        return *rv;
    }
};

// after types are complete

template <typename Base>
class StateEntry final {
public:
    using ConstructorFunctions_ = ConstructorFunctions<Base>;

    explicit StateEntry(void * space): m_space(space) {}

    void set_state
        (Base * base_ptr,
         const ConstructorFunctions_ & constructors)
    {
        m_base = base_ptr;
        m_constructors = &constructors;
    }

    void * space() const
        { return m_space; }

    Base * base_state() const
        { return m_base; }

    const ConstructorFunctions_ * constructors() const
        { return m_constructors; }

    void swap(StateEntry & rhs) {
        std::swap(m_space       , rhs.m_space       );
        std::swap(m_base        , rhs.m_base        );
        std::swap(m_constructors, rhs.m_constructors);
    }

    void move(StateEntry & rhs) {
        // assumed clear
        verify_has_no_state();
        rhs.verify_has_state();
        m_base         = rhs.constructors()->
            move_function()(m_space, rhs.m_space);
        m_constructors = rhs.constructors();
        rhs.clear();
    }

    void copy(const StateEntry & rhs) {
        verify_has_no_state();
        rhs.verify_has_state();
        m_base         = rhs.constructors()->
            copy_function()(m_space, rhs.m_space);
        m_constructors = rhs.constructors();
    }

    void clear() {
        m_base->~Base();
        m_base = nullptr;
        m_constructors = nullptr;
    }

    bool has_state() const
        { return m_base; }

private:
    void verify_has_state() const {
        assert(m_base && m_constructors);
    }

    void verify_has_no_state() const {
        assert(!m_base && !m_constructors);
    }

    void * m_space = nullptr;
    Base * m_base = nullptr;
    const ConstructorFunctions_ * m_constructors = nullptr;
};

template <typename Base, typename ... Types>
class RestrictedStateSwitcherComplete final :
    public RestrictedStateSwitcher<Base, Types...>
{
public:
    static_assert(std::has_virtual_destructor_v<Base>);

    using ConstructorFunctions_ = ConstructorFunctions<Base>;

    using StateEntry_ = StateEntry<Base>;

    explicit RestrictedStateSwitcherComplete(StateEntry_ & entry):
        m_entry(entry) {}

    void * prepare_next_state_space() final {
        m_entry.clear();
        return m_entry.space();
    }

    void assign_next_state_base
        (Base * base_ptr,
         const ConstructorFunctions_ & constructors) final
    { m_entry.set_state(base_ptr, constructors); }

private:
    StateEntry_ & m_entry;
};

template <typename ... Types>
class AlignedSpace final {
public:
    static const constexpr std::size_t k_size      = 0;
    static const constexpr std::size_t k_alignment = 1;

    using SpaceType = std::aligned_storage_t<k_size, k_alignment>;
};

template <typename Head, typename ... Types>
class AlignedSpace<Head, Types...> final {
public:
    static const constexpr std::size_t k_size =
        std::max(AlignedSpace<Types...>::k_size, sizeof(Head));
    static const constexpr std::size_t k_alignment =
        std::max(AlignedSpace<Types...>::k_alignment, alignof(Head));

    using SpaceType = std::aligned_storage_t<k_size, k_alignment>;
};


template <typename Base, typename ... Types>
class StateMachineDriver final {
    using ConstructorFunctions_ = ConstructorFunctions<Base>;
public:
    using StatesSpace = typename AlignedSpace<Types...>::SpaceType;

    using StateSwitcher = RestrictedStateSwitcherComplete<Base, Types...>;

    StateMachineDriver():
        m_current_entry(&m_space_a),
        m_next_entry   (&m_space_b) {}

    StateMachineDriver(const StateMachineDriver & rhs):
        StateMachineDriver()
        { copy(rhs); }

    StateMachineDriver(StateMachineDriver && rhs):
        StateMachineDriver()
        { move(rhs); }

    ~StateMachineDriver()
        { clear(); }

    StateMachineDriver & operator = (const StateMachineDriver & rhs) {
        if (this == &rhs)
            { copy(rhs); }
        return *this;
    }

    StateMachineDriver & operator = (StateMachineDriver && rhs) {
        if (this == &rhs)
            { move(rhs); }
        return *this;
    }

    template <typename T, typename ... ConstructorTypes>
    T & set_current_state(ConstructorTypes &&... constructor_args) {
        using ConstructorFunctions_ = ConstructorFunctions<Base>;

        if (m_current_entry.has_state()) {
            m_current_entry.clear();
        }

        T * state = new (m_current_entry.space())
            T{std::forward<ConstructorTypes>(constructor_args)...};
        m_current_entry.set_state
            (state, ConstructorFunctions_::template constructor_functions_for<T>());
        return *state;
    }

    StateSwitcher state_switcher()
        { return StateSwitcher{m_next_entry}; }

    template <void (Base::*preadvance_member_f)(const Base &)>
    StateMachineDriver & on_advanceable() {
        if (is_advanceable()) {
            (m_next_entry.base_state()->*preadvance_member_f)
                (*m_current_entry.base_state());
        }
        return *this;
    }

    StateMachineDriver & advance() {
        if (is_advanceable()) {
            m_current_entry.swap(m_next_entry);
            m_next_entry.clear();
        }
        return *this;
    }

    bool is_advanceable() const
        { return m_next_entry.has_state(); }

    Base * operator -> () const { return m_current_entry.base_state(); }

    Base & operator * () const { return *m_current_entry.base_state(); }

private:
    using Entry = StateEntry<Base>;

    void move(StateMachineDriver & rhs) {
        m_current_entry.move(rhs.m_current_entry);
        m_next_entry   .move(rhs.m_next_entry   );
    }

    void copy(const StateMachineDriver & rhs) {
        m_current_entry.copy(rhs.m_current_entry);
        m_next_entry   .copy(rhs.m_next_entry   );
    }

    void clear() {
        m_current_entry.clear();
        m_next_entry.clear();
    }

    Entry m_current_entry;
    Entry m_next_entry;
    StatesSpace m_space_a, m_space_b;
};

struct MapLoadingSuccess final {
    UniquePtr<TiledMapRegion> loaded_region;
    MapLoadingWarnings warnings;
};

namespace tiled_map_loading {

class BaseState;
class FileContentsWaitState;
// waits for string contents for each tileset
class InitialDocumentReadState;

// states for composite maps
// ... none atm

// states for producable maps
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

    virtual MapLoadResult update_progress(StateSwitcher &) = 0;

    virtual ~BaseState() {}

    // rename me
    void carry_shared_state_stuff(const BaseState & rhs) {
        m_platform = rhs.m_platform;
        m_unfinished_warnings = rhs.m_unfinished_warnings;
    }

protected:
    Platform & platform() const { return *m_platform; }

    MapLoadingWarningsAdder & warnings_adder() { return m_unfinished_warnings; }

private:
    Platform * m_platform = nullptr;
    UnfinishedMapLoadingWarnings m_unfinished_warnings;
};

class FileContentsWaitState final : public BaseState {
public:
    explicit FileContentsWaitState(FutureStringPtr && future_):
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

    const TiXmlElement & element() const { return *m_tileset_element; }

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
             std::vector<TileSetContent> && finished):
            m_unfinished(std::move(unfinished)),
            m_finished(std::move(finished)) {}

        std::vector<UnfinishedTileSetContent> move_out_unfinished()
            { return std::move(m_unfinished); }

        std::vector<TileSetContent> move_out_finished()
            { return std::move(m_finished); }

    private:
        std::vector<UnfinishedTileSetContent> m_unfinished;
        std::vector<TileSetContent> m_finished;
    };

    TileSetWaitState
        (DocumentOwningNode && document_root,
         std::vector<Grid<int>> && layers,
         std::vector<UnfinishedTileSetContent> && unfinished_tilesets):
        m_document_root(std::move(document_root)),
        m_layers(std::move(layers)),
        m_unfinished_contents(std::move(unfinished_tilesets)) {}

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
    using TileSetAndStartGid = TileMapIdToSetMapping::TileSetAndStartGid;

    static std::vector<TileSetAndStartGid>
        convert_to_tileset_and_start_gids
        (std::vector<TileSetContent> &&,
         Platform & platform);

    static ProducableTileViewGrid make_producable_view_grid
        (std::vector<Grid<int>> &&,
         TileMapIdToSetMapping &&);

    ProducableLoadState
        (DocumentOwningNode && document_root,
         std::vector<Grid<int>> && layers,
         std::vector<TileSetContent> && finished_tilesets);

    MapLoadResult update_progress(StateSwitcher &) final;

private:
    TileMapIdToSetMapping make_tidgid_mapping();

    ProducableTileViewGrid make_producable_view_grid();

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

    MapLoadStateMachine(Platform & platform, const char * filename)
        { initialize_starting_state(platform, filename); }

    void initialize_starting_state
        (Platform & platform, const char * filename)
    {
        auto file_contents_promise = platform.promise_file_contents(filename);
        m_state_driver.
            set_current_state<FileContentsWaitState>
            (std::move(file_contents_promise));
    }

    MapLoadResult update_progress() {
        auto switcher = m_state_driver.state_switcher();
        auto result = m_state_driver.
            on_advanceable<&BaseState::carry_shared_state_stuff>().
            advance()->
            update_progress(switcher);
        if (result.is_empty() && m_state_driver.is_advanceable())
            { return update_progress(); }
        return result;
    }

private:
    using StateSwitcher = BaseState::StateSwitcher;
    // there is a way to dry this up, I just want to see if this works first
    using CompleteStateSwitcher = BaseState::StateSwitcher::StateSwitcherComplete;
    using StateDriver = StateSwitcher::StatesDriver;

    StateDriver m_state_driver;
};

} // end of tiled_map_loading namespace

class MapLoadingStateHolder;
class MapLoadingState;
class MapLoadingWaitingForFileContents;
class MapLoadingWaitingForTileSets;
class MapLoadingReady;
class MapLoadingExpired;

class MapLoadingContext {
public:
    using State = MapLoadingState;
    using WaitingForFileContents = MapLoadingWaitingForFileContents;
    using WaitingForTileSets = MapLoadingWaitingForTileSets;
    using Ready = MapLoadingReady;
    using Expired = MapLoadingExpired;
    using StateHolder = MapLoadingStateHolder;
    using MapLoadResult = OptionalEither<MapLoadingError, MapLoadingSuccess>;

protected:
    MapLoadingContext() {}
};

class MapLoadingState {
public:
    using MapLoadResult = MapLoadingContext::MapLoadResult;

    virtual ~MapLoadingState() {}

    virtual MapLoadResult
        update_progress(MapLoadingStateHolder &)
        { return {}; }

    MapLoadingState & set_others_stuff(MapLoadingState & lhs) const {
        lhs.m_platform = m_platform;
        lhs.m_unfinished_warnings = std::move(m_unfinished_warnings);
        return lhs;
    }

protected:
    MapLoadingState() {}

    explicit MapLoadingState
        (Platform & platform):
        m_platform(&platform) {}

    Platform & platform() const;

    MapLoadingWarningsAdder & warnings_adder() { return m_unfinished_warnings; }

private:
    void verify_shared_set() const;

    Platform * m_platform = nullptr;
    UnfinishedMapLoadingWarnings m_unfinished_warnings;
};

class MapLoadingWaitingForFileContents final : public MapLoadingState {
public:
    MapLoadingWaitingForFileContents
        (Platform & platform, const char * filename):
         MapLoadingState(platform)
    { m_file_contents = platform.promise_file_contents(filename); }

    MapLoadResult update_progress(MapLoadingStateHolder & next_state) final;

private:
    void add_tileset(const TiXmlElement & tileset, UnfinishedTileSetCollection &);

    FutureStringPtr m_file_contents;
};

class MapLoadingWaitingForTileSets final : public MapLoadingState {
public:
    MapLoadingWaitingForTileSets
        (InProgressTileSetCollection && cont_, std::vector<Grid<int>> && layers_):
        m_tilesets_container(make_shared<InProgressTileSetCollection>(std::move(cont_))),
        m_layers(std::move(layers_)) {}

    MapLoadResult update_progress(MapLoadingStateHolder & next_state) final;

private:
    SharedPtr<InProgressTileSetCollection> m_tilesets_container;
    std::vector<Grid<int>> m_layers;
};

class MapLoadingReady final : public MapLoadingState {
public:
    MapLoadingReady
        (TileMapIdToSetMapping && idtrans_,
         std::vector<Grid<int>> && layers_):
        m_tidgid_translator(std::move(idtrans_)),
        m_layers(std::move(layers_)) {}

    MapLoadResult update_progress(MapLoadingStateHolder & next_state) final;

private:
    TileMapIdToSetMapping m_tidgid_translator;
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

    Tuple<StatePtrGetter, StateSpace> move_out_state();

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
    MapLoadResult update_progress();

    bool is_expired() const
        { return std::holds_alternative<Expired>(m_state_space); }

private:
    using StatePtrGetter = State * (*)(StateSpace &);
    StateSpace m_state_space;
    StatePtrGetter m_get_state = nullptr;
};
