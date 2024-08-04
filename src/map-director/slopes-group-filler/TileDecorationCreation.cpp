/******************************************************************************

    GPLv3 License
    Copyright (c) 2024 Aria Janke

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

#include "TileDecorationCreation.hpp"

#include "../MapElementValuesMap.hpp"
#include "../ProducableGrid.hpp"
#include "../MapObjectSpawner.hpp"

namespace {

using EntityCreator = MapObjectSpawner::EntityCreator;

constexpr const Real k_decoration_chance = 0.2;
constexpr const Real k_tree_change = 0.1;
constexpr const std::array k_leaf_rotations = { 0., k_pi*(2. / 3.),  k_pi*(4. / 3.) };

class ProducableTileProperties final : public MapItemPropertiesRetrieval {
public:
    explicit ProducableTileProperties(ProducableTileCallbacks & callbacks):
        m_callbacks(callbacks),
        m_random_pt_in_tile
            (callbacks.next_random(), 0, callbacks.next_random()),
        m_y_rotation(callbacks.next_random()) {}

    const char * get_string(FieldType, const char *) const final
        { return nullptr; }

    Optional<Vector> get_vector_property(const char * name) const final {
        if (::strcmp(name, "scale") == 0) {
            return Vector{1, 1, 1}; //return m_callbacks.model_scale().value;
        } else if (::strcmp(name, "translation") == 0) {
            return m_callbacks.model_translation().value + m_random_pt_in_tile;
        }
        return {};
    }

private:
    Optional<int> get_integer(FieldType, const char * name) const final
        { return {}; }

    Optional<Real> get_real_number
        (FieldType type, const char * name) const final
    {
        if (type == FieldType::property && ::strcmp(name, "y-rotation") == 0) {
            return m_y_rotation;
        }
        return {};
    }

    const ProducableTileCallbacks & m_callbacks;
    Vector m_random_pt_in_tile;
    Real m_y_rotation;
};

} // end of <anonymous> namespace

/* static */ void TileDecorationCreation::create_tile_decoration_with
    (ProducableTileCallbacks & callbacks)
{ return TileDecorationCreation{callbacks}.created_tile_decoration(); }

TileDecorationCreation::TileDecorationCreation
    (ProducableTileCallbacks & callbacks):
    m_callbacks(callbacks) {}

void TileDecorationCreation::created_tile_decoration() {
    if (random_roll() >= k_decoration_chance)
        { return; }

    auto rl = random_roll();
    if (rl >= k_tree_change) {
        return make_grass();
    }
    return make_tree();
}

/* private*/ void TileDecorationCreation::make_grass() {
    MapObjectSpawner::spawn_grass
        (ProducableTileProperties{m_callbacks},
         EntityCreator::make([&] { return m_callbacks.make_entity(); }),
         m_callbacks.assets_retrieval());
}

/* private*/ void TileDecorationCreation::make_tree() {
    MapObjectSpawner::spawn_tree
        (ProducableTileProperties{m_callbacks},
         EntityCreator::make([&] { return m_callbacks.make_entity(); }),
         m_callbacks.assets_retrieval());
}

/* private */ Real TileDecorationCreation::random_roll() const
    { return m_callbacks.next_random() + 0.5; }
