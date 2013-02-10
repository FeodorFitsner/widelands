/*
 * Copyright (C) 2002-2004, 2006-2011 by the Widelands Development Team
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

#ifndef FONT_HANDLER_H
#define FONT_HANDLER_H

#include <boost/scoped_ptr.hpp>

#include "point.h"
#include "align.h"

class RenderTarget;

namespace UI {

struct TextStyle;

/**
 * Main class for string rendering. Manages the cache of pre-rendered strings.
 */
struct Font_Handler {
	Font_Handler();
	~Font_Handler();

	void draw_text
		(RenderTarget &,
		 const TextStyle &,
		 Point dstpoint,
		 const std::string & text,
		 Align align = Align_CenterLeft,
		 uint32_t caret = std::numeric_limits<uint32_t>::max());
	void draw_text_shadow
		(RenderTarget &,
		 const TextStyle &,
		 Point dstpoint,
		 const std::string & text,
		 Align align = Align_CenterLeft,
		 uint32_t caret = std::numeric_limits<uint32_t>::max());
	uint32_t draw_text_raw(RenderTarget &, const TextStyle &, Point dstpoint, const std::string & text);

	void get_size
		(const TextStyle &,
		 const std::string & text,
		 uint32_t & w, uint32_t & h,
		 uint32_t wrap = std::numeric_limits<uint32_t>::max());
	void get_size
		(const std::string & fontname, int32_t size,
		 const std::string & text,
		 uint32_t & w, uint32_t & h,
		 uint32_t wrap = std::numeric_limits<uint32_t>::max());
	uint32_t get_fontheight(const std::string & name, int32_t size);
	void do_align
		(Align, int32_t & dstx, int32_t & dsty, int32_t w, int32_t h);

private:
	struct Data;
	boost::scoped_ptr<Data> d;

private:
	void draw_caret
		(RenderTarget &,
		 const TextStyle &,
		 Point dstpoint,
		 const std::string & text,
		 uint32_t caret);
};

extern Font_Handler * g_fh;

}

#endif
