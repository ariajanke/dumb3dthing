/******************************************************************************

    GPLv3 License
    Copyright (c) 2022 Aria Janke

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

#include "RenderModel.hpp"

void RenderModel::load(const RenderModelData & model_data)
    { load(model_data.vertices, model_data.elements); }

void RenderModel::load
    (const std::vector<Vertex> & vertices, const std::vector<unsigned> & elements)
{
    load(&vertices.front(), &vertices.front() + vertices.size(),
         &elements.front(), &elements.front() + elements.size());
}

void RenderModel::load
    (const Vertex   * vertex_beg  , const Vertex   * vertex_end,
     const unsigned * elements_beg, const unsigned * elements_end)
{ load_(vertex_beg, vertex_end, elements_beg, elements_end); }
