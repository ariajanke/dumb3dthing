/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "../../../src/map-director/slopes-group-filler/SplitWallGeometry.hpp"

class TestStripTriangleCollection final :
    public LinearStripTriangleCollection
{
public:
    static TestStripTriangleCollection & instance() {
        static TestStripTriangleCollection inst;
        return inst;
    }

    struct AddedTriangle final {
        AddedTriangle() {}

        AddedTriangle
            (const TriangleSegment & t,
             ToPlanePositionFunction f):
            triangle(t),
            plane_position_function(f) {}

        TriangleSegment triangle;
        ToPlanePositionFunction plane_position_function = nullptr;
    };

    void add_triangle
        (const TriangleSegment & tri, ToPlanePositionFunction f) final
    { m_added_triangles.emplace_back(tri, f); }

    void add_triangle(const StripTriangle & tri) final
        { m_strip_triangles.push_back(tri); }

    static bool triangle_points_match
        (const TriangleSegment & lhs, const TriangleSegment & rhs)
    {
        return lhs.point_a() == rhs.point_a() &&
               lhs.point_b() == rhs.point_b() &&
               lhs.point_c() == rhs.point_c();
    }

    bool has_triangle_added(const TriangleSegment & triangle) const {
        return std::any_of
            (m_added_triangles.begin(),
             m_added_triangles.end(),
             [triangle](const AddedTriangle & added)
             {
                return triangle_points_match(triangle, added.triangle);
             }) ||
            std::any_of
            (m_strip_triangles.begin(),
             m_strip_triangles.end(),
             [triangle](const StripTriangle & added)
             {
                return triangle_points_match(triangle, added.to_triangle_segment());
             });
    }

private:
    std::vector<AddedTriangle> m_added_triangles;
    std::vector<StripTriangle> m_strip_triangles;
};
