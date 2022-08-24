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
#include "PointAndPlaneDriver.hpp"

#include "platform/platform.hpp"

#include <common/SubGrid.hpp>

using cul::ConstSubGrid;

struct AppearanceId {
    int id = 0;
protected:
    AppearanceId() {}
    AppearanceId(int id_): id(id_) {}
};

struct VoidSpace final {};

struct Pit final {};

struct EndOfRow final {};

struct Slopes final : public AppearanceId {
    Slopes() {}

    Slopes(int id_, float ne_, float nw_, float sw_, float se_):
        AppearanceId(id_),
        nw(nw_), ne(ne_), sw(sw_), se(se_) {}

    bool operator == (const Slopes & rhs) const noexcept
        { return are_same(rhs); }

    bool operator != (const Slopes & rhs) const noexcept
        { return !are_same(rhs); }

    bool are_same(const Slopes & rhs) const noexcept {
        using Fe = std::equal_to<float>;
        return    id == rhs.id && Fe{}(nw, rhs.nw) && Fe{}(ne, rhs.ne)
               && Fe{}(sw, rhs.sw) && Fe{}(se, rhs.se);
    }

    float nw, ne, sw, se;
};

struct Flat final : public AppearanceId {
    Flat() {}
    Flat(int id_, float y_): AppearanceId(id_), y(y_) {}
    float y;
};

using Cell = Variant<VoidSpace, Pit, Slopes, Flat>;

using CellSubGrid = ConstSubGrid<Cell>;

class TileGraphicGenerator {
public:
    using WallDips = std::array<float, 4>;
    using TriangleVec = std::vector<SharedPtr<TriangleSegment>>;
    using EntityVec = std::vector<Entity>;

    TileGraphicGenerator(EntityVec &, TriangleVec &, Platform::ForLoaders &);

    void setup();

    void create_slope(Vector2I, const Slopes &);

    void create_flat(Vector2I, const Flat &, const WallDips &);

    // Can't think of a method fast than O(n^2), though n is always 4 in this
    // case
    // A non real number is returned if there is no such rotation
    static Real rotation_between(const Slopes & rhs, const Slopes & lhs);

    static Slopes sub_minimum_value(const Slopes &);

    std::size_t triangle_count() const noexcept
        { return m_triangles_out.size(); }

    cul::View<TriangleVec::iterator> triangles_view()
        { return cul::View{ m_triangles_out.begin(), m_triangles_out.end() }; }

    std::vector<Entity> give_entities()
        { return std::move(m_entities_out); }

private:
    struct SlopesHasher final {
        std::size_t operator () (const Slopes & slopes) const noexcept {
            std::hash<float> hf{};
            return hf(slopes.ne) ^ hf(slopes.nw) ^ hf(slopes.se) ^ hf(slopes.sw) ^ std::hash<int>{}(slopes.id);
        }
    };

    struct SlopesEquality final {
        bool operator () (const Slopes & rhs, const Slopes & lhs) const {
            return cul::is_real(TileGraphicGenerator::rotation_between(rhs, lhs));
        }
    };

    SharedPtr<Texture> & ensure_texture(SharedPtr<Texture> &, const char * filename);

    Tuple<Slopes, SharedPtr<RenderModel>,
          SharedPtr<TriangleSegment>, SharedPtr<TriangleSegment>>
        get_slope_model_(const Slopes & slopes, const Vector & translation);

    EntityVec & m_entities_out;
    TriangleVec & m_triangles_out;
    Platform::ForLoaders & m_platform;

    SharedPtr<Texture> m_ground_texture, m_tileset4;

    // these things are unable to destruct themselves before the library closes!
    SharedPtr<RenderModel> m_flat_model, m_wall_model;
    std::unordered_map<Slopes, SharedPtr<RenderModel>, SlopesHasher, SlopesEquality> m_slopes_map;
};

class CharToCell {
public:
    using MaybeCell = Variant<VoidSpace, Pit, Slopes, Flat, EndOfRow>;

    virtual ~CharToCell() {}

    MaybeCell operator () (char c) const;

    static Cell to_cell(const MaybeCell &);

    static MaybeCell to_maybe_cell(const Cell &);

    static const CharToCell & default_instance();

protected:
    virtual Cell to_cell(char) const = 0;
};

Tuple<std::vector<TriangleLinks>,
      std::vector<Entity>       > load_map_graphics
    (TileGraphicGenerator &, CellSubGrid);

Grid<Cell> load_map_cell(const char * layout, const CharToCell &);

void run_map_loader_tests();
