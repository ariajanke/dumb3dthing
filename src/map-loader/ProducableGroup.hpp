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

#include "ViewGrid.hpp"

class ProducableTile;
class ProducableGroup_;
class TileSet;
class UnfinishedProducableTileViewGrid;

/// This base class represents how to clean up a tile group.
class ProducableGroup_ {
public:
    virtual ~ProducableGroup_() {}
};

/// The process by which a group of producable tiles are made.
template <typename T>
class UnfinishedProducableGroup final {
public:
    UnfinishedProducableGroup & at_position(const Vector2I & r) {
        m_positions.push_back(r);
        return *this;
    }

    template <typename ... Types>
    void make_producable(Types && ... args) {
        verify_container_sizes("make_producable");
        m_producables.emplace_back(std::forward<Types>(args)...);
    }

    SharedPtr<ProducableGroup_> finish(Grid<ProducableTile *> & target) {
        verify_finishable("finish");
        class Impl final : public ProducableGroup_ {
        public:
            explicit Impl(std::vector<T> && producables_):
                producables(std::move(producables_)) {}

            std::vector<T> producables;
        };
        auto rv = make_shared<Impl>(std::move(m_producables));
        m_producables.clear();
        for (std::size_t i = 0; i != m_positions.size(); ++i) {
            target(m_positions[i]) = &rv->producables[i];
        }
        m_positions.clear();
        return rv;
    }

private:
    void verify_finishable(const char * caller) const {
        using namespace cul::exceptions_abbr;
        if (m_positions.size() == m_producables.size()) return;
        throw RtError{  "UnfinishedProducableGroup::" + std::string{caller}
                      + ": to finish a group, both every call to 'at_position' "
                        "must be followed by exactly one call to 'make_producable'"};
    }

    void verify_container_sizes(const char * caller) const {
        using namespace cul::exceptions_abbr;
        if (m_positions.size() == m_producables.size() + 1) return;
        throw RtError{  "UnfinishedProducableGroup::" + std::string{caller}
                      + ": at_position must be called exactly once before this "
                        "method is called"};
    }

    std::vector<T> m_producables;
    std::vector<Vector2I> m_positions;
};
