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

#include "DocumentOwningXmlElement.hpp"

#include <tinyxml2.h>

/* static */ Optional<DocumentOwningXmlElement>
    DocumentOwningXmlElement::load_from_contents(std::string && file_contents)
{
    struct OwnerImpl final : public Owner {
        TiXmlDocument document;
    };
    auto owner = make_shared<OwnerImpl>();
    auto & document = owner->document;
    if (document.Parse(file_contents.c_str()) != tinyxml2::XML_SUCCESS) {
        return {};
    }
    return DocumentOwningXmlElement{owner, *document.RootElement()};
}

DocumentOwningXmlElement DocumentOwningXmlElement::make_with_same_owner
    (const TiXmlElement & same_document_element) const
{ return DocumentOwningXmlElement{m_owner, same_document_element}; }

const TiXmlElement & DocumentOwningXmlElement::element() const
    { return *m_element; }
