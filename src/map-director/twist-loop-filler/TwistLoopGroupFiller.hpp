#pragma once

#include "../ProducableTileFiller.hpp"

// how does tile generation work here?
// only load in geometry relevant to a tile? (yes)

class TileSetXmlGrid;
class TwistTileGroup {
public:
    using Rectangle = cul::Rectangle<int>;

    virtual ~TwistTileGroup() {}

    virtual void load(const Rectangle &, const TileSetXmlGrid &, Platform &) = 0;

    virtual void operator ()
        (const Vector2I & position_in_group, const Vector2I & tile_offset,
         EntityAndTrianglesAdder &, Platform &) const = 0;
};

// some intermediate class...?
class TwoWayTwistTileGroup : public TwistTileGroup {
public:
    void load(const Rectangle &, const TileSetXmlGrid &, Platform &) final;

    void operator ()
        (const Vector2I & position_in_group, const Vector2I & tile_offset,
         EntityAndTrianglesAdder &, Platform &) const final;

private:
    Grid<SharedPtr<const RenderModel>> m_group_models;
};

class TwistLoopTile final : public ProducableTile {
public:
    void operator () (const Vector2I & maps_offset,
                      EntityAndTrianglesAdder & adder, Platform & platform) const final
        { (*m_twist_group)(m_position_in_group, m_position_in_map + maps_offset, adder, platform); }

private:
    Vector2I m_position_in_map;
    Vector2I m_position_in_group;
    SharedPtr<TwistTileGroup> m_twist_group;
};

class TwistLoopGroupFiller final : public ProducableTileFiller {
public:
    void load(const TileSetXmlGrid & xml_grid, Platform & platform);

    UnfinishedTileGroupGrid operator ()
        (const std::vector<TileLocation> &, UnfinishedTileGroupGrid &&) const final;
};
