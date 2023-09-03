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
#include "MapRegionTracker.hpp"
#include "MapRegion.hpp"

class RegionDecayCollector;

class RegionLoadJob final {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    RegionLoadJob() {}

    RegionLoadJob(const Vector2I & on_field_position,
                  const Vector2I & maps_offset,
                  const ProducableSubGrid &,
                  const ScaleComputation &);

    void operator () (MapRegionContainer &,
                      RegionEdgeConnectionsAdder &,
                      LoaderTask::Callbacks &) const;

private:
    Vector2I m_on_field_position;
    Vector2I m_maps_offset;
    ProducableSubGrid m_subgrid;
    ScaleComputation m_scale;
};

class RegionDecayJob final {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    RegionDecayJob
        (const RectangleI & subgrid_bounds,
         SharedPtr<ViewGridTriangle> &&,
         std::vector<Entity> &&);

    void operator () (RegionEdgeConnectionsRemover &,
                      LoaderTask::Callbacks &) const;

private:
    RectangleI m_on_field_subgrid;
    SharedPtr<ViewGridTriangle> m_triangle_grid;
    std::vector<Entity> m_entities;
};

class RegionLoadCollector final : public RegionLoadCollectorBase {
public:
    explicit RegionLoadCollector(MapRegionContainer &);

    void collect_load_job
        (const Vector2I & on_field_position,
         const Vector2I & maps_offset,
         const ProducableSubGrid &,
         const ScaleComputation &) final;

    RegionDecayCollector finish();

private:
    std::vector<RegionLoadJob> m_entries;
    MapRegionContainer & m_container;
};

class RegionDecayCollector final : public MapRegionContainer::RegionDecayAdder {
public:
    using ViewGridTriangle = MapRegionContainer::ViewGridTriangle;

    explicit RegionDecayCollector(std::vector<RegionLoadJob> &&);

    void add(const Vector2I & on_field_position,
             const Size2I & grid_size,
             SharedPtr<ViewGridTriangle> && triangle_links,
             std::vector<Entity> && entities) final;

    SharedPtr<LoaderTask> finish_into_task_with
        (RegionEdgeConnectionsContainer &,
         MapRegionContainer &);

private:
    std::vector<RegionLoadJob> m_load_entries;
    std::vector<RegionDecayJob> m_decay_entries;
};

class MapRegionChangesTask final : public LoaderTask {
public:
    MapRegionChangesTask
        (std::vector<RegionLoadJob> &&,
         std::vector<RegionDecayJob> &&,
         RegionEdgeConnectionsContainer &,
         MapRegionContainer &);

    void operator () (LoaderTask::Callbacks &) const final;

private:
    std::vector<RegionLoadJob> m_load_entries;
    std::vector<RegionDecayJob> m_decay_entries;
    RegionEdgeConnectionsContainer & m_edge_container;
    MapRegionContainer & m_container;
};
