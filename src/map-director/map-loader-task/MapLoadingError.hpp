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

#include "../../Definitions.hpp"

#include "../DocumentOwningXmlElement.hpp"

namespace map_loading_messages {

enum WarningEnum {
    k_non_csv_tile_data,
    k_tile_layer_has_no_data_element,
    k_invalid_tile_data
};

enum ErrorEnum {
    k_tile_map_file_contents_not_retrieved
};

} // end of map_loading_messages namespace

class MapLoadingWarnings final {};

using MapLoadingErrorEnum = map_loading_messages::ErrorEnum;

using MapLoadingWarningEnum = map_loading_messages::WarningEnum;

class MapLoadingWarningsAdder {
public:
    virtual ~MapLoadingWarningsAdder() {}

    virtual void add(MapLoadingWarningEnum) = 0;
};

class UnfinishedMapLoadingWarnings final : public MapLoadingWarningsAdder {
public:
    void add(MapLoadingWarningEnum) final {}

    MapLoadingWarnings finish() { return MapLoadingWarnings{}; }

private:
};


class MapLoadingError final {
public:
    static Either<MapLoadingError, DocumentOwningXmlElement>
        failed_load_as_error(Optional<DocumentOwningXmlElement> &&);

    explicit MapLoadingError(MapLoadingErrorEnum) {}
};

