/*
 * Copyright (C) 2002, 2006-2007 by the Widelands Development Team
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

#ifndef __S__MULTILINE_TEXTAREA_H
#define __S__MULTILINE_TEXTAREA_H

#include <string>
#include "font_handler.h"
#include "ui_panel.h"
#include "ui_scrollbar.h"

namespace UI {
struct Scrollbar;

/**
 * This defines an area, where a text can easily be printed.
 * The textarea transparently handles explicit line-breaks and word wrapping.
 *
 * Do not use it blindly for big texts: the font handler needs to re-break the
 * entire text whenever the textarea is drawn, this is a trade-off which greatly
 * simplifies this class.
 */
struct Multiline_Textarea : public Panel {
      enum ScrollMode {
         ScrollNormal = 0,    ///< (default) only explicit or forced scrolling
         ScrollLog = 1,       ///< follow the bottom of the text
		};

public:
	Multiline_Textarea
		(Panel * const parent,
		 const int x, const int y, const uint w, const uint h,
		 const std::string & text         = std::string(),
		 const Align                      = Align_Left,
		 const bool always_show_scrollbar = false);
      ~Multiline_Textarea();

      std::string get_text() const { return m_text; }
      ScrollMode get_scrollmode() const { return m_scrollmode; }

	void set_text(const std::string &text);
      void set_align(Align align);
      void set_scrollpos(int pixels);
      void set_scrollmode(ScrollMode mode);

	uint scrollbar_w() const throw () {return 24;}
	uint get_eff_w() const throw () {return get_w() - scrollbar_w();}

      inline void set_font(std::string name, int size, RGBColor fg) { m_fontname=name; m_fontsize=size; m_fcolor=fg; set_text(m_text.c_str()); }

      // Drawing and event handlers
      void draw(RenderTarget* dst);

	bool handle_mousepress  (const Uint8 btn, int x, int y);

      inline const char* get_font_name() { return m_fontname.c_str(); }
      inline const int get_font_size() { return m_fontsize; }
      inline RGBColor& get_font_clr() { return m_fcolor; }

   private:
	std::string  m_text;
	Scrollbar    m_scrollbar;
		ScrollMode     m_scrollmode;

   protected:
	Align        m_align;
	uint         m_cache_id; ///picid of the whole textarea surface

	///set to Widget_Cache_Update if the whole textarea has to be rebuild
	Widget_Cache m_cache_mode;

		std::string    m_fontname;
		int            m_fontsize;
		RGBColor       m_fcolor;
		uint m_textheight;  ///< total height of wrapped text, in pixels
		uint m_textpos;     ///< current scrolling position in pixels (0 is top)

      inline int get_m_textpos(void) { return m_textpos; }
      void draw_scrollbar();
      int get_halign();
};
};

#endif // __S__MULTILINE_TEXTAREA_H
