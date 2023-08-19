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

#include "ViewGrid.hpp"

#include <unordered_map>

struct Vector2IHasher final {
    std::size_t operator () (const Vector2I & r) const {
        using IntHash = std::hash<int>;
        return IntHash{}(r.x) ^ IntHash{}(r.y);
    }
};

class MapRegionContainer final {
public:
    using ViewGridTriangle =  ViewGrid<SharedPtr<TriangleLink>>;

    struct RegionDecayAdder {
        virtual ~RegionDecayAdder() {}

        virtual void add(const Vector2I & on_field_position,
                         const Size2I & grid_size,
                         SharedPtr<ViewGridTriangle> &&,
                         std::vector<Entity> &&) = 0;
    };

    class RegionRefresh final {
    public:
        explicit RegionRefresh(bool & flag): m_flag(flag) {}

        void keep_this_frame() { m_flag = true; }

    private:
        bool & m_flag;
    };

    Optional<RegionRefresh> region_refresh_at(const Vector2I & on_field_position);

    void decay_regions(RegionDecayAdder &);

    void set_region(const Vector2I & on_field_position,
                    const SharedPtr<ViewGridTriangle> & triangle_grid,
                    std::vector<Entity> && entities);

private:
    struct LoadedMapRegion {
        Size2I region_size;
        std::vector<Entity> entities;
        SharedPtr<ViewGridTriangle> triangle_links;
        bool keep_on_refresh = true;
    };

    using LoadedRegionMap = std::unordered_map
        <Vector2I, LoadedMapRegion, Vector2IHasher>;

    LoadedMapRegion * find(const Vector2I &);

    LoadedRegionMap m_loaded_regions;
};
