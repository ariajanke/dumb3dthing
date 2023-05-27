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

#include "TileSetCollection.hpp"

#include <tinyxml2.h>

namespace {

using TileSetAndStartGid = InProgressTileSetCollection::TileSetAndStartGid;
using LoadEntry = UnfinishedTileSetCollection::LoadEntry;

} // end of <anonymous> namespace

UnfinishedTileSetCollection::LoadEntry::LoadEntry
    (int start_gid_,
     Either<FutureStringPtr, SharedPtr<TileSet>> && either):
    start_gid(start_gid_),
    future (either.left_or (nullptr)),
    tileset(either.right_or(nullptr))
{}

// ----------------------------------------------------------------------------

UnfinishedTileSetCollection::UnfinishedTileSetCollection
    (MapLoadingWarningsAdder & warnings_adder):
    m_warnings_adder(&warnings_adder)
{}

void UnfinishedTileSetCollection::add
    (int start_gid, SharedPtr<TileSet> && tileset_ptr)
{ m_entries.emplace_back(start_gid, std::move(tileset_ptr)); }

void UnfinishedTileSetCollection::add
    (int start_gid, FutureStringPtr && future_ptr)
{ m_entries.emplace_back(start_gid, std::move(future_ptr)); }

void UnfinishedTileSetCollection::add(MapLoadingWarningEnum warning) {
    if (!m_warnings_adder) return;
    m_warnings_adder->add(warning);
}

InProgressTileSetCollection UnfinishedTileSetCollection::finish()
    { return InProgressTileSetCollection{std::move(m_entries)}; }

// ----------------------------------------------------------------------------

InProgressTileSetCollection::InProgressTileSetCollection(LoadEnties && entries):
    m_entries(std::move(entries)) {}

Optional<TileMapIdToSetMapping> InProgressTileSetCollection::
    attempt_finish(Platform & platform)
{
    update_entries(platform);
    auto converted_entries = convert_entries();
    if (!converted_entries)
        return {};

    return TileMapIdToSetMapping{std::move(*converted_entries)};
}

/* private static */ bool InProgressTileSetCollection::entry_contains_tileset
    (const LoadEntry & entry)
{ return !!entry.tileset; }

/* private static */ LoadEntry InProgressTileSetCollection::update_entry
    (Platform & platform, LoadEntry && entry)
{
    using Lost = Future<std::string>::Lost;
    using EitherFutureOrTileSetPtr = LoadEntry::EitherFutureOrTileSetPtr;

    if (!entry.future) return std::move(entry);

    auto ei = (*entry.future)();
    return LoadEntry{entry.start_gid, std::move(ei).
        fold<EitherFutureOrTileSetPtr>( std::move(entry.future) ).
        map([&platform] (std::string && contents) -> EitherFutureOrTileSetPtr {
            auto tsptr = make_shared<TileSet>();
            TiXmlDocument document;
            document.Parse(contents.c_str());
            tsptr->load(platform, *document.RootElement());
            return tsptr;
        }).
        map_left([] (Lost) -> EitherFutureOrTileSetPtr
            { return SharedPtr<TileSet>{}; }).
        value()};
}

/* private */ void InProgressTileSetCollection::update_entries
    (Platform & platform)
{
    for (auto & entry : m_entries) {
        entry = update_entry(platform, std::move(entry));
    }
}

/* private */ Optional<std::vector<TileSetAndStartGid>>
    InProgressTileSetCollection::convert_entries()
{
    if (!std::all_of(m_entries.begin(), m_entries.end(),
                     entry_contains_tileset))
    { return {}; }

    std::vector<TileSetAndStartGid> new_entries;
    new_entries.reserve(m_entries.size());
    for (auto & entry : m_entries) {
        TileSetAndStartGid new_entry;
        new_entry.tileset = entry.tileset;
        new_entry.start_gid = entry.start_gid;
        if (!new_entry.tileset) continue;
        new_entries.emplace_back(std::move(new_entry));
    }
    m_entries.clear();
    return new_entries;
}
