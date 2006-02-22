/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
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

#ifndef __S__MAPVIEWPIXELFUNCTIONS_H
#define __S__MAPVIEWPIXELFUNCTIONS_H

#include "geometry.h"

struct Map;

namespace MapviewPixelFunctions {

float calc_brightness
	(const int l, const int r, const int tl, const int tr, const int bl, const int br);

unsigned int calc_pix_distance(const Map &, Point a, Point b);

unsigned int get_map_end_screen_x(const Map &);
unsigned int get_map_end_screen_y(const Map &);

/**
 * Calculate the coordinates of the triangle the given point in pixels is in.
 *
 * Also chose 1 node that the point is close to. This may not always be the
 * closest node, because it is calculated in a simple way. It only choses among
 * the 2 neighbouring nodes whose screen coordinates define the rectangle that
 * the point is in. But this should be fully correct for all but the most
 * bizarre triangle shapes, and acceptable even for them.
 */
Node_and_Triangle calc_node_and_triangle(const Map &, unsigned int x, unsigned int y);

void normalize_pix(const Map &, Point & p);

/**
 * Calculate the on-screen position of the node without taking height into
 * account.
 */
void get_basepix(const Coords fc, int & px, int & py);

/**
 * Calculate the on-screen position of the node.
 */
void get_pix(const FCoords fc, int & px, int & py);

void get_pix(const Map &, const Coords c, int & px, int & py);

void get_save_pix(const Map &, const Coords c, int & px, int & py);
}

//  Implementation follows:
//  The rest of the content of this file is only here to be inlined. It should
//  have been in the cc file otherwise. Now objectcode modularity is not
//  achieved, but only sourcecode modularity is required.

#include "field.h"
#include "map.h"
#include "mapviewpixelconstants.h"

inline unsigned int MapviewPixelFunctions::get_map_end_screen_x(const Map & map)
{return map.get_width() * TRIANGLE_WIDTH;}
inline unsigned int MapviewPixelFunctions::get_map_end_screen_y(const Map & map)
{return map.get_height() * TRIANGLE_HEIGHT;}

/*
===============
Calculate the on-screen position of the node without taking height
into account.
===============
*/
inline void MapviewPixelFunctions::get_basepix
(const Coords c, int & px, int & py)
{
	py = c.y * TRIANGLE_HEIGHT;
	px = c.x * TRIANGLE_WIDTH + (c.y & 1) * (TRIANGLE_WIDTH / 2);
}


inline void MapviewPixelFunctions::get_pix(const FCoords fc, int & px, int & py)
{
	get_basepix(fc, px, py);
	py -= fc.field->get_height() * HEIGHT_FACTOR;
}

inline
void MapviewPixelFunctions::get_pix
(const Map & map, const Coords c, int & px, int & py)
{get_pix(map.get_fcoords(c), px, py);}

// fx and fy might be out of range, must be normalized for the field
// theres no need for such a function for FCoords, since x,y out of range
// but field valid doesn't make sense
inline void MapviewPixelFunctions::get_save_pix
(const Map & map, const Coords c, int & px, int & py)
{
	Coords c1 = c;
	map.normalize_coords(&c1);
	FCoords fc = map.get_fcoords(c1);
	fc.x = c.x;
	fc.y = c.y;
	get_pix(fc, px, py);
}

#endif // __S__MAPVIEWPIXELFUNCTIONS_H
