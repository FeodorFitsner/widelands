/*
 * Copyright (C) 2002, 2006, 2008-2010 by the Widelands Development Team
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

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "constants.h"
#include "panel.h"
#include "m_signal.h"

#include "rgbcolor.h"

namespace UI {

struct Font;

/// This is simply a button. Override void clicked() to react to the click.
/// This is all that is needed in most cases, but if there is a need to give a
/// callback function to the button, there are some templates for that below.
struct Button : public NamedPanel {
	Button /// for textual buttons
		(Panel * const parent,
		 std::string const & name,
		 int32_t const x, int32_t const y, uint32_t const w, uint32_t const h,
		 PictureID const background_pictute_id,
		 std::string const & title_text,
		 std::string const & tooltip_text = std::string(),
		 bool const _enabled = true,
		 bool const flat    = false);
	Button /// for pictorial buttons
		(Panel * const parent,
		 std::string const & name,
		 const int32_t x, const int32_t y, const uint32_t w, const uint32_t h,
		 const PictureID background_pictute_id,
		 const PictureID foreground_picture_id,
		 std::string const & tooltip_text = std::string(),
		 bool const _enabled = true,
		 bool const flat     = false);
	~Button();

	void set_pic(PictureID picid);
	void set_title(const std::string &);
	const std::string & get_title() const throw () {return m_title;}

	bool enabled() const {return m_enabled;}
	void set_enabled(bool on);
	void set_repeating(bool const on) {m_repeating = on;}
	void set_draw_caret(bool draw_caret) {m_draw_caret = draw_caret;}
	void set_font(Font * font) {m_font = font;}
	bool is_snap_target() const {return true;}

	// Drawing and event handlers
	void draw(RenderTarget &)__attribute__((hot));
	void think();

	void handle_mousein(bool inside);
	bool handle_mousepress  (Uint8 btn, int32_t x, int32_t y);
	bool handle_mouserelease(Uint8 btn, int32_t x, int32_t y);
	bool handle_mousemove(Uint8, int32_t, int32_t, int32_t, int32_t);

	// Set the permanently pressed state of the button
	void set_perm_pressed(bool state);

protected:
	virtual void clicked() = 0; /// Override this to react on the click.

	bool        m_highlighted;    //  mouse is over the button
	bool        m_pressed;        //  mouse is clicked over the button
	bool        m_permpressed;    //  button should appear  pressed
	bool        m_enabled;
	bool        m_repeating;
	bool        m_flat;

	int32_t     m_time_nextact;

	std::string m_title;          //  title string used when _mypic == 0

	PictureID   m_pic_background; //  background texture (picture ID)
	PictureID   m_pic_custom;     //  custom icon on the button
	PictureID   m_pic_custom_disabled;
	Font * m_font;

	RGBColor    m_clr_down; //  color of border while a flat button is "down"
	bool        m_draw_caret;
};


/// A verion of Button that uses a function object to the the callback.
struct Callback_Button : public Button {
	Callback_Button /// for textual buttons
		(Panel * const parent,
		 std::string const & name,
		 const int32_t x, const int32_t y, const uint32_t w, const uint32_t h,
		 const PictureID background_pictute_id,
		 boost::function<void()> callback_function,
		 const std::string & title_text,
		 std::string const & tooltip_text = std::string(),
		 bool const _enabled = true,
		 bool const flat     = false)
		:
		Button
			(parent, name,
			 x, y, w, h,
			 background_pictute_id,
			 title_text,
			 tooltip_text,
			 _enabled, flat),
		_callback_function     (callback_function)
	{}
	Callback_Button /// for pictorial buttons
		(Panel * const parent,
		 std::string const & name,
		 const int32_t x, const int32_t y, const uint32_t w, const uint32_t h,
		 const PictureID background_pictute_id,
		 const PictureID foreground_picture_id,
		 boost::function<void()> callback_function,
		 std::string const & tooltip_text = std::string(),
		 bool const _enabled = true,
		 bool const flat     = false)
		:
		Button
			(parent, name,
			 x, y, w, h,
			 background_pictute_id,
			 foreground_picture_id,
			 tooltip_text,
			 _enabled, flat),
		_callback_function     (callback_function)
	{}

protected:
	boost::function<void()> _callback_function;
	void clicked() {_callback_function();}
};

}

#endif
