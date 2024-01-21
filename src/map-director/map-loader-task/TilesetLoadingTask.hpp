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

#include "../MapObject.hpp"

#include "MapLoadingError.hpp"
#include "TilesetBase.hpp"

class MapContentLoader;
class TilesetBase;

class TilesetProvider {
public:
    virtual ~TilesetProvider() {}

    virtual OptionalEither<MapLoadingError, SharedPtr<TilesetBase>>
        retrieve() = 0;
};

// ----------------------------------------------------------------------------

class TilesetLoadingTask final :
    public BackgroundTask, public TilesetProvider
{
public:
    static TilesetLoadingTask begin_loading
        (const char * filename, MapContentLoader & content_provider);

    static TilesetLoadingTask begin_loading
        (DocumentOwningXmlElement && tileset_xml, MapContentLoader & content_provider);

    Continuation & in_background
        (Callbacks &, ContinuationStrategy &) final;

    OptionalEither<MapLoadingError, SharedPtr<TilesetBase>>
        retrieve() final;

private:
    using FillerFactoryMap = MapContentLoader::FillerFactoryMap;
    struct UnloadedTileSet final {
        UnloadedTileSet() {}

        UnloadedTileSet
            (SharedPtr<TilesetBase> && tile_set_,
             DocumentOwningXmlElement && xml_content_):
            tile_set(std::move(tile_set_)),
            xml_content(std::move(xml_content_)) {}

        SharedPtr<TilesetBase> tile_set;
        DocumentOwningXmlElement xml_content;
    };

    TilesetLoadingTask
        (FutureStringPtr && content_,
         const FillerFactoryMap & filler_map):
        m_tile_set_content(std::move(content_)),
        m_filler_factory_map(&filler_map) {}

    TilesetLoadingTask
        (UnloadedTileSet && unloaded_ts_,
         const FillerFactoryMap & filler_map):
        m_unloaded(std::move(unloaded_ts_)),
        m_filler_factory_map(&filler_map) {}

    static OptionalEither<MapLoadingError, UnloadedTileSet> get_unloaded
        (FutureStringPtr & tile_set_content);

    static OptionalEither<MapLoadingError, DocumentOwningXmlElement>
        optionally_load_root(std::string && file_contents);

    UnloadedTileSet m_unloaded;
    SharedPtr<TilesetBase> m_loaded_tile_set;
    FutureStringPtr m_tile_set_content;
    Optional<MapLoadingError> m_loading_error;
    const FillerFactoryMap * m_filler_factory_map = nullptr;
};

// ----------------------------------------------------------------------------

template <typename T>
struct StartGidWith {
    StartGidWith() {}

    StartGidWith
        (int start_gid_, T && other_):
        other(std::move(other_)),
        start_gid(start_gid_) {}

    T other;
    int start_gid = 0;
};

using TilesetLoadersWithStartGid = StartGidWith<TilesetLoadingTask>;
using TilesetProviderWithStartGid = StartGidWith<SharedPtr<TilesetProvider>>;
using TilesetWithStartGid = StartGidWith<SharedPtr<TilesetBase>>;
