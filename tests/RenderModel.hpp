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

#pragma once

#include "../src/RenderModel.hpp"

class TestRenderModel final : public RenderModel {
public:
    void render() const final {}

    bool is_loaded() const noexcept final { return m_loaded; }

private:
    void load_(const Vertex   *, const Vertex   *,
               const unsigned *, const unsigned *) final
    {
        m_loaded = true;
    }

    bool m_loaded = false;
};
