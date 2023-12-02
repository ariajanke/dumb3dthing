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

#include <cassert>

template <typename Base>
class ConstructorFunctions final {
public:
    using MoveFunction = Base * (*)(void * destination, void * source);

    using CopyFunction = Base * (*)(void * destination, const void * source);

    template <typename T>
    static const ConstructorFunctions & constructor_functions_for();

    ConstructorFunctions() {}

    ConstructorFunctions(CopyFunction copy_, MoveFunction move_):
        m_move(move_), m_copy(copy_) {}

    const MoveFunction & move_function() const { return m_move; }

    const CopyFunction & copy_function() const { return m_copy; }

private:
    template <typename T>
    static std::enable_if_t<std::is_copy_constructible_v<T>, CopyFunction>
        copy_function_for();

    template <typename T>
    static std::enable_if_t<!std::is_copy_constructible_v<T>, CopyFunction>
        copy_function_for();

    template <typename T>
    static MoveFunction move_function_for();

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
    T & set_next_state(ConstructorTypes &&... constructor_args);
};

// after types are complete

template <typename Base>
class StateEntry final {
public:
    using ConstructorFunctions_ = ConstructorFunctions<Base>;

    explicit StateEntry(void * space): m_space(space) {}

    void set_state
        (Base * base_ptr,
         const ConstructorFunctions_ & constructors);

    void * space() const { return m_space; }

    Base * base_state() const { return m_base; }

    const ConstructorFunctions_ * constructors() const;

    void swap(StateEntry & rhs);

    void move(StateEntry & rhs);

    void copy(const StateEntry & rhs);

    void clear();

    bool has_state() const { return m_base; }

private:
    void verify_has_state() const
        { assert(m_base && m_constructors); }

    void verify_has_no_state() const
        { assert(!m_base && !m_constructors); }

    void checkless_clear();

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

    explicit RestrictedStateSwitcherComplete(StateEntry_ & entry);

    void * prepare_next_state_space() final;

    void assign_next_state_base
        (Base * base_ptr,
         const ConstructorFunctions_ & constructors) final;

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

    StateMachineDriver();

    StateMachineDriver(const StateMachineDriver & rhs);

    StateMachineDriver(StateMachineDriver && rhs);

    ~StateMachineDriver();

    StateMachineDriver & operator = (const StateMachineDriver & rhs);

    StateMachineDriver & operator = (StateMachineDriver && rhs);

    template <typename T, typename ... ConstructorTypes>
    T & set_current_state(ConstructorTypes &&... constructor_args);

    StateSwitcher state_switcher();

    template <void (Base::*preadvance_member_f)(const Base &)>
    StateMachineDriver & on_advanceable();

    StateMachineDriver & advance();

    bool is_advanceable() const;

    Base * operator -> () const { return m_current_entry.base_state(); }

    Base & operator * () const { return *m_current_entry.base_state(); }

private:
    using Entry = StateEntry<Base>;

    void move(StateMachineDriver & rhs);

    void copy(const StateMachineDriver & rhs);

    void clear();

    Entry m_current_entry;
    Entry m_next_entry;
    StatesSpace m_space_a, m_space_b;
};

// ----------------------------------------------------------------------------

template <typename Base>

template <typename T>
/* static */ const ConstructorFunctions<Base> &
    ConstructorFunctions<Base>::constructor_functions_for()
{
    static_assert(std::is_base_of_v<Base, T>);
    static const ConstructorFunctions funcs
        {copy_function_for<T>(), move_function_for<T>()};
    return funcs;
}

template <typename Base>
template <typename T>
/* private static */
    std::enable_if_t<std::is_copy_constructible_v<T>,
                     typename ConstructorFunctions<Base>::CopyFunction>
    ConstructorFunctions<Base>::copy_function_for()
{
    return [] (void * destination, const void * source) {
        // destination is assumed to be free
        const T & obj = *reinterpret_cast<const T *>(source);
        return static_cast<Base *>(new (destination) T{obj});
    };
}

template <typename Base>
template <typename T>
/* private static */
    std::enable_if_t<!std::is_copy_constructible_v<T>,
                     typename ConstructorFunctions<Base>::CopyFunction>
    ConstructorFunctions<Base>::copy_function_for()
{
    return [] (void *, const void *) -> Base * {
        throw std::runtime_error{"Cannot copy this type"};
    };
}

template <typename Base>
template <typename T>
/* private static */
    typename ConstructorFunctions<Base>::MoveFunction
    ConstructorFunctions<Base>::move_function_for()
{
    return [] (void * destination, void * source) {
        // destination is assumed to be free
        T & obj = *reinterpret_cast<T *>(source);
        return static_cast<Base *>(new (destination) T{std::move(obj)});
    };
}

// ----------------------------------------------------------------------------

// call only when types are complete
template <typename Base, typename ... Types>
template <typename T, typename ... ConstructorTypes>
    T & RestrictedStateSwitcher<Base, Types...>::
    set_next_state(ConstructorTypes &&... constructor_args)
{
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

// ----------------------------------------------------------------------------

template <typename Base>
void StateEntry<Base>::set_state
    (Base * base_ptr,
     const ConstructorFunctions_ & constructors)
{
    m_base = base_ptr;
    m_constructors = &constructors;
}

template <typename Base>
const typename StateEntry<Base>::ConstructorFunctions_ *
    StateEntry<Base>::constructors() const
    { return m_constructors; }

template <typename Base>
void StateEntry<Base>::swap(StateEntry & rhs) {
    std::swap(m_space       , rhs.m_space       );
    std::swap(m_base        , rhs.m_base        );
    std::swap(m_constructors, rhs.m_constructors);
}

template <typename Base>
void StateEntry<Base>::move(StateEntry & rhs) {
    clear();
    if (!rhs.has_state()) return;
    m_base = rhs.constructors()->move_function()(m_space, rhs.m_space);
    m_constructors = rhs.constructors();
    rhs.checkless_clear();
}

template <typename Base>
void StateEntry<Base>::copy(const StateEntry & rhs) {
    clear();
    if (!rhs.has_state()) return;
    m_base = rhs.constructors()->copy_function()(m_space, rhs.m_space);
    m_constructors = rhs.constructors();
}

template <typename Base>
void StateEntry<Base>::clear() {
    if (!has_state()) return;
    checkless_clear();
}

template <typename Base>
/* private */ void StateEntry<Base>::checkless_clear() {
    m_base->~Base();
    m_base = nullptr;
    m_constructors = nullptr;
}

// ----------------------------------------------------------------------------

template <typename Base, typename ... Types>
RestrictedStateSwitcherComplete<Base, Types...>::
    RestrictedStateSwitcherComplete(StateEntry_ & entry):
    m_entry(entry) {}

template <typename Base, typename ... Types>
void * RestrictedStateSwitcherComplete<Base, Types...>::
    prepare_next_state_space()
{
    m_entry.clear();
    return m_entry.space();
}

template <typename Base, typename ... Types>
void RestrictedStateSwitcherComplete<Base, Types...>::
    assign_next_state_base
    (Base * base_ptr,
     const ConstructorFunctions_ & constructors)
{ m_entry.set_state(base_ptr, constructors); }

// ----------------------------------------------------------------------------

template <typename Base, typename ... Types>
StateMachineDriver<Base, Types...>::StateMachineDriver():
    m_current_entry(&m_space_a),
    m_next_entry   (&m_space_b) {}

template <typename Base, typename ... Types>
StateMachineDriver<Base, Types...>::
    StateMachineDriver(const StateMachineDriver & rhs):
    StateMachineDriver()
    { copy(rhs); }

template <typename Base, typename ... Types>
StateMachineDriver<Base, Types...>::
    StateMachineDriver(StateMachineDriver && rhs):
    StateMachineDriver()
    { move(rhs); }

template <typename Base, typename ... Types>
StateMachineDriver<Base, Types...>::~StateMachineDriver()
    { clear(); }

template <typename Base, typename ... Types>
StateMachineDriver<Base, Types...> &
    StateMachineDriver<Base, Types...>::operator =
    (const StateMachineDriver & rhs)
{
    if (this != &rhs)
        { copy(rhs); }
    return *this;
}

template <typename Base, typename ... Types>
StateMachineDriver<Base, Types...> &
    StateMachineDriver<Base, Types...>::operator =
    (StateMachineDriver && rhs)
{
    if (this != &rhs)
        { move(rhs); }
    return *this;
}

template <typename Base, typename ... Types>
template <typename T, typename ... ConstructorTypes>
T & StateMachineDriver<Base, Types...>::set_current_state
    (ConstructorTypes &&... constructor_args)
{
    using ConstructorFunctions_ = ConstructorFunctions<Base>;

    m_current_entry.clear();

    T * state = new (m_current_entry.space())
        T{std::forward<ConstructorTypes>(constructor_args)...};
    m_current_entry.set_state
        (state, ConstructorFunctions_::template constructor_functions_for<T>());
    return *state;
}

template <typename Base, typename ... Types>
typename StateMachineDriver<Base, Types...>::StateSwitcher
    StateMachineDriver<Base, Types...>::state_switcher()
    { return StateSwitcher{m_next_entry}; }

template <typename Base, typename ... Types>
template <void (Base::*preadvance_member_f)(const Base &)>
StateMachineDriver<Base, Types...> &
    StateMachineDriver<Base, Types...>::on_advanceable()
{
    if (is_advanceable()) {
        (m_next_entry.base_state()->*preadvance_member_f)
            (*m_current_entry.base_state());
    }
    return *this;
}

template <typename Base, typename ... Types>
StateMachineDriver<Base, Types...> &
    StateMachineDriver<Base, Types...>::advance()
{
    if (is_advanceable()) {
        m_current_entry.swap(m_next_entry);
        m_next_entry.clear();
    }
    return *this;
}

template <typename Base, typename ... Types>
bool StateMachineDriver<Base, Types...>::is_advanceable() const
    { return m_next_entry.has_state(); }

template <typename Base, typename ... Types>
/* private */ void StateMachineDriver<Base, Types...>::
    move(StateMachineDriver & rhs)
{
    m_current_entry.move(rhs.m_current_entry);
    m_next_entry   .move(rhs.m_next_entry   );
}

template <typename Base, typename ... Types>
/* private */ void StateMachineDriver<Base, Types...>::
    copy(const StateMachineDriver & rhs)
{
    m_current_entry.copy(rhs.m_current_entry);
    m_next_entry   .copy(rhs.m_next_entry   );
}

template <typename Base, typename ... Types>
/* private */ void StateMachineDriver<Base, Types...>::clear() {
    m_current_entry.clear();
    m_next_entry   .clear();
}
