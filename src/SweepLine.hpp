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

#include "Defs.hpp"
#include "TriangleSegment.hpp"

template <typename T>
class SweepLineMap;

class SweepLine final {
public:
    using Triangle = TriangleSegment;
    struct Interval final {
        Real min = 0;
        Real max = 1;
    };

    template <typename T, typename TToTriangle>
    static SweepLine make_for(const SweepLineMap<T> &, TToTriangle &&);

    SweepLine() {}

    SweepLine(const Vector & a_, const Vector & b_):
        m_a(a_), m_b(b_) {}

    Interval interval_for(const Triangle &) const;

    Real point_for(const Vector &) const;

private:
    Vector m_a, m_b;
};

// ----------------------------------------------------------------------------
// Not possible
template <typename T>
class SweepLineMap final {
public:
    using Interval = SweepLine::Interval;

private:
    struct Entry final {
        Entry() {}
        Entry(const Interval & intv, const T & obj_):
            min(intv.min), max(intv.max),
            obj(obj_) {}
        Real min;
        Real max;
        T obj;
    };
    using IteratorImpl = typename std::vector<Entry>::const_iterator;

public:
    class Iterator final {
    public:
        explicit Iterator(IteratorImpl impl):
            m_itr(impl) {}

        Iterator & operator ++ () {
            ++m_itr;
            return *this;
        }

        const T & operator * () const { return m_itr->obj; }

        const T * operator -> () const { return &(m_itr->obj); }

        bool operator != (const Iterator & rhs) const noexcept
            { return m_itr != rhs.m_itr; }

        bool operator == (const Iterator & rhs) const noexcept
            { return m_itr == rhs.m_itr; }

        auto operator - (const Iterator & rhs) const
            { return m_itr - rhs.m_itr; }

    private:
        IteratorImpl m_itr;
    };

    void sort();

    void push_entry(const T & obj);

    void push_entry(const Interval & intv, const T & obj);

    template <typename ObjAsTriangle>
    void update_as_triangles(const SweepLine & line, ObjAsTriangle && f);

    cul::View<Iterator> view_for(Real x) const;

    cul::View<Iterator> view_for(Real low, Real high) const;

    cul::View<Iterator> view_for(const Interval & intv) const;

    void remove_all();

    template <typename Pred>
    void remove_if(Pred && should_remove);

    bool is_sorted() const noexcept { return m_sorted; }

    void mark_as_unsorted() { m_sorted = false; }

    std::size_t count() const
        { return m_sorted; }

    Iterator begin() const { return Iterator{m_container.begin()}; }

    Iterator end() const { return Iterator{m_container.end()}; }

private:
    void verify_is_sorted(const char * caller) const;

    bool m_sorted = true;
    std::vector<Entry> m_container;
};

// ----------------------------------------------------------------------------

// for ease of implementation
// no bubbles
// for this application, I maybe able to accept dupelicates
// the goal is thusly: reduce that scan load, check for a collision with the
// smallest resulting displacement
template <typename T>
class SpatialPartitionMap final {
public:
    using Interval = SweepLine::Interval;

private:
    struct Entry final {
        Entry() {}
        Entry(const Interval & intv, const T & obj_):
            min(intv.min), max(intv.max),
            obj(obj_) {}
        Real min;
        Real max;
        T obj;
    };

    using IteratorImpl = typename std::vector<Entry>::const_iterator;

public:
    class Iterator final {
    public:
        explicit Iterator(IteratorImpl impl):
            m_itr(impl) {}

        Iterator & operator ++ () {
            ++m_itr;
            return *this;
        }

        const T & operator * () const { return m_itr->obj; }

        const T * operator -> () const { return &(m_itr->obj); }

        bool operator != (const Iterator & rhs) const noexcept
            { return m_itr != rhs.m_itr; }

        bool operator == (const Iterator & rhs) const noexcept
            { return m_itr == rhs.m_itr; }

        auto operator - (const Iterator & rhs) const
            { return m_itr - rhs.m_itr; }

    private:
        IteratorImpl m_itr;
    };

    // sorts the container, and computes divisions...
    // essentially get the container ready
    void sort();

    template <typename TToTriangle>
    void update_intervals(TToTriangle &&);

    void push_entry(const T & obj);

    cul::View<Iterator> view_for(Real x) const;

    cul::View<Iterator> view_for(Real low, Real high) const;

    cul::View<Iterator> view_for(const Interval & intv) const;

    void set_line(const SweepLine &);

    void remove_all();

    template <typename Pred>
    void remove_if(Pred && should_remove);

    bool is_sorted() const noexcept { return m_sorted; }

    void mark_as_unsorted() { m_sorted = false; }

    std::size_t count() const;

    Iterator begin() const { return Iterator{m_entries.begin()}; }

    Iterator end() const { return Iterator{m_entries.end()}; }

private:
    void verify_is_sorted(const char * caller) const;

    void recompute_division(std::vector<Real> & divisions) const;

    bool m_sorted = true;
    SweepLine m_line;
    std::vector<Real> m_divisions;
    std::vector<IteratorImpl> m_divided_entries;
    std::vector<Entry> m_entries;
};

// ----------------------------------------------------------------------------

template <typename T, typename TToTriangle>
/* static */ SweepLine SweepLine::make_for
    (const SweepLineMap<T> & line_map, TToTriangle && to_triangle)
{
    auto mk_initial_interval = [] {
        Interval intv;
        intv.min = k_inf;
        intv.max = -k_inf;
        return intv;
    };
    auto update_interval = [](Interval & intv, Real x) {
        intv.min = std::min(intv.min, x);
        intv.max = std::max(intv.max, x);
    };
    using std::make_tuple;
    Vector low, high;
    using Func = Real &(*)(Vector &);
    std::array<Func, 3> getters = {
        [] (Vector & r) -> Real & { return r.x; },
        [] (Vector & r) -> Real & { return r.y; },
        [] (Vector & r) -> Real & { return r.z; }
    };
    for (auto get_ : getters) {
        Interval intv = mk_initial_interval();
        for (const T & obj : line_map) {
            Triangle triangle = to_triangle(obj);
            std::array triangle_array{
                triangle.point_a(), triangle.point_b(), triangle.point_c()
            };
            for (auto pt : triangle_array) {
                update_interval(intv, get_(pt));
            }
        }
        get_(low ) = intv.min;
        get_(high) = intv.max;
    }
    return SweepLine{low, high};
}

// ----------------------------------------------------------------------------

template <typename T>
void SweepLineMap<T>::sort() {
    std::sort(m_container.begin(), m_container.end(),
              [](const Entry & lhs, const Entry & rhs)
    { return lhs.min < rhs.min; });
    m_sorted = true;
}

template <typename T>
void SweepLineMap<T>::push_entry(const T & obj)
    { push_entry(Interval{}, obj); }

template <typename T>
void SweepLineMap<T>::push_entry(const Interval & intv, const T & obj) {
    m_container.emplace_back(intv, obj);
    m_sorted = false;
}

template <typename T>
template <typename ObjAsTriangle>
void SweepLineMap<T>::update_as_triangles(const SweepLine & line, ObjAsTriangle && f) {
    using Triangle = TriangleSegment;
    for (auto & entry : m_container) {
        Triangle gv = f(entry.obj);
        auto intv = line.interval_for(gv);
        entry.min = intv.min;
        entry.max = intv.max;
    }
    m_sorted = false;
}

template <typename T>
cul::View<typename SweepLineMap<T>::Iterator> SweepLineMap<T>::view_for
    (Real x) const
{
    static constexpr Real k_error = 0.005;
    Interval intv;
    intv.max = x + k_error;
    intv.min = x - k_error;
    return view_for(intv);
}

template <typename T>
cul::View<typename SweepLineMap<T>::Iterator> SweepLineMap<T>::view_for
    (Real low, Real high) const
{
    Interval intv;
    intv.max = high;
    intv.min = low;
    return view_for(intv);
}

template <typename T>
cul::View<typename SweepLineMap<T>::Iterator> SweepLineMap<T>::view_for
    (const Interval & intv) const
{
    verify_is_sorted("view_for");
    auto low = intv.min;
    auto itr = std::upper_bound(
        m_container.begin(), m_container.end(), low,
        [](Real low, const Entry & entry)
        { return entry.min < low; });

    auto high = intv.max;
    auto last_itr = itr;
    for (; last_itr != m_container.end(); ++last_itr) {
        if (last_itr->min >= high)
            break;
    }

    // turn last_itr -> end_itr
    if (last_itr != m_container.end()) ++last_itr;
    return cul::View<Iterator>{Iterator{itr}, Iterator{last_itr}};
}

template <typename T>
void SweepLineMap<T>::remove_all() {
    m_container.clear();
    m_sorted = true;
}

template <typename T>
template <typename Pred>
void SweepLineMap<T>::remove_if(Pred && should_remove) {
    auto new_end = std::remove_if(
        m_container.begin(), m_container.end(),
        [should_remove = std::move(should_remove)]
        (const Entry & entry)
        { return should_remove(entry.obj); });
    m_container.erase(new_end, m_container.end());
}

template <typename T>
/* private */ void SweepLineMap<T>::verify_is_sorted(const char * caller) const {
    using namespace cul::exceptions_abbr;
    if (m_sorted) return;
    throw RtError{"SweepLineMap::" + std::string{caller} + ": cannot be "
                  "called with an unsorted container"};
}
