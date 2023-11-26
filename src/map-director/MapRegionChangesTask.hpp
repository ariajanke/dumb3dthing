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

#include "../Definitions.hpp"
#include "../Components.hpp"

#include "ProducableGrid.hpp"
#include "MapRegion.hpp"

class RegionDecayCollector;
class RegionEdgeConnectionsRemover;
class RegionEdgeConnectionsContainer;

class RegionLoadJob final {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    RegionLoadJob() {}

    RegionLoadJob(const SubRegionPositionFraming &, const ProducableSubGrid &);

    void operator () (MapRegionContainer &,
                      RegionEdgeConnectionsAdder &,
                      TaskCallbacks &) const;

private:
    SubRegionPositionFraming m_sub_region_framing;
    ProducableSubGrid m_subgrid;
};

// ----------------------------------------------------------------------------

class RegionDecayJob final {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    RegionDecayJob
        (const Vector2I & on_field_position,
         ScaledTriangleViewGrid &&,
         std::vector<Entity> &&);

    void operator () (RegionEdgeConnectionsRemover &,
                      TaskCallbacks &) const;

private:
    Vector2I m_on_field_position;
    ScaledTriangleViewGrid m_triangle_grid;
    std::vector<Entity> m_entities;
};

// ----------------------------------------------------------------------------

class RegionCollectorBase {
protected:
    static std::vector<RegionDecayJob>
        verify_empty_decay_jobs(std::vector<RegionDecayJob> &&);

    static std::vector<RegionLoadJob>
        verify_empty_load_jobs(std::vector<RegionLoadJob> &&);
};

// ----------------------------------------------------------------------------

class RegionLoadCollector final :
    public RegionLoadCollectorBase,
    public RegionCollectorBase
{
public:
    explicit RegionLoadCollector(MapRegionContainer &);

    RegionLoadCollector
        (std::vector<RegionLoadJob> &&,
         std::vector<RegionDecayJob> &&,
         MapRegionContainer &);

    void collect_load_job
        (const SubRegionPositionFraming &, const ProducableSubGrid &) final;

    RegionDecayCollector finish();

private:
    std::vector<RegionLoadJob> m_entries;
    MapRegionContainer * m_container = nullptr;

    std::vector<RegionDecayJob> m_passed_around_decay_jobs;
};

// ----------------------------------------------------------------------------

class RegionDecayCollector final :
    public MapRegionContainer::RegionDecayAdder,
    public RegionCollectorBase
{
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    RegionDecayCollector
        (std::vector<RegionLoadJob> &&,
         std::vector<RegionDecayJob> &&,
         MapRegionContainer &);

    void add(const Vector2I & on_field_position,
             ScaledTriangleViewGrid && scaled_grid,
             std::vector<Entity> && entities) final;

    RegionLoadCollector run_changes
        (TaskCallbacks &,
         RegionEdgeConnectionsContainer &,
         MapRegionContainer &);

private:
    std::vector<RegionLoadJob> m_load_entries;
    std::vector<RegionDecayJob> m_decay_entries;

    MapRegionContainer * m_passed_around_container = nullptr;
};
