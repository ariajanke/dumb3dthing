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

#include "TilesetBase.hpp"

/// Tilesets map tileset ids to tile group fillers.
///
/// maybe a loader thing
class ProducablesTileset final : public TilesetBase {
public:
    static const FillerFactoryMap & builtin_fillers();

    Continuation & load
        (const TiXmlElement &, MapContentLoader &) final;

    void add_map_elements
        (TilesetMapElementCollector &, const TilesetLayerWrapper & mapping_view) const final;

private:
    Size2I size2() const final { return m_filler_grid.size2(); }

    // TODO
    // kind of ugly in general, reveals the need for refactoring of filler
    // classes, BUT not doing yet
    std::map<
        SharedPtr<ProducableGroupFiller>,
        std::vector<TileLocation>>
        make_fillers_and_locations(const TilesetLayerWrapper &) const;

    Grid<SharedPtr<ProducableGroupFiller>> m_filler_grid;
    std::vector<SharedPtr<const ProducableGroupFiller>> m_unique_fillers;
};
