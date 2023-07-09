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

#include <ariajanke/cul/TypeSet.hpp>
#include <ariajanke/cul/Util.hpp>

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
        rhs.checkless_clear();
    }

    void copy(const StateEntry & rhs) {
        verify_has_no_state();
        rhs.verify_has_state();
        m_base         = rhs.constructors()->
            copy_function()(m_space, rhs.m_space);
        m_constructors = rhs.constructors();
    }

    void clear() {
        if (!has_state()) return;
        checkless_clear();
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

    void checkless_clear() {
        m_base->~Base();
        m_base = nullptr;
        m_constructors = nullptr;
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

        m_current_entry.clear();

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
        m_next_entry   .clear();
    }

    Entry m_current_entry;
    Entry m_next_entry;
    StatesSpace m_space_a, m_space_b;
};
