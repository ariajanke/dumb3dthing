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

#include "MapLoadingError.hpp"
#include "TileSet.hpp"

#include "../ParseHelpers.hpp"

#include "../../platform.hpp"
#include "../../Definitions.hpp"

#include <ariajanke/cul/Either.hpp>

struct MapContentLoader;

// ----------------------------------------------------------------------------

class DocumentOwningNode final {
public:
    static Either<MapLoadingError, DocumentOwningNode>
        load_root(std::string && file_contents);

    static OptionalEither<MapLoadingError, DocumentOwningNode>
        optionally_load_root(std::string && file_contents);

    DocumentOwningNode() {}

    DocumentOwningNode make_with_same_owner
        (const TiXmlElement & same_document_element) const;

    const TiXmlElement * operator -> () const { return &element(); }

    const TiXmlElement & operator * () const { return element(); }

    const TiXmlElement & element() const;

    explicit operator bool() const { return m_element; }

private:
    struct Owner {
        virtual ~Owner() {}
    };

    DocumentOwningNode
        (const SharedPtr<Owner> & owner, const TiXmlElement & element_):
        m_owner(owner), m_element(&element_) {}

    SharedPtr<Owner> m_owner;
    const TiXmlElement * m_element = nullptr;
};

// ----------------------------------------------------------------------------

class TileSetProvider {
public:
    virtual ~TileSetProvider() {}

    virtual OptionalEither<MapLoadingError, SharedPtr<TileSetBase>>
        retrieve() = 0;
};

// loading tile set stages
// load the actual document
// load any prerequsite documents (which may include other tile maps

class TileSetLoadingTask final :
    public BackgroundTask, public TileSetProvider
{
public:
    using Readiness = Future<std::string>::Readiness;

    static TileSetLoadingTask begin_loading
        (const char * filename, MapContentLoader & content_provider);

    static TileSetLoadingTask begin_loading
        (DocumentOwningNode && tileset_xml);

    BackgroundTaskCompletion operator () (Callbacks &) final;

    OptionalEither<MapLoadingError, SharedPtr<TileSetBase>>
        retrieve() final;

private:
    struct UnloadedTileSet final {
        UnloadedTileSet() {}

        UnloadedTileSet
            (SharedPtr<TileSetBase> && tile_set_,
             DocumentOwningNode && xml_content_):
            tile_set(std::move(tile_set_)),
            xml_content(std::move(xml_content_)) {}

        SharedPtr<TileSetBase> tile_set;
        DocumentOwningNode xml_content;
    };

    explicit TileSetLoadingTask(FutureStringPtr && content_):
        m_tile_set_content(std::move(content_)) {}

    explicit TileSetLoadingTask(UnloadedTileSet && unloaded_ts_):
        m_unloaded(std::move(unloaded_ts_)) {}

    static Either<MapLoadingError, UnloadedTileSet> get_unloaded
        (FutureStringPtr & tile_set_content);

    UnloadedTileSet m_unloaded;
    SharedPtr<TileSetBase> m_loaded_tile_set;
    FutureStringPtr m_tile_set_content;
    Optional<MapLoadingError> m_loading_error;
};

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

using TileSetLoadersWithStartGid = StartGidWith<TileSetLoadingTask>;
using TileSetProviderWithStartGid = StartGidWith<SharedPtr<TileSetProvider>>;
using TileSetWithStartGid = StartGidWith<SharedPtr<TileSetBase>>;
