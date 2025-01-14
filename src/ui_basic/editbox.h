/*
 * Copyright (C) 2003, 2006-2008, 2011 by the Widelands Development Team
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

#ifndef WL_UI_BASIC_EDITBOX_H
#define WL_UI_BASIC_EDITBOX_H

#include <memory>

#include <SDL_keyboard.h>
#include <boost/signals2.hpp>

#include "graphic/align.h"
#include "ui_basic/button.h"
#include "graphic/graphic.h"

#define CHAT_HISTORY_SIZE 5

namespace UI {

struct EditBoxImpl;

/// An editbox can be clicked, then the user can change its text (title). When
/// return is pressed, the editbox is unfocused, the keyboard released and a
/// callback function is called
struct EditBox : public Panel {
	EditBox
		(Panel *,
		 int32_t x, int32_t y, uint32_t w,
		 const Image* background = g_gr->images().get("images/ui_basic/but2.png"),
		 int font_size = UI_FONT_SIZE_SMALL);
	virtual ~EditBox();

	boost::signals2::signal<void ()> changed;
	boost::signals2::signal<void ()> ok;
	boost::signals2::signal<void ()> cancel;

	const std::string & text() const;
	void set_text(const std::string &);
	uint32_t max_length() const;
	void set_max_length(uint32_t);

	void activate_history(bool activate) {m_history_active = activate;}

	bool handle_mousepress(uint8_t btn, int32_t x, int32_t y) override;
	bool handle_mouserelease(uint8_t btn, int32_t x, int32_t y) override;
	bool handle_key(bool down, SDL_Keysym) override;
	bool handle_textinput(const std::string& text) override;

	void draw(RenderTarget &) override;

private:
	std::unique_ptr<EditBoxImpl> m;

	void check_caret();

	bool        m_history_active;
	int16_t     m_history_position;
	std::string m_history[CHAT_HISTORY_SIZE];
};

}

#endif  // end of include guard: WL_UI_BASIC_EDITBOX_H
