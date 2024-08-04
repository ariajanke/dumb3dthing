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

#include "MapObjectSpawner.hpp"

#include "../AssetsRetrieval.hpp"

namespace {

constexpr const std::array k_leaf_rotations = { 0., k_pi*(2. / 3.),  k_pi*(4. / 3.) };

ModelScale mk_scale(const MapItemPropertiesRetrieval & props) {
    return ModelScale{props.
        get_vector_property("scale").
        value_or(Vector{1, 1, 1})};
}

ModelTranslation mk_translation(const MapItemPropertiesRetrieval & props) {
    return ModelTranslation{props.
        get_vector_property("translation").
        value_or(Vector{})};
}

Real get_rotation(const MapItemPropertiesRetrieval & props) {
    return props.
        get_numeric_property<Real>("y-rotation").
        value_or(0.);
}

} // end of <anonymous> namespace

/* static */ void MapObjectSpawner::spawn_tree
    (const MapItemPropertiesRetrieval & props,
     const EntityCreator & create_entity,
     AssetsRetrieval & assets_retrieval)
{
    auto make_ent = [&create_entity] {
        return create_entity();
    };
    auto model = assets_retrieval.make_vaguely_tree_like_model();
    auto mk_tx = [&assets_retrieval]
        { return assets_retrieval.make_ground_texture(); };
    auto mk_leaves = [&assets_retrieval]
        { return assets_retrieval.make_vaguely_palm_leaves(); };
    const auto base_rot = get_rotation(props);

    auto trunk = make_ent();
    TupleBuilder{}.
        add(YRotation{}).
        add(mk_translation(props)).
        add(mk_scale(props)).
        add(assets_retrieval.make_vaguely_tree_like_model()).
        add(mk_tx()).
        add_to_entity(trunk);
    for (auto rot : k_leaf_rotations) {
        auto ent = make_ent();
        TupleBuilder{}.
            add(mk_leaves()).
            add(mk_tx()).
            add(mk_translation(props)).
            add(mk_scale(props)).
            add(YRotation{rot + base_rot}).
            add_to_entity(ent);
    }
}

/* static */ void MapObjectSpawner::spawn_grass
    (const MapItemPropertiesRetrieval & props,
     const EntityCreator & entity_adder,
     AssetsRetrieval & assets_retrieval)
{
    auto ent = entity_adder();
    TupleBuilder{}.
        add(assets_retrieval.make_grass_model()).
        add(assets_retrieval.make_ground_texture()).
        add(YRotation{get_rotation(props)}).
        add(mk_translation(props)).
        add(mk_scale(props)).
        add_to_entity(ent);
}
