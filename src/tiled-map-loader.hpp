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

class StringSplitterIteratorEnd {};

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc>
class StringSplitterIterator {
public:
    StringSplitterIterator(CharIter beg, CharIter end,
                           const SplitterFunc & splitter_,
                           const WithAdditionalFunc & with_):
        m_beg(beg), m_segment_end(beg), m_end(end),
        m_splitter_f(std::move(splitter_)),
        m_with_f(std::move(with_))
    { update_end_segment(); }

    bool operator == (const StringSplitterIteratorEnd &) const noexcept
        { return is_at_end(); }

    bool operator != (const StringSplitterIteratorEnd &) const noexcept
        { return !is_at_end(); }

    StringSplitterIterator & operator ++ () {
        move_to_next();
        update_end_segment();
        return *this;
    }

    cul::View<CharIter> operator * () const {
        auto b = m_beg;
        auto e = m_segment_end;
        m_with_f(b, e);
        return cul::View<CharIter>{b, e};
    }

private:
    bool is_at_end() const noexcept
        { return m_beg == m_end; }

    void update_end_segment()
        { m_segment_end = std::find_if(m_beg, m_end, m_splitter_f); }

    void move_to_next() {
        m_beg = m_segment_end;
        if (m_beg != m_end) ++m_beg;
    }

    CharIter m_beg, m_segment_end, m_end;
    SplitterFunc m_splitter_f;
    WithAdditionalFunc m_with_f;
};

inline auto make_is_comma() { return [](char c) { return c == ','; }; }

inline bool is_whitespace(char c)
    { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }

inline auto make_trim_whitespace() {
    return [] (const char *& beg, const char *& end) {
        cul::trim<is_whitespace>(beg, end);
    };
}

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc>
using SplitRangeView =
    cul::View<StringSplitterIterator<CharIter, SplitterFunc, WithAdditionalFunc>,
              StringSplitterIteratorEnd>;

template <typename CharIter, typename SplitterFunc, typename WithAdditionalFunc>
SplitRangeView<CharIter, SplitterFunc, WithAdditionalFunc>
    split_range(CharIter beg, CharIter end,
                const SplitterFunc & splitter_,
                const WithAdditionalFunc & with_ = [](CharIter &, CharIter &){})
{
    return SplitRangeView<CharIter, SplitterFunc, WithAdditionalFunc> {
        StringSplitterIterator{beg, end, std::move(splitter_), std::move(with_)},
        StringSplitterIteratorEnd{}};
}

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

private:
    void add_tileset(tinyxml2::XMLElement * tileset);

    bool check_has_remaining_pending_tilesets();

    void do_content_load(std::string && contents);

    FutureStringPtr m_file_contents;

    Grid<int> m_layer;
    Platform::ForLoaders * m_platform;

    std::vector<SharedPtr<TileSet>> m_tilesets;
    std::vector<int> m_startgids;

    std::vector<Tuple<std::size_t, FutureStringPtr>> m_pending_tilesets;
    GidTidTranslator m_tidgid_translator;
};
