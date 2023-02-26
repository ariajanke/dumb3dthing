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
    class MakerPosition final {
    public:
        MakerPosition(Vector2I target,
                      std::vector<Vector2I> & locations,
                      std::vector<T> & producables);

        template <typename ... Types>
        void make_producable(Types && ... args);

    private:
        Vector2I m_target;
        std::vector<Vector2I> & m_locations;
        std::vector<T> & m_producables;
    };

    [[nodiscard]] MakerPosition at_location(const Vector2I & on_map)
        { return MakerPosition{on_map, m_positions, m_producables}; }

    SharedPtr<ProducableGroup_> finish(Grid<ProducableTile *> & target);

private:
    std::vector<T> m_producables;
    std::vector<Vector2I> m_positions;
};

// ----------------------------------------------------------------------------

template <typename T>
UnfinishedProducableGroup<T>::MakerPosition::MakerPosition
    (Vector2I target, std::vector<Vector2I> & locations,
     std::vector<T> & producables):
    m_target     (target),
    m_locations  (locations),
    m_producables(producables) {}

template <typename T>
template <typename ... Types>
void UnfinishedProducableGroup<T>::MakerPosition::make_producable(Types && ... args) {
    m_locations.push_back(m_target);
    m_producables.emplace_back(std::forward<Types>(args)...);
}

// ----------------------------------------------------------------------------

template <typename T>
SharedPtr<ProducableGroup_> UnfinishedProducableGroup<T>::finish(Grid<ProducableTile *> & target) {
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
