/*
 * Copyright (C) 2002-2004, 2006-2010 by the Widelands Development Team
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

#ifndef RECT_H
#define RECT_H

#include "point.h"

struct Rect : public Point {
	Rect() throw () {}
	Rect(const Point p, const uint32_t W, const uint32_t H) throw ()
		: Point(p), w(W), h(H)
	{}
	Point bottom_right() const {return *this + Point(w, h);}

	/**
	 * Returns true if this rectangle contains the given point.
	 *
	 * The bottom and right borders of the rectangle are considered to be excluded.
	 */
	bool contains(Point pt) const {
		return
			pt.x >= x && pt.x < x + static_cast<int32_t>(w) &&
			pt.y >= y && pt.y < y + static_cast<int32_t>(h);
	}

	uint32_t w, h;
};

#endif
