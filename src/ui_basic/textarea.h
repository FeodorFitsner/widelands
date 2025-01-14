/*
 * Copyright (C) 2002, 2006, 2008 by the Widelands Development Team
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

#ifndef WL_UI_BASIC_TEXTAREA_H
#define WL_UI_BASIC_TEXTAREA_H

#include "graphic/align.h"
#include "graphic/text_layout.h"
#include "ui_basic/panel.h"

namespace UI {

/**
 * This defines a non responsive (to clicks) text area, where a text
 * can easily be printed.
 *
 * Textareas can operate in auto-move mode or in layouted mode.
 *
 * In auto-move mode, which is selected by constructors that take x/y coordinates
 * as parameters, the given (x,y) is used as the anchor for the text.
 * The panel automatically changes its size and position so that the
 * given (x,y) always stay the anchor point. This is incompatible with
 * using the Textarea in a layouted situation, e.g. inside \ref Box.
 *
 * In layouted mode, which is selected by the constructor that does not
 * take coordinates, the textarea simply sets its desired size
 * appropriately for the contained text.
 *
 * Finally, there is static mode, which does not change desired or actual
 * size in any way based on the text.
 *
 * A multiline Textarea differs from a \ref MultilineTextarea in that
 * the latter provides scrollbars.
 */
struct Textarea : public Panel {
	Textarea
		(Panel * parent,
		 int32_t x, int32_t y,
		 const std::string & text = std::string(),
		 Align align = UI::Align::kLeft);
	Textarea
		(Panel * parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 Align align = UI::Align::kLeft);
	Textarea
		(Panel *  const parent,
		 int32_t x, int32_t y, uint32_t w, uint32_t h,
		 const std::string & text,
		 Align align = UI::Align::kLeft);
	Textarea
		(Panel * parent,
		 const std::string & text = std::string(),
		 Align align = UI::Align::kLeft);

	/**
	 * If fixed_width > 0, the Textarea will not change its width.
	 * Use this if you need a Textarea that keeps changing its contents, but you don't want the
	 * surrounding elements to shift, e.g. in a Box.
	 */
	void set_fixed_width(uint32_t w);

	void set_text(const std::string &);
	const std::string& get_text();

	// Drawing and event handlers
	void draw(RenderTarget &) override;

	void set_textstyle(const UI::TextStyle & style);
	const UI::TextStyle & get_textstyle() const {return m_textstyle;}

	void set_font(const std::string & name, int size, RGBColor clr);

protected:
	void update_desired_size() override;

private:
	enum LayoutMode {
		AutoMove,
		Layouted,
		Static
	};

	void init();
	void collapse();
	void expand();

	LayoutMode m_layoutmode;
	std::string m_text;
	const Image* rendered_text_;
	Align m_align;
	UI::TextStyle m_textstyle;
	uint32_t fixed_width_;
};

}

#endif  // end of include guard: WL_UI_BASIC_TEXTAREA_H
