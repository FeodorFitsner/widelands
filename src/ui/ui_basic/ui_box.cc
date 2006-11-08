/*
 * Copyright (C) 2003, 2006 by the Widelands Development Team
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

#include <cassert>
#include "types.h"
#include "ui_box.h"
#include "wexception.h"

namespace UI {
/**
Initialize an empty box
*/
Box::Box(Panel* parent, int x, int y, uint orientation)
	: Panel(parent, x, y, 0, 0)
{
	m_orientation = orientation;
}


/**
Adjust all the children and the box's size.
*/
void Box::resize()
{
	uint idx;
	int totaldepth;
	int maxbreadth;

	// Adjust out size
	totaldepth = 0;
	maxbreadth = 0;

	for(idx = 0; idx < m_items.size(); idx++)
		{
		int depth, breadth;

		get_item_size(idx, &depth, &breadth);

		totaldepth += depth;
		if (breadth > maxbreadth)
			maxbreadth = breadth;
		}

	if (m_orientation == Horizontal)
		set_size(totaldepth, maxbreadth);
	else
		set_size(maxbreadth, totaldepth);

	// Position the children
	totaldepth = 0;

	for(idx = 0; idx < m_items.size(); idx++)
		{
		int depth;

		get_item_size(idx, &depth, 0);
		set_item_pos(idx, totaldepth);

		totaldepth += depth;
		}
}


/**
Add a new panel to be controlled by this box
*/
void Box::add(Panel* panel, uint align)
{
	Item it;

	it.type = Item::ItemPanel;
	it.u.panel.panel = panel;
	it.u.panel.align = align;

	m_items.push_back(it);

	resize();
}


/**
Add spacing of empty pixels.
*/
void Box::add_space(uint space)
{
	Item it;

	it.type = Item::ItemSpace;
	it.u.space = space;

	m_items.push_back(it);

	resize();
}


/**
Retrieve the given item's size. depth is the size of the item along the
orientation axis, breadth is the size perpendicular to the orientation axis.
*/
void Box::get_item_size(uint idx, int* pdepth, int* pbreadth)
{
	assert(idx < m_items.size());

	const Item& it = m_items[idx];
	int depth, breadth;

	switch(it.type)
		{
		case Item::ItemPanel:
			if (m_orientation == Horizontal)
				{
				depth = it.u.panel.panel->get_w();
				breadth = it.u.panel.panel->get_h();
				}
			else
				{
				depth = it.u.panel.panel->get_h();
				breadth = it.u.panel.panel->get_w();
				}
			break;

		case Item::ItemSpace:
			depth = it.u.space;
			breadth = 0;
			break;

		default:
			throw wexception("Box::get_item_size: bad type %u", it.type);
		}

	if (pdepth)
		*pdepth = depth;
	if (pbreadth)
		*pbreadth = breadth;
}


/**
Position the given item according to its parameters.
pos is the position relative to the parent in the direction of the orientation
axis.
*/
void Box::set_item_pos(uint idx, int pos)
{
	assert(idx < m_items.size());

	const Item& it = m_items[idx];

	switch(it.type)
		{
		case Item::ItemPanel:
			{
			int breadth, maxbreadth;

			if (m_orientation == Horizontal)
				{
				breadth = it.u.panel.panel->get_h();
				maxbreadth = get_h();
				}
			else
				{
				breadth = it.u.panel.panel->get_w();
				maxbreadth = get_w();
				}

			switch(it.u.panel.align)
				{
				case AlignLeft:
				default:
					breadth = 0;
					break;

				case AlignCenter:
					breadth = (maxbreadth - breadth) / 2;
					break;

				case AlignRight:
					breadth = maxbreadth - breadth;
					break;
				}

			if (m_orientation == Horizontal)
				it.u.panel.panel->set_pos(pos, breadth);
			else
				it.u.panel.panel->set_pos(breadth, pos);
			}
			break;

		case Item::ItemSpace:
			break; // no need to do anything
		}
}
};
