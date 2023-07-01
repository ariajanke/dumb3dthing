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


template <typename Base, typename ... Types>
class RestrictedStateMachineDriver final {
public:
    using StateTypeSet = cul::TypeSet<Types...>;
private:
    template <typename T>
    using TypeInheritsFromBase = std::conditional_t<std::is_base_of_v<Base, T>, std::true_type, std::false_type>;

    using TrueListInheritingFromBase = typename cul::TypeList<Types...>::template Transform<TypeInheritsFromBase>;
    using StatesVariant = Variant<std::monostate, Types...>;
public:
    static_assert
        (TrueListInheritingFromBase::template
         kt_occurance_count<std::true_type> == sizeof...(Types),
         "All state classes must inherit from Base");

    // have a state switcher that auto advances and one that does not

    class RestrictedStateSwitcher final {
    public:
        RestrictedStateSwitcher
            (Base *& base_ptr,
             StatesVariant *& next,
             StatesVariant *& current):
            m_base_ptr(base_ptr), m_current(current), m_next(next) {}

        ~RestrictedStateSwitcher() { advance(); }

        template <typename T, typename ... ConstructorTypes>
        T & set_next_state(ConstructorTypes &&... constructor_args) {
            *m_next = StatesVariant
                {std::in_place_type_t<T>{},
                 std::forward<ConstructorTypes>(constructor_args)...};
            T * rv = std::get_if<T>(m_next);
            m_base_ptr = rv;
            return rv;
        }

    private:
        void advance() {
            if (std::holds_alternative<std::monostate>(*m_next))
                { return; }
            std::swap(m_current, m_next);
            *m_next = StatesVariant{std::monostate{}};
        }

        Base *& m_base_ptr;
        StatesVariant *& m_current;
        StatesVariant *& m_next   ;
    };

    [[nodiscard]] RestrictedStateSwitcher state_switcher()
        { return RestrictedStateSwitcher{m_base_ptr, m_next, m_current}; }

    template <typename T, typename ... ConstructorTypes>
    T & set_current_state(ConstructorTypes &&... constructor_args) {
        *m_current = StatesVariant
            {std::in_place_type_t<T>{},
             std::forward<ConstructorTypes>(constructor_args)...};
        *m_next = StatesVariant{std::monostate{}};
        auto * ptr = std::get_if<T>(m_current);
        m_base_ptr = ptr;
        return *ptr;
    }

    Base * operator -> () const { return m_base_ptr; }

    Base & operator * () const { return *m_base_ptr; }

private:
    StatesVariant m_left  = std::monostate{};
    StatesVariant m_right = std::monostate{};
    StatesVariant * m_current = &m_left;
    StatesVariant * m_next    = &m_right;
    Base * m_base_ptr = nullptr;
};

#if 0
class StateSwitcherBase {
protected:
    StateSwitcherBase() {}

    template <typename T>
    static std::size_t type_id_for() {
        static std::byte s;
        return reinterpret_cast<std::size_t>(&s);
    }
};

template <typename ... Types>
class StorageFor {
public:
    static constexpr const std::size_t k_total_size  = 0;
    static constexpr const std::size_t k_total_align = 1;

    using Type = std::aligned_storage_t<k_total_size, k_total_align>;
};

template <typename Head, typename ... Types>
class StorageFor<Head, Types...> {
public:
    static constexpr const std::size_t k_total_size =
        std::max(StorageFor<Types...>::k_total_size, sizeof(Head));
    static constexpr const std::size_t k_total_align =
        std::max(StorageFor<Types...>::k_total_align, alignof(Head));

    using Type = std::aligned_storage_t<k_total_size, k_total_align>;
};

template <typename ... Types>
using StorageTypeFor = typename StorageFor<Types...>::Type;

template <typename ... StateTypes>
class TypeRestrictedStateMachine final {
public:
    using StateTypeSet = cul::TypeSet<StateTypes...>;

    class StateSwitcher final : public StateSwitcherBase {
    private:
        virtual void destroy_next_state() = 0;

        virtual void * space_for_next() = 0;
    };

private:
    std::array<StorageTypeFor<StateTypes...>, 2> m_storage;
    StorageTypeFor<StateTypes...> * m_current_state;
    StorageTypeFor<StateTypes...> * m_next_state;
};

// can have different state switcher types
template <typename StateBaseType, typename ... StateTypes>
class RestrictedStateSwitcher {
public:
    using StateTypeSet = cul::TypeSet<StateTypes...>;

    template <typename T, typename ... ConstructorTypes>
    T & set_next_state(ConstructorTypes &&... constructor_args)
    {
        // in the other specialization, we check against the typeset and
        // immediately report an error if we can
        destroy_next_state();
        void * space = space_for_type_id(type_id_for<T>());
        return *new (space) T{std::forward<ConstructorTypes>(constructor_args)...};
    }


protected:
    virtual ~StateSwitcher() {}

    virtual void destroy_next_state() = 0;
    virtual void * space_for_type_id(std::size_t) = 0;
};
#endif
class MapLoadingStateHolder;
class MapLoadingState;
class MapLoadingWaitingForFileContents;
class MapLoadingWaitingForTileSets;
class MapLoadingReady;
class MapLoadingExpired;

struct MapLoadingSuccess final {
    UniquePtr<TiledMapRegion> loaded_region;
    MapLoadingWarnings warnings;
};

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
