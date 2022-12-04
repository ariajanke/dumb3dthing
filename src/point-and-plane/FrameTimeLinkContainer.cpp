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

#include "FrameTimeLinkContainer.hpp"

#include <iostream>

namespace {

using namespace cul::exceptions_abbr;
using Iterator = FrameTimeLinkContainer::Iterator;

constexpr const bool k_report_triangle_drops = true;

} // end of <anonymous> namespace

void FrameTimeLinkContainer::defer_addition_of
    (const SharedPtr<const TriangleLink> & ptr)
{
    m_to_add_links.push_back(ptr);
    m_add_dirty = true;
}

void FrameTimeLinkContainer::defer_removal_of
    (const SharedPtr<const TriangleLink> & ptr)
{
    m_to_remove_links.push_back(ptr);
}

void FrameTimeLinkContainer::update() {
    if (!m_to_remove_links.empty()) {
        // TODO: this bit doesn't work
        std::sort(m_to_remove_links.begin(), m_to_remove_links.end());
        std::sort(m_to_add_links.begin(), m_to_add_links.end());
        auto rem_itr = m_to_remove_links.begin();
        auto rem_end = m_to_remove_links.end();
        auto add_itr = m_to_add_links.begin();
        auto add_end = m_to_add_links.end();
        for (; rem_itr != rem_end && add_itr != add_end; ++add_itr) {
            if (*rem_itr == *add_itr) {
                *add_itr = nullptr;
                ++rem_itr;
            }
        }
        m_add_dirty = true;
        std::size_t old_size = m_to_add_links.size();
        m_to_add_links.erase
            (std::remove(m_to_add_links.begin(), m_to_add_links.end(), nullptr),
             m_to_add_links.end());
        m_to_remove_links.clear();
        if constexpr (k_report_triangle_drops) {
            std::cout << (old_size - m_to_add_links.size())
                      << " triangles dropped" << std::endl;
        }
    }

    if (m_add_dirty) {
        m_spm.populate(m_to_add_links);
        m_add_dirty = false;
    }
}

View<Iterator> FrameTimeLinkContainer::view_for
    (const Vector & a, const Vector & b) const
{
    if (is_dirty()) {
        throw RtError{"FrameTimeLinkContainer::view_for: update must be called "
                      "first"};
    }
    return m_spm.view_for(a, b);
}

void FrameTimeLinkContainer::clear() {
    m_to_add_links.clear();
    m_to_remove_links.clear();
    // v can this accept empty containers?
    m_spm.populate(m_to_add_links);
}
