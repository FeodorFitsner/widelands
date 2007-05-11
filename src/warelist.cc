/*
 * Copyright (C) 2002-2004, 2006-2007 by the Widelands Development Team
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "error.h"
#include "warelist.h"
#include "types.h"


/**
 * Delete the list. Print a warning message if the storage is not empty.
 *
 * This is because most of the time, a WareList should be zeroed by cleanup
 * operations before the destructor is called. If you are sure of what you're
 * doing, call clear().
 */
WareList::~WareList()
{
	for(uint id = 0; id < m_wares.size(); id++) {
		if (m_wares[id])
			log("WareList: %i items of %i left.\n", m_wares[id], id);
	}
}


/**
 * Add the given number of items (default = 1) to the storage.
 */
void WareList::add(int id, int count)
{
	if (!count)
		return;

	assert(id >= 0);

	if (id >= (int)m_wares.size())
		m_wares.resize(id+1, 0);
	m_wares[id] += count;
	assert(m_wares[id] >= count);
}


void WareList::add(const WareList &wl)
{
	if (wl.m_wares.size() > m_wares.size())
		m_wares.reserve(wl.m_wares.size());

	for(uint id = 0; id < wl.m_wares.size(); id++)
		if (wl.m_wares[id])
			add(id, wl.m_wares[id]);
}


/**
 * Remove the given number of items (default = 1) from the storage.
 */
void WareList::remove(int id, int count)
{
	if (!count)
		return;

	assert(id >= 0 && id < (int)m_wares.size());
   assert(m_wares[id] >= count);
	m_wares[id] -= count;
}


void WareList::remove(const WareList &wl)
{
	for(uint id = 0; id < wl.m_wares.size(); id++)
		if (wl.m_wares[id])
			remove(id, wl.m_wares[id]);
}

/**
 * Return the number of wares of a given type stored in this storage.
 */
int WareList::stock(int id) const
{
	assert(id >= 0);

	if (id >= (int)m_wares.size())
		return 0;
	return m_wares[id];
}


/**
 * Two WareLists are only equal when they contain the exact same stock of
 * all wares types.
*/
bool WareList::operator==(const WareList &wl) const
{
	uint i = 0;

	while(i < wl.m_wares.size()) {
		int count = wl.m_wares[i];
		if (i < this->m_wares.size()) {
			if (count != this->m_wares[i])
				return false;
		} else {
			if (count) // wl2 has 0 stock per definition
				return false;
		}
		i++;
	}

	while(i < this->m_wares.size()) {
		if (this->m_wares[i]) // wl1 has 0 stock per definition
			return false;
		i++;
	}

	return true;
}

