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

#include "ParseHelpers.hpp"

#include <tinyxml2.h>

TiXmlIter::TiXmlIter(): el(nullptr), name(nullptr) {}

TiXmlIter::TiXmlIter(const TiXmlElement * el_, const char * name_):
    el(el_), name(name_) {}

TiXmlIter & TiXmlIter::operator ++ () {
    el = el->NextSiblingElement(name);
    return *this;
}

bool TiXmlIter::operator != (const TiXmlIter & rhs) const
    { return el != rhs.el; }

const TiXmlElement & TiXmlIter::operator * () const
    { return *el; }

XmlRange::XmlRange(const TiXmlElement * el_, const char * name_):
    m_beg(el_ ? el_->FirstChildElement(name_) : nullptr, name_) {}

XmlRange::XmlRange(const TiXmlElement & el_, const char * name_):
    m_beg(el_.FirstChildElement(name_), name_) {}

TiXmlIter XmlRange::begin() const { return m_beg;       }

TiXmlIter XmlRange::end()   const { return TiXmlIter{}; }
