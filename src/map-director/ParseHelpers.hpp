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

#include "../Defs.hpp"

#include <ariajanke/cul/StringUtil.hpp>

namespace tinyxml2 {

class XMLDocument;
class XMLElement;

} // end of tinyxml2 namespace

using TiXmlElement  = tinyxml2::XMLElement ;
using TiXmlDocument = tinyxml2::XMLDocument;

class TiXmlIter final {
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = int;
    using value_type = const TiXmlElement &;
    using reference = const TiXmlElement &;
    using pointer = const TiXmlElement *;

    TiXmlIter();
    TiXmlIter(const TiXmlElement * el_, const char * name_);

    TiXmlIter & operator ++ ();
    bool operator != (const TiXmlIter & rhs) const;
    const TiXmlElement & operator * () const;

private:
    const TiXmlElement * el;
    const char * name;
};

class XmlRange final {
public:
    XmlRange(const TiXmlElement * el_, const char * name_);
    XmlRange(const TiXmlElement & el_, const char * name_);

    TiXmlIter begin() const;
    TiXmlIter end()   const;

private:
    TiXmlIter m_beg;
};


class StringSplitterIteratorEnd {};

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc, typename EndIter = CharIter>
class StringSplitterIteratorBase {
protected:
    StringSplitterIteratorBase() {}

    StringSplitterIteratorBase(
        CharIter beg, EndIter end,
        SplitterFunc && splitter_, WithAdditionalFunc && with_):
        m_beg(beg), m_segment_end(beg), m_end(end),
        m_splitter_f(std::move(splitter_)),
        m_with_f(std::move(with_))
    { update_end_segment(); }


    bool is_at_end() const noexcept
        { return m_beg == m_end; }

    void update_end_segment()
        { m_segment_end = std::find_if(m_beg, m_end, m_splitter_f); }

    void move_to_next() {
        m_beg = m_segment_end;
        if (m_beg != m_end) ++m_beg;
    }

    View<CharIter> deref() const {
        auto b = m_beg;
        auto e = m_segment_end;
        m_with_f(b, e);
        return View<CharIter>{b, e};
    }

    bool is_same_as(const StringSplitterIteratorBase & lhs) const noexcept
        { return m_beg == lhs.m_beg && m_segment_end == lhs.m_segment_end; }

    using Element          = decltype(*CharIter{});
    using IteratorCategory = std::forward_iterator_tag;

private:
    template <typename Func>
    using WrapFunction = std::conditional_t<
        std::is_function_v<Func>,
        std::add_pointer_t<Func>,
        Func>;

    CharIter m_beg, m_segment_end;
    EndIter m_end;
    WrapFunction<SplitterFunc> m_splitter_f;
    WrapFunction<WithAdditionalFunc> m_with_f;
};

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc, typename EndIter = CharIter>
class StringSplitterIterator final :
    public StringSplitterIteratorBase<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>
{
    using Super = StringSplitterIteratorBase<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>;
public:
    using difference_type   = int;
    using iterator_category = typename Super::IteratorCategory;
    using value_type        = typename Super::Element;

    StringSplitterIterator() {}

    StringSplitterIterator
        (CharIter beg, EndIter end, SplitterFunc && splitter_,
         WithAdditionalFunc && with_):
        Super(beg, end, std::move(splitter_), std::move(with_))
    {}

    bool operator == (const StringSplitterIteratorEnd &) const noexcept
        { return Super::is_at_end(); }

    bool operator != (const StringSplitterIteratorEnd &) const noexcept
        { return !Super::is_at_end(); }

    bool operator == (const StringSplitterIterator & lhs) const noexcept
        { return is_same_as(lhs); }

    bool operator != (const StringSplitterIterator & lhs) const noexcept
        { return !is_same_as(lhs); }

    StringSplitterIterator & operator ++ () {
        Super::move_to_next();
        Super::update_end_segment();
        return *this;
    }

    View<CharIter> operator * () const { return Super::deref(); }
};


template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc, typename EndIter = CharIter>
class StringSplitterIteratorWithIndex final :
    public StringSplitterIteratorBase<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>
{
    using Super = StringSplitterIteratorBase<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>;
public:
    using difference_type   = int;
    using iterator_category = typename Super::IteratorCategory;
    using value_type        = typename Super::Element;

    StringSplitterIteratorWithIndex() {}

    StringSplitterIteratorWithIndex
        (CharIter beg, EndIter end, SplitterFunc && splitter_,
         WithAdditionalFunc && with_):
        Super(beg, end, std::move(splitter_), std::move(with_))
    {}

    explicit StringSplitterIteratorWithIndex
        (const StringSplitterIterator<CharIter, SplitterFunc, WithAdditionalFunc> & rhs):
        Super(rhs)
    {}

    bool operator == (const StringSplitterIteratorEnd &) const noexcept
        { return Super::is_at_end(); }

    bool operator != (const StringSplitterIteratorEnd &) const noexcept
        { return !Super::is_at_end(); }

    bool operator == (const StringSplitterIteratorWithIndex & lhs) const noexcept
        { return is_same_as(lhs); }

    bool operator != (const StringSplitterIteratorWithIndex & lhs) const noexcept
        { return !is_same_as(lhs); }

    StringSplitterIteratorWithIndex & operator ++ () {
        Super::move_to_next();
        Super::update_end_segment();
        ++m_index;
        return *this;
    }

    Tuple<View<CharIter>, int> operator * () const
        { return make_tuple(Super::deref(), m_index); }

private:
    int m_index = 0;
};

inline bool is_comma(char c) { return c == ','; }

inline auto make_is_comma() { return [](char c) { return c == ','; }; }

inline bool is_whitespace(char c)
    { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }

template <typename CharIter>
inline auto make_trim_whitespace() {
    return [] (CharIter & beg, CharIter & end)
        { cul::trim<is_whitespace>(beg, end); };
}

template <typename CharIter>
inline auto make_trim_nothing()
    { return [](CharIter &, CharIter &) {}; }

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc, typename EndIter = CharIter>
using StringSplitterView = View<StringSplitterIterator<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>, StringSplitterIteratorEnd>;

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc, typename EndIter = CharIter>
using StringSplitterViewWithIndex = View<StringSplitterIteratorWithIndex<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>, StringSplitterIteratorEnd>;

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc, typename EndIter = CharIter>
StringSplitterView<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>
    split_range(CharIter beg, EndIter end, SplitterFunc && splitter_,
                WithAdditionalFunc && with_ = [](CharIter &, CharIter &){})
{
    using Iterator = StringSplitterIterator<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>;
    return View<Iterator, StringSplitterIteratorEnd>{
        Iterator{beg, end, std::move(splitter_), std::move(with_)},
        StringSplitterIteratorEnd{}};
}

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc, typename EndIter = CharIter>
StringSplitterViewWithIndex<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>
    split_range_with_index
    (CharIter beg, EndIter end, SplitterFunc && splitter_,
     WithAdditionalFunc && with_ = [](CharIter &, CharIter &){})
{
    return StringSplitterViewWithIndex<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>{
        StringSplitterIteratorWithIndex<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>
            {beg, end, std::move(splitter_), std::move(with_)},
        StringSplitterIteratorEnd{}};
}
template <typename IterType>
std::string view_to_string(const View<IterType> & view)
    { return std::string{view.begin(), view.end()}; }
