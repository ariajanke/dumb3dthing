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

#include "ParseHelpers.hpp"

#include <map>

class Platform;

class TileProperties final {
public:
    using PropertiesMap = std::map<std::string, std::string>;

    TileProperties() {}

    explicit TileProperties(const TiXmlElement & tile_el)
        { load(tile_el); }

    void load(const TiXmlElement & tile_el);

    bool is_empty() const noexcept { return m_id == k_no_id; }

    int id() const { return m_id; }

    const std::string & type() const { return m_type; }

    template <typename KeyType, typename Func>
    void for_value(const KeyType & key, Func && f) const {
        class Dummy final {};
        (void)for_value(key, Dummy{}, [&f] (const auto & value) {
            f(value);
            return Dummy{};
        });
    }

    template <typename KeyType, typename DefaultType, typename Func>
    DefaultType for_value
        (const KeyType & key, DefaultType && def, Func && f) const
    {
        auto itr = m_properties.find(key);
        if (itr == m_properties.end())
            return std::move(def);

        const std::string & value = itr->second;
        return f(value);
    }

private:
    static constexpr const int k_no_id = -1;

    int m_id = k_no_id;
    std::string m_type;

    PropertiesMap m_properties;
};

// ----------------------------------------------------------------------------

// this can't be loading, it's a tileset/filler thing
// Grid of xml elements, plus info on tileset
class TilesetXmlGrid final {
public:
    static Vector2I tid_to_tileset_location(const Size2I &, int tid);

    template <typename T>
    static Vector2I tid_to_tileset_location(const Grid<T> & grid, int tid)
        { return tid_to_tileset_location(grid.size2(), tid); }

    void load(SharedPtr<Texture> &&, const TiXmlElement &);

    const TileProperties & operator() (const Vector2I & r) const
        { return m_elements(r); }

    Size2 tile_size() const
        { return m_tile_size; }

    Size2 texture_size() const
        { return m_texture_size; }

    SharedPtr<const Texture> texture() const
        { return m_texture; }

    Vector2I next(const Vector2I & r) const
        { return m_elements.next(r); }

    Vector2I end_position() const
        { return m_elements.end_position(); }

    Size2I size2() const
        { return m_elements.size2(); }

    auto size() const { return m_elements.size(); }

    bool has_position(const Vector2I & r) const
        { return m_elements.has_position(r); }

private:
    static Tuple<SharedPtr<const Texture>, Size2>
        load_texture(Platform &, const TiXmlElement &);

    // there's something weird going on with ownership of TiXmlElements
    Grid<TileProperties> m_elements;
    SharedPtr<const Texture> m_texture;
    Size2 m_tile_size;
    Size2 m_texture_size;
};
