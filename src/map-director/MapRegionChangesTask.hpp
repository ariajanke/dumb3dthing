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

#include "../Defs.hpp"
#include "../Components.hpp"

#include "ProducableGrid.hpp"
#include "MapRegionTracker.hpp"

class RegionDecayCollector;

class RegionLoadJob final {
public:
    using ProducableSubGrid = ProducableTileViewGrid::SubGrid;

    RegionLoadJob() {}

    RegionLoadJob(const Vector2I & on_field_position,
                  const Vector2I & maps_offset,
                  const ProducableSubGrid &);

    void operator () (MapRegionContainer &,
                      RegionEdgeConnectionsAdder &,
                      LoaderTask::Callbacks &) const;

private:
    Vector2I m_on_field_position;
    Vector2I m_maps_offset;
    ProducableSubGrid m_subgrid;
};

class RegionDecayJob final {
public:
    RegionDecayJob
        (const RectangleI & subgrid_bounds,
         std::vector<SharedPtr<TriangleLink>> &&,
         std::vector<Entity> &&);

    void operator () (RegionEdgeConnectionsRemover &,
                      LoaderTask::Callbacks &) const;

private:
    RectangleI m_on_field_subgrid;
    std::vector<SharedPtr<TriangleLink>> m_triangle_links;
    std::vector<Entity> m_entities;
};

// finish into a decay collector
class RegionLoadCollector final : public RegionLoadCollectorBase {
public:
    explicit RegionLoadCollector(MapRegionContainer &);

    void add_tiles
        (const Vector2I & on_field_position,
         const Vector2I & maps_offset,
         const ProducableSubGrid &);

    RegionDecayCollector finish();

private:
    std::vector<RegionLoadJob> m_entries;
    MapRegionContainer & m_container;
};

class RegionDecayCollector final : public MapRegionContainer::RegionDecayAdder {
public:
    explicit RegionDecayCollector(std::vector<RegionLoadJob> &&);

    void add(const Vector2I & on_field_position,
             const Size2I & grid_size,
             std::vector<SharedPtr<TriangleLink>> && triangle_links,
             std::vector<Entity> && entities) final;

    SharedPtr<LoaderTask> finish
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
