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

#include "TileSet.hpp"
#include "MapLoadingError.hpp"
#include "TileMapIdToSetMapping.hpp"

#include "../../platform.hpp"

#include <ariajanke/cul/Either.hpp>

class InProgressTileSetCollection;

class UnfinishedTileSetCollection final {
public:
    struct LoadEntry final {
        using EitherFutureOrTileSetPtr =
            Either<FutureStringPtr, SharedPtr<TileSet>>;

        LoadEntry(int start_gid_, EitherFutureOrTileSetPtr &&);

        int start_gid;
        FutureStringPtr future;
        SharedPtr<TileSet> tileset;
    };
    using LoadEnties = std::vector<LoadEntry>;

    UnfinishedTileSetCollection() {}

    UnfinishedTileSetCollection(MapLoadingWarningsAdder & adder);

    void add(int start_gid, SharedPtr<TileSet> &&);

    void add(int start_gid, FutureStringPtr &&);

    void add(MapLoadingWarningEnum);

    InProgressTileSetCollection finish();

private:
    LoadEnties m_entries;
    MapLoadingWarningsAdder * m_warnings_adder = nullptr;
};

class InProgressTileSetCollection final {
public:
    using LoadEnties = UnfinishedTileSetCollection::LoadEnties;
    using LoadEntry  = UnfinishedTileSetCollection::LoadEntry;
    using TileSetAndStartGid = TileMapIdToSetMapping::TileSetAndStartGid;

    explicit InProgressTileSetCollection(LoadEnties &&);

    Optional<TileMapIdToSetMapping> attempt_finish(Platform &);

private:
    static bool entry_contains_tileset(const LoadEntry & entry);

    static LoadEntry update_entry(Platform &, LoadEntry &&);

    void update_entries(Platform &);

    Optional<std::vector<TileSetAndStartGid>> convert_entries();

    LoadEnties m_entries;
};
