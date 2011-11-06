/*
 * Copyright (C) 2003, 2006-2011 by the Widelands Development Team
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

#include "box.h"

#include "graphic/graphic.h"
#include "wexception.h"

#include "scrollbar.h"

#include <algorithm>

namespace UI {
/**
 * Initialize an empty box
*/
Box::Box
	(Panel * const parent,
	 int32_t const x, int32_t const y,
	 uint32_t const orientation,
	 int32_t const max_x, int32_t const max_y)
	:
	Panel        (parent, x, y, 0, 0),

	m_max_x      (max_x ? max_x : g_gr->get_xres()),
	m_max_y      (max_y ? max_y : g_gr->get_yres()),

	m_scrolling(false),
	m_scrollbar(0),
	m_orientation(orientation),
	m_mindesiredbreadth(0)
{}

/**
 * Enable or disable the creation of a scrollbar if the maximum
 * depth is exceeded.
 *
 * Scrollbars are only created for the direction in which boxes
 * are added, e.g. if the box has a \ref Vertical orientation,
 * only a vertical scrollbar may be added.
 */
void Box::set_scrolling(bool scroll)
{
	if (scroll == m_scrolling)
		return;

	m_scrolling = scroll;
	update_desired_size();
}

/**
 * Set the minimum desired breadth.
 *
 * The breadth is the dimension of the box that is orthogonal to
 * its orientation.
 */
void Box::set_min_desired_breadth(uint32_t min)
{
	if (min == m_mindesiredbreadth)
		return;

	m_mindesiredbreadth = min;
	update_desired_size();
}

/**
 * Compute the desired size based on our children.
 */
void Box::update_desired_size()
{
	uint32_t totaldepth = 0;
	uint32_t maxbreadth = m_mindesiredbreadth;

	for (uint32_t idx = 0; idx < m_items.size(); ++idx) {
		uint32_t depth, breadth;
		get_item_desired_size(idx, depth, breadth);

		totaldepth += depth;
		if (breadth > maxbreadth)
			maxbreadth = breadth;
	}

	if (m_orientation == Horizontal) {
		if (totaldepth > m_max_x && m_scrolling) {
			maxbreadth += Scrollbar::Size;
		}
		set_desired_size
			(std::min(totaldepth, m_max_x), std::min(maxbreadth, m_max_y));
	} else {
		if (totaldepth > m_max_y && m_scrolling) {
			maxbreadth += Scrollbar::Size;
		}
		set_desired_size
			(std::min(maxbreadth, m_max_x), std::min(totaldepth, m_max_y));
	}

	//  This is not redundant, because even if all this does not change our
	//  desired size, we were typically called because of a child window that
	//  changed, and we need to relayout that.
	layout();
}

/**
 * Adjust all the children and the box's size.
 */
void Box::layout()
{
	uint32_t totalbreadth = m_orientation == Horizontal ? get_w() : get_h();

	// First pass: compute the depth and adjust whether we have a scrollbar
	uint32_t totaldepth = 0;

	for (uint32_t idx = 0; idx < m_items.size(); ++idx) {
		uint32_t depth, tmp;
		get_item_desired_size(idx, depth, tmp);

		totaldepth += depth;
	}

	bool needscrollbar = false;
	if (m_orientation == Horizontal) {
		if (totaldepth > m_max_x && m_scrolling) {
			totalbreadth -= Scrollbar::Size;
			needscrollbar = true;
		}
	} else {
		if (totaldepth > m_max_y && m_scrolling) {
			totalbreadth -= Scrollbar::Size;
			needscrollbar = true;
		}
	}

	if (needscrollbar) {
		int32_t sb_x, sb_y, sb_w, sb_h;
		int32_t pagesize;
		if (m_orientation == Horizontal) {
			sb_x = 0;
			sb_y = get_h() - Scrollbar::Size;
			sb_w = get_w();
			sb_h = Scrollbar::Size;
			pagesize = get_w() - Scrollbar::Size;
		} else {
			sb_x = get_w() - Scrollbar::Size;
			sb_y = 0;
			sb_w = Scrollbar::Size;
			sb_h = get_h();
			pagesize = get_h() - Scrollbar::Size;
		}
		if (!m_scrollbar) {
			m_scrollbar = new Scrollbar
					(this, sb_x, sb_y, sb_w,
					 sb_h, m_orientation == Horizontal);
			m_scrollbar->moved.set(this, &Box::scrollbar_moved);
		} else {
			m_scrollbar->set_pos(Point(sb_x, sb_y));
			m_scrollbar->set_size(sb_w, sb_h);
		}

		m_scrollbar->set_steps(totaldepth - pagesize);
		m_scrollbar->set_singlestepsize(Scrollbar::Size);
		m_scrollbar->set_pagesize(pagesize);
	} else {
		delete m_scrollbar;
		m_scrollbar = 0;
	}

	// Second pass: Update positions and sizes of all items
	update_positions();
}

void Box::update_positions()
{
	int32_t scrollpos = m_scrollbar ? m_scrollbar->get_scrollpos() : 0;

	uint32_t totaldepth = 0;
	uint32_t totalbreadth = m_orientation == Horizontal ? get_h() : get_w();
	if (m_scrollbar)
		totalbreadth -= Scrollbar::Size;

	for (uint32_t idx = 0; idx < m_items.size(); ++idx) {
		uint32_t depth, breadth;
		get_item_desired_size(idx, depth, breadth);

		if (m_items[idx].type == Item::ItemPanel) {
			set_item_size
				(idx, depth, m_items[idx].u.panel.fullsize ?
				 totalbreadth : breadth);
			set_item_pos(idx, totaldepth - scrollpos);
		}

		totaldepth += depth;
	}
}

/**
 * Callback for scrollbar movement.
 */
void Box::scrollbar_moved(int32_t)
{
	update_positions();
}


/**
 * Add a new panel to be controlled by this box
 *
 * @param fullsize when true, @p panel will be extended to cover the entire width (or height)
 * of the box for horizontal (vertical) panels. If false, then @p panel may end up smaller;
 * in that case, it will be aligned according to @p align
 */
void Box::add(Panel * const panel, uint32_t const align, bool fullsize)
{
	Item it;

	it.type = Item::ItemPanel;
	it.u.panel.panel = panel;
	it.u.panel.align = align;
	it.u.panel.fullsize = fullsize;

	m_items.push_back(it);

	update_desired_size();
}


/**
 * Add spacing of empty pixels.
*/
void Box::add_space(uint32_t space)
{
	Item it;

	it.type = Item::ItemSpace;
	it.u.space = space;

	m_items.push_back(it);

	update_desired_size();
}


/**
 * Retrieve the given item's desired size. depth is the size of the
 * item along the orientation axis, breadth is the size perpendicular
 * to the orientation axis.
*/
void Box::get_item_desired_size
	(uint32_t const idx, uint32_t & depth, uint32_t & breadth)
{
	assert(idx < m_items.size());

	Item const & it = m_items[idx];

	switch (it.type) {
	case Item::ItemPanel:
		if (m_orientation == Horizontal) {
			it.u.panel.panel->get_desired_size(depth, breadth);
		} else {
			it.u.panel.panel->get_desired_size(breadth, depth);
		}
		break;

	case Item::ItemSpace:
		depth   = it.u.space;
		breadth = 0;
		break;

	default:
		throw wexception("Box::get_item_size: bad type %u", it.type);
	}
}

/**
 * Set the given items actual size.
 */
void Box::set_item_size(uint32_t idx, uint32_t depth, uint32_t breadth)
{
	assert(idx < m_items.size());

	Item const & it = m_items[idx];

	if (it.type == Item::ItemPanel) {
		if (m_orientation == Horizontal)
			it.u.panel.panel->set_size(depth, breadth);
		else
			it.u.panel.panel->set_size(breadth, depth);
	}
}

/**
 * Position the given item according to its parameters.
 * pos is the position relative to the parent in the direction of the
 * orientation axis.
*/
void Box::set_item_pos(uint32_t idx, int32_t pos)
{
	assert(idx < m_items.size());

	Item const & it = m_items[idx];

	switch (it.type) {
	case Item::ItemPanel: {
		int32_t breadth, maxbreadth;

		if (m_orientation == Horizontal) {
			breadth = it.u.panel.panel->get_h();
			maxbreadth = get_h();
		} else {
			breadth = it.u.panel.panel->get_w();
			maxbreadth = get_w();
		}
		switch (it.u.panel.align) {
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
			it  .u.panel.panel->set_pos(Point(pos, breadth));
		else it.u.panel.panel->set_pos(Point(breadth, pos));
		break;
	}

	case Item::ItemSpace:; //  no need to do anything
	};
}

}
