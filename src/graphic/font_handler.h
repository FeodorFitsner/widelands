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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef FONT_HANDLER_H
#define FONT_HANDLER_H

#include "ui_basic/align.h"
#include "point.h"
#include "rgbcolor.h"
#include "graphic.h"
#include "picture_id.h"
#include "ui_basic/widget_cache.h"

#include <SDL_ttf.h>

#include <boost/scoped_ptr.hpp>

#include <list>
#include <cstring>
#include <vector>

struct RenderTarget;

namespace UI {
struct Text_Block;
struct TextStyle;

/** class Font_Handler
 *
 * This class generates font Pictures out of strings and returns them
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
	uint32_t draw_text_raw(RenderTarget &, const TextStyle &, Point dstpoint, const std::string & text);
	void draw_multiline
		(RenderTarget &,
		 const TextStyle &,
		 Point dstpoint,
		 const std::string & text,
		 Align align = Align_CenterLeft,
		 uint32_t wrap = std::numeric_limits<uint32_t>::max(),
		 uint32_t caret = std::numeric_limits<uint32_t>::max());

	void get_size
		(const TextStyle &,
		 const std::string & text,
		 uint32_t & w, uint32_t & h,
		 uint32_t wrap = std::numeric_limits<uint32_t>::max());
	void get_size
		(std::string const & fontname, int32_t size,
		 const std::string & text,
		 uint32_t & w, uint32_t & h,
		 uint32_t wrap = std::numeric_limits<uint32_t>::max());
	int32_t calc_linewidth(TTF_Font &, const std::string & text);
	uint32_t get_fontheight(std::string const & name, int32_t size);
	std::string word_wrap_text
		(TTF_Font &,
		 const std::string & unwrapped_text,
		 int32_t max_width);
	std::string word_wrap_text
		(std::string const & font, int32_t size,
		 std::string const & unwrapped_text, int32_t max_width);
	void do_align
		(Align, int32_t & dstx, int32_t & dsty, int32_t w, int32_t h);
	// This deletes all cached pictures, it is called
	// from the graphics code before the graphics are flushed,
	// to make sure that everything is forgotten
	void flush_cache();
	void draw_richtext
		(RenderTarget &,
		 RGBColor bg,
		 Point dstpoint,
		 std::string text,
		 int32_t wrap,
		 Widget_Cache widget_cache = Widget_Cache_None,
		 PictureID * widget_cache_id = 0,
		 bool transparent = true);
	void get_size_from_cache
		(PictureID widget_cache_id, uint32_t & w, uint32_t & h);

private:
	struct _Cache_Infos {
		PictureID picture_id;
		std::string str;
		const TTF_Font * f;
		RGBColor fg;
		RGBColor bg;
		int32_t caret;
		uint32_t      w;
		uint32_t      h;

		bool operator== (_Cache_Infos const & who) const {
			return
				str == who.str &&
				f   == who.f   &&
				fg  == who.fg  &&
				bg  == who.bg &&
				caret == who.caret;
		}
	};

private:
	static const uint32_t CACHE_ARRAY_SIZE = 500;

	std::list<_Cache_Infos> m_cache;

	struct Data;
	boost::scoped_ptr<Data> d;

private:
	PictureID convert_sdl_surface
		(SDL_Surface &, const RGBColor bg, bool transparent = false);
	SDL_Surface * draw_string_sdl_surface
		(std::string const & fontname, int32_t fontsize,
		 RGBColor fg, RGBColor bg,
		 const std::string & text,
		 Align,
		 uint32_t            wrap        = std::numeric_limits<uint32_t>::max(),
		 int32_t style = TTF_STYLE_NORMAL,
		 uint32_t            linespacing = 0);
	SDL_Surface * create_sdl_text_surface
		(TTF_Font &, RGBColor fg, RGBColor bg,
		 std::string const & text,
		 Align,
		 uint32_t            wrap        = std::numeric_limits<uint32_t>::max(),
		 uint32_t            linespacing = 0);
	SDL_Surface * create_static_long_text_surface
		(TTF_Font &, RGBColor fg, RGBColor bg,
		 std::string const & text,
		 Align,
		 uint32_t            wrap        = std::numeric_limits<uint32_t>::max(),
		 uint32_t            linespacing = 0);
	SDL_Surface * create_single_line_text_surface
		(TTF_Font &, RGBColor fg, RGBColor bg,
		 std::string text, Align);
	SDL_Surface * create_empty_sdl_surface(uint32_t w, uint32_t h);
	SDL_Surface * join_sdl_surfaces
		(uint32_t w, uint32_t h,
		 const std::vector<SDL_Surface *> & surfaces,
		 RGBColor bg,
		 Align align        = Align_Left,
		 int32_t spacing        = 0,
		 bool vertical      = false,
		 bool keep_surfaces = false);
	SDL_Surface * load_image(std::string file);
	SDL_Surface * render_space
		(Text_Block &, RGBColor bg, int32_t style = TTF_STYLE_NORMAL);

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
