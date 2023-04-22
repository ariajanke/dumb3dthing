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

#include "WallTileFactory.hpp"

class SingleModelSlopedTileFactory : public SlopesBasedTileFactory {
public:
    void operator ()
        (EntityAndTrianglesAdder &, const SlopeGroupNeighborhood &,
         Platform &) const final;

    Slopes tile_elevations() const final
        { return translate_y(model_tile_elevations(), translation().y); }

protected:
    Entity make_entity(Platform & platform, Vector2I r) const
        { return TranslatableTileFactory::make_entity(platform, r, m_render_model); }

    virtual Slopes model_tile_elevations() const = 0;

    void setup_(const TileProperties &, Platform &,
                const SlopeFillerExtra &,
                const Vector2I & location_on_tileset) override;

private:
    SharedPtr<const RenderModel> m_render_model;
};

class RampTileFactory : public SingleModelSlopedTileFactory {
public:
    template <typename T>
    static Tuple<const std::vector<Vector> &, const std::vector<unsigned> &>
        get_model_positions_and_elements_(const Slopes & slopes);

protected:
    virtual void set_direction(const char *) = 0;

private:
    void setup_(const TileProperties & properties, Platform & platform,
                const SlopeFillerExtra &,
                const Vector2I & location_on_tileset) final;
};

class CornerRampTileFactory : public RampTileFactory {
protected:
    CornerRampTileFactory() {}

    Slopes model_tile_elevations() const final
        { return m_slopes; }

    virtual Slopes non_rotated_slopes() const = 0;

    void set_direction(const char * dir) final;

private:
    Slopes m_slopes;
};

class InRampTileFactory final : public CornerRampTileFactory {
    Slopes non_rotated_slopes() const final
        { return Slopes{1, 1, 1, 0}; }
};

class OutRampTileFactory final : public CornerRampTileFactory {
    Slopes non_rotated_slopes() const final
        { return Slopes{0, 0, 0, 1}; }
};

class TwoRampTileFactory final : public RampTileFactory {
    Slopes model_tile_elevations() const final
        { return m_slopes; }

    void set_direction(const char * dir) final;

    Slopes m_slopes;
};

class FlatTileFactory final : public SingleModelSlopedTileFactory {
    Slopes model_tile_elevations() const final
        { return Slopes{0, 0, 0, 0}; }
};

// ----------------------------------------------------------------------------

template <typename T>
/* static */ Tuple<const std::vector<Vector> &, const std::vector<unsigned> &>
    RampTileFactory::get_model_positions_and_elements_(const Slopes & slopes)
{
    // force unique per class
    static std::vector<Vector> s_positions;
    if (!s_positions.empty()) {
        return Tuple<const std::vector<Vector> &, const std::vector<unsigned> &>
            {s_positions, get_common_elements()};
    }
    auto pts = get_points_for(slopes);
    s_positions = std::vector<Vector>{pts.begin(), pts.end()};
    return get_model_positions_and_elements_<T>(slopes);
}
