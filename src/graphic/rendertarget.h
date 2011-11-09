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

#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "compositemode.h"
#include "picture_id.h"
#include "surfaceptr.h"
#include "rect.h"
#include "rgbcolor.h"

#include <vector>

namespace Widelands {
struct Player;
};

/**
 * This abstract class represents anything that can be rendered to.
 *
 * It supports windows, which are composed of a clip rectangle and a drawing
 * offset:
 * The drawing offset will be added to all coordinates that are passed to
 * drawing routines. Therefore, the offset is usually negative. Then the
 * coordinates are interpreted as relative to the clip rectangle and the
 * primitives are clipped accordingly.
 * \ref enter_window() can be used to enter a sub-window of the current window.
 * When you're finished drawing in the window, restore the old window by calling
 * \ref set_window() with the values stored in previous and prevofs.
 * \note If the sub-window would be empty/invisible, \ref enter_window() returns
 * false and doesn't change the window state at all.
*/
struct RenderTarget {
	RenderTarget(SurfacePtr);
	RenderTarget(OffscreenSurfacePtr);
	void set_window(Rect const & rc, Point const & ofs);
	bool enter_window(Rect const & rc, Rect * previous, Point * prevofs);

	int32_t get_w() const;
	int32_t get_h() const;

	void draw_line
		(int32_t x1,
		 int32_t y1,
		 int32_t x2,
		 int32_t y2,
		 RGBColor color);
	void draw_rect(Rect, RGBColor);
	void fill_rect(Rect, RGBAColor);
	void brighten_rect(Rect, int32_t factor);
	void clear();

	void blit(Point dst, PictureID picture, Composite cm = CM_Normal);
	void blitrect(Point dst, PictureID picture, Rect src, Composite cm = CM_Normal);
	void tile(Rect, PictureID picture, Point ofs, Composite cm = CM_Normal);

	void drawanim
		(Point                     dst,
		 uint32_t                  animation,
		 uint32_t                  time,
		 Widelands::Player const * = 0);

	void drawstatic
			(Point                     dst,
			 uint32_t                  animation,
			 Widelands::Player const * = 0);

	void drawanimrect
		(Point                     dst,
		 uint32_t                  animation,
		 uint32_t                  time,
		 Widelands::Player const *,
		 Rect                      srcrc);

	void reset();

	SurfacePtr get_surface() {return m_surface;}

protected:
	bool clip(Rect & r) const throw ();

	void doblit(Point dst, PictureID src, Rect srcrc, Composite cm = CM_Normal);

	///The target surface
	SurfacePtr m_surface;
	///The current clip rectangle
	Rect m_rect;
	///Drawing offset
	Point m_offset;

};

#endif
