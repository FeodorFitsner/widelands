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

#ifndef __S__FONT_HANDLER_H
#define __S__FONT_HANDLER_H

#include "geometry.h"
#include "rgbcolor.h"

#include <SDL_ttf.h>

#include <list>
#include <string>
#include <vector>

class Font_Loader;
class RenderTarget;
class Text_Block;

enum Align {
    Align_Left = 0,
    Align_HCenter = 1,
    Align_Right = 2,
    Align_Horizontal = 3,

    Align_Top = 0,
    Align_VCenter = 4,
    Align_Bottom = 8,
    Align_Vertical = 12,

    Align_TopLeft = 0,
    Align_CenterLeft = Align_VCenter,
    Align_BottomLeft = Align_Bottom,

    Align_TopCenter = Align_HCenter,
    Align_Center = Align_HCenter|Align_VCenter,
    Align_BottomCenter = Align_HCenter|Align_Bottom,

    Align_TopRight = Align_Right,
    Align_CenterRight = Align_Right|Align_VCenter,
    Align_BottomRight = Align_Right|Align_Bottom,
};

enum Widget_Cache {
    Widget_Cache_None = 0,
    Widget_Cache_Use = 1,
    Widget_Cache_New = 2,
    Widget_Cache_Update = 3
};

/** class Font_Handler
 *
 * This class generates font Pictures out of strings and returns them
 */
typedef std::string (*Varibale_Callback)(std::string, void* data);
class Font_Handler {
public:
	Font_Handler();
	~Font_Handler();
	void draw_string
		(RenderTarget &,
		 const std::string & font,
		 int size,
		 const RGBColor fg, const RGBColor bg,
		 Point dstpoint,
		 const std::string & text,
		 const Align align = Align_CenterLeft,
		 const int wrap = -1,
		 const Widget_Cache widget_cache = Widget_Cache_None,
		 uint * const widget_cache_id = 0,
		 const int caret = -1,
		 bool transparent = true);
	void get_size
		(const std::string & fontname, const int size,
		 std::string text,
		 int *w, int *h, int wrap = -1);
	int calc_linewidth(TTF_Font &, const std::string & text);
	uint get_fontheight(const std::string & name, const int size);
	std::string remove_first_space(const std::string &text);
	std::string word_wrap_text
		(TTF_Font &,
		 const std::string & unwrapped_text,
		 const int max_width);
	std::string word_wrap_text
		(const std::string & font, const int size,
		 const std::string & unwrapped_text, const int max_width);
	void do_align(Align align, int *dstx, int *dsty, int w, int h);
	// This deletes all cached pictures, it is called
	// from the graphics code before the graphics are flushed,
	// to make sure that everything is forgotten
	void flush_cache();
	void delete_widget_cache(uint widget_cache_id);
	void draw_richtext
		(RenderTarget &,
		 const RGBColor bg,
		 Point dstpoint,
		 std::string text,
		 int wrap,
		 Widget_Cache widget_cache = Widget_Cache_None,
		 uint * const widget_cache_id = 0,
		 bool transparent = true);
	void get_size_from_cache(const uint widget_cache_id, uint & w, uint & h);

	// Register a callback which is used whenever the tag <variable name="kljdf"> appears
	void register_variable_callback(Varibale_Callback, void* cbdata);
	void unregister_variable_callback();

private:
	struct _Cache_Infos {
		uint surface_id;
		std::string str;
		const TTF_Font * f;
		RGBColor fg;
		RGBColor bg;
		uint      w;
		uint      h;

		inline bool operator== (const _Cache_Infos& who) const {
			return (str == who.str &&
			         f == who.f &&
			         fg == who.fg &&
			         bg == who.bg);
		}
	};

private:
	static const uint CACHE_ARRAY_SIZE = 500;

	Font_Loader* m_font_loader;
	std::list<_Cache_Infos> m_cache;
	Varibale_Callback m_varcallback;
	void* m_cbdata;

private:
	uint create_text_surface
		(TTF_Font &,
		 const RGBColor fg, const RGBColor bg,
		 const std::string & text, const Align, const int wrap,
		 const int caret = -1, bool transparent = true);
	uint convert_sdl_surface
		(SDL_Surface &, const RGBColor bg, bool transparent = false);
	SDL_Surface * draw_string_sdl_surface
		(const std::string & fontname, const int fontsize,
		 const RGBColor fg, const RGBColor bg,
		 const std::string & text,
		 const Align align, const int wrap, const int style = TTF_STYLE_NORMAL,
		 const int line_spacing = 0);
	SDL_Surface* create_sdl_text_surface
		(TTF_Font &, const RGBColor fg, const RGBColor bg,
		 const std::string & text,
		 const Align align, const int wrap, const int line_spacing = 0);
	SDL_Surface * create_static_long_text_surface
		(TTF_Font &, const RGBColor fg, const RGBColor bg,
		 std::string text,
		 const Align align, const int wrap, const int line_spacing = 0,
		 const int caret = -1);
	SDL_Surface* create_single_line_text_surface
		(TTF_Font &, const RGBColor fg, const RGBColor bg,
		 std::string text,
		 const Align align, const int caret = -1);
	SDL_Surface* create_empty_sdl_surface(uint w, uint h);
	SDL_Surface* join_sdl_surfaces
		(const uint w, const uint h,
		 const std::vector<SDL_Surface *> & surfaces,
		 const RGBColor bg,
		 const Align align        = Align_Left,
		 const int spacing        = 0,
		 const bool vertical      = false,
		 const bool keep_surfaces = false);
	SDL_Surface* load_image(std::string file);
	SDL_Surface* render_space(Text_Block &block, RGBColor bg, int style = TTF_STYLE_NORMAL);
	void render_caret
		(TTF_Font &,
		 SDL_Surface & line,
		 const std::string & text_caret_pos);
};

extern Font_Handler* g_fh; // the default font

#endif
