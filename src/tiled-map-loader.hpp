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

#include "platform/platform.hpp"
#include "Components.hpp"
#include "TileSet.hpp"

#include <common/StringUtil.hpp>

namespace tinyxml2 {

class XMLDocument;
class XMLElement;

}

using TiXmlElement  = tinyxml2::XMLElement ;
using TiXmlDocument = tinyxml2::XMLDocument;

class TiXmlIter {
public:
    TiXmlIter();
    TiXmlIter(const TiXmlElement * el_, const char * name_);
    TiXmlIter & operator ++ ();
    bool operator != (const TiXmlIter & rhs) const;
    const TiXmlElement & operator * () const;
private:
    const TiXmlElement * el;
    const char * name;
};

class XmlRange {
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
        SplitterFunc && splitter_, WithAdditionalFunc && with_)
    :
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

    cul::View<CharIter> deref() const {
        auto b = m_beg;
        auto e = m_segment_end;
        m_with_f(b, e);
        return cul::View<CharIter>{b, e};
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

    cul::View<CharIter> operator * () const { return Super::deref(); }
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

    Tuple<cul::View<CharIter>, int> operator * () const
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
using StringSplitterView = cul::View<StringSplitterIterator<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>, StringSplitterIteratorEnd>;

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc, typename EndIter = CharIter>
using StringSplitterViewWithIndex = cul::View<StringSplitterIteratorWithIndex<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>, StringSplitterIteratorEnd>;

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc, typename EndIter = CharIter>
StringSplitterView<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>
    split_range(CharIter beg, EndIter end, SplitterFunc && splitter_,
                WithAdditionalFunc && with_ = [](CharIter &, CharIter &){})
{
    return StringSplitterView<CharIter, SplitterFunc, WithAdditionalFunc, EndIter>{
        StringSplitterIterator{beg, end, std::move(splitter_), std::move(with_)},
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
std::string view_to_string(const cul::View<IterType> & view)
    { return std::string{view.begin(), view.end()}; }

class MapEdgeLinks final {
public:

    class TileRange final {
    public:
        constexpr TileRange () {}

        constexpr TileRange(Vector2I beg_, Vector2I end_):
            m_begin(beg_), m_end(end_) {}

        constexpr Vector2I begin_location() const { return m_begin; }

        constexpr Vector2I end_location() const { return m_end; }

        constexpr TileRange displace(Vector2I r) const
            { return TileRange{m_begin + r, m_end + r}; }

        constexpr bool operator == (const TileRange & rhs) const
            { return is_same_as(rhs); }

        constexpr bool operator != (const TileRange & rhs) const
            { return is_same_as(rhs); }

    private:
        constexpr bool is_same_as(const TileRange & rhs) const
            { return rhs.m_begin == m_begin && rhs.m_end == m_end; }

        Vector2I m_begin, m_end;
    };

    struct MapLinks final {
        std::string filename;
        TileRange range;
    };

    using LinksView = cul::View<const MapLinks *>;
    enum class Side { north, south, east, west };

    MapEdgeLinks() { m_views.fill(make_tuple(k_uninit, k_uninit)); }

    LinksView north_links() const { return links(Side::north); }

    LinksView south_links() const { return links(Side::south); }

    LinksView east_links() const { return links(Side::east); }

    LinksView west_links() const { return links(Side::west); }

    void set_side(Side side, const std::vector<MapLinks> & links) {
        auto idx = static_cast<std::size_t>(side);
        auto & [b, e] = m_views[idx]; {}
        b = m_links.size();
        m_links.insert(m_links.end(), links.begin(), links.end());
        e = m_links.size();
    }

private:
    static constexpr const std::size_t k_uninit = -1;

    LinksView links(Side side) const {
        auto [b, e] = m_views[static_cast<std::size_t>(side)]; {}
        if (b == k_uninit || m_links.empty()) {
            throw std::runtime_error{"Side was not set."};
        }
        auto fptr = &m_links.front();
        return LinksView{fptr + b, fptr + e};
    }

    std::vector<MapLinks> m_links;
    std::array<Tuple<std::size_t, std::size_t>, 4> m_views;
};

class MapLoader final {
public:
    class TeardownTask final : public OccasionalTask {
    public:
        TeardownTask() {}

        explicit TeardownTask(std::vector<Entity> && entities):
            m_entities(std::move(entities)) {}

        void on_occasion(Callbacks &) final {
            for (auto & ent : m_entities)
                { ent.request_deletion(); }
        }

    private:
        std::vector<Entity> m_entities;
    };

    explicit MapLoader(Platform::ForLoaders & platform):
        m_platform(&platform) {}

    Tuple<SharedPtr<LoaderTask>, SharedPtr<TeardownTask>> operator ()
        (const Vector2I & offset);

    void start_preparing(const char * filename)
        { m_file_contents = m_platform->promise_file_contents(filename); }

    // whooo accessors and further methods

    int width() const noexcept // not in tiles, but ues, implementation and interface happen to match for now
        { return m_layer.width(); }

    int height() const noexcept
        { return m_layer.height(); }

    auto northern_maps() const { return m_links.north_links(); }

    // names?

    auto southern_maps() const { return m_links.south_links(); }

    auto eastern_maps() const { return m_links.east_links(); }

    auto western_maps() const { return m_links.west_links(); }

private:
    void add_tileset(const tinyxml2::XMLElement & tileset);

    bool check_has_remaining_pending_tilesets();

    void do_content_load(std::string && contents);

    FutureStringPtr m_file_contents;

    Grid<int> m_layer;
    Platform::ForLoaders * m_platform;

    std::vector<SharedPtr<TileSet>> m_tilesets;
    std::vector<int> m_startgids;

    std::vector<Tuple<std::size_t, FutureStringPtr>> m_pending_tilesets;
    GidTidTranslator m_tidgid_translator;

    MapEdgeLinks m_links;
};

inline MapEdgeLinks::TileRange operator + (const MapEdgeLinks::TileRange & lhs, Vector2I rhs)
    { return lhs.displace(rhs); }

inline MapEdgeLinks::TileRange operator + (Vector2I rhs, const MapEdgeLinks::TileRange & lhs)
    { return lhs.displace(rhs); }
