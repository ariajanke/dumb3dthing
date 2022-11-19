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

#include "../platform.hpp"
#include "../Components.hpp"
#include "TileSet.hpp"
#include "ParseHelpers.hpp"

#include <ariajanke/cul/StringUtil.hpp>

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

    using LinksView = View<const MapLinks *>;
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

    explicit MapLoader(Platform & platform):
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
    void add_tileset(const TiXmlElement & tileset);

    bool check_has_remaining_pending_tilesets();

    void do_content_load(std::string && contents);

    FutureStringPtr m_file_contents;

    Grid<int> m_layer;
    Platform * m_platform;

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
