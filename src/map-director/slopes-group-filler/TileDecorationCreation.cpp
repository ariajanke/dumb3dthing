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

#include "../../AssetsRetrieval.hpp"

namespace {

constexpr const Real k_decoration_chance = 0.2;
constexpr const Real k_tree_change = 0.1;

} // end of <anonymous> namespace

/* static */ Entity TileDecorationCreation::create_tile_decoration_with
    (ProducableTileCallbacks & callbacks)
{ return TileDecorationCreation{callbacks}.created_tile_decoration(); }

TileDecorationCreation::TileDecorationCreation(ProducableTileCallbacks & callbacks):
    m_callbacks(callbacks),
    m_assets_retrieval(callbacks.assets_retrieval())
{}

Entity TileDecorationCreation::created_tile_decoration() const {
    if (random_roll() >= k_decoration_chance)
        return Entity{};
    auto ent = m_callbacks.add_entity();
    const Vector random_pt_in_tile{ random_position(), 0, random_position() };
    ent.get<ModelTranslation>() += random_pt_in_tile;
    auto rl = random_roll();
    if (rl >= k_tree_change) {
        TupleBuilder{}.
            add(m_assets_retrieval.make_grass_model()).
            add(m_assets_retrieval.make_ground_texture()).
            add(YRotation{k_pi*2*random_position()}).
            add_to_entity(ent);
    } else {
        // TODO DRY me
        auto model = m_assets_retrieval.make_vaguely_tree_like_model();
        auto tx = m_assets_retrieval.make_ground_texture();
        auto base_rot = k_pi*2*random_position();

        TupleBuilder{}.
            add(SharedPtr<const RenderModel>{model}).
            add(SharedPtr<const Texture>{tx}).
            add(YRotation{base_rot}).
            add_to_entity(ent);

        auto leaves = m_assets_retrieval.make_vaguely_palm_leaves();

        for (auto rot : { base_rot, base_rot + k_pi*(2. / 3.), base_rot + k_pi*(4. / 3.) }) {
            ent = m_callbacks.add_entity();
            ent.get<ModelTranslation>() += random_pt_in_tile;
            TupleBuilder{}.
                add(SharedPtr<const RenderModel>{leaves}).
                add(SharedPtr<const Texture>{tx}).
                add(YRotation{rot}).
                add_to_entity(ent);
        }
    }
    return ent;
}

/* private*/ Real TileDecorationCreation::random_position() const
    { return m_callbacks.next_random(); }

/* private */ Real TileDecorationCreation::random_roll() const
    { return m_callbacks.next_random() + 0.5; }
