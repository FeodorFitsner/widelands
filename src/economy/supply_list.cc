/*
 * Copyright (C) 2004, 2006-2009 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "economy/supply_list.h"

#include "base/wexception.h"
#include "economy/request.h"
#include "economy/supply.h"

namespace Widelands {

/**
 * Add a supply to the list.
*/
void SupplyList::add_supply(Supply & supp)
{
	m_supplies.push_back(&supp);
}

/**
 * Remove a supply from the list.
*/
void SupplyList::remove_supply(Supply & supp)
{
	for (Supplies::iterator item_iter = m_supplies.begin();
		  item_iter != m_supplies.end();
		  ++item_iter) {

		if (*item_iter == &supp) {
			*item_iter = *(m_supplies.end() - 1);
			return m_supplies.pop_back();
		}
	}
	throw wexception("SupplyList::remove: not in list");
}

/**
 * Return whether there is at least one available
 * supply that can match the given request.
 */
bool SupplyList::have_supplies(Game & game, const Request & req)
{
	for (size_t i = 0; i < m_supplies.size(); ++i)
		if (m_supplies[i]->nr_supplies(game, req))
			return true;

	return false;
}

}
