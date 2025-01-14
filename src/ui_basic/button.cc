/*
 * Copyright (C) 2002, 2006-2011, 2015 by the Widelands Development Team
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

#include "ui_basic/button.h"

#include "graphic/font_handler1.h"
#include "graphic/image.h"
#include "graphic/rendertarget.h"
#include "graphic/text_constants.h"
#include "graphic/text_layout.h"
#include "ui_basic/mouse_constants.h"
#include "wlapplication.h"

namespace UI {

// Margin around image. The image will be scaled down to fit into this rectangle with preserving size.
constexpr int kButtonImageMargin = 2;

Button::Button //  for textual buttons. If h = 0, h will resize according to the font's height.
	(Panel * const parent,
	 const std::string & name,
	 int32_t const x, int32_t const y, uint32_t const w, uint32_t const h,
	 const Image* bg_pic,
	 const std::string & title_text,
	 const std::string & tooltip_text,
	 bool const _enabled, bool const flat)
	:
	NamedPanel           (parent, name, x, y, w, h, tooltip_text),
	m_highlighted   (false),
	m_pressed       (false),
	m_permpressed   (false),
	m_enabled       (_enabled),
	m_repeating     (false),
	m_flat          (flat),
	m_draw_flat_background(false),
	m_time_nextact  (0),
	m_title         (title_text),
	m_pic_background(bg_pic),
	m_pic_custom    (nullptr),
	m_clr_down      (229, 161, 2)
{
	// Automatically resize for font height and give it a margin.
	if (h < 1) {
		int new_height = UI::g_fh1->render(as_uifont("."))->height() + 4;
		set_desired_size(w, new_height);
		set_size(w, new_height);
	}
	set_thinks(false);
}

Button::Button //  for pictorial buttons
	(Panel * const parent,
	 const std::string & name,
	 const int32_t x, const int32_t y, const uint32_t w, const uint32_t h,
	 const Image* bg_pic,
	 const Image* fg_pic,
	 const std::string & tooltip_text,
	 bool const _enabled, bool const flat, const bool keep_image_size)
	:
	NamedPanel      (parent, name, x, y, w, h, tooltip_text),
	m_highlighted   (false),
	m_pressed       (false),
	m_permpressed   (false),
	m_enabled       (_enabled),
	m_repeating     (false),
	m_flat          (flat),
	m_keep_image_size(keep_image_size),
	m_draw_flat_background(false),
	m_time_nextact  (0),
	m_pic_background(bg_pic),
	m_pic_custom    (fg_pic),
	m_clr_down      (229, 161, 2)
{
	set_thinks(false);
}


Button::~Button()
{
}


/**
 * Sets a new picture for the Button.
*/
void Button::set_pic(const Image* pic)
{
	m_title.clear();

	if (m_pic_custom == pic)
		return;

	m_pic_custom = pic;
}


/**
 * Set a text title for the Button
*/
void Button::set_title(const std::string & title) {
	if (m_title == title)
		return;

	m_pic_custom = nullptr;
	m_title      = title;
}


/**
 * Enable/Disable the button (disabled buttons can't be clicked).
 * Buttons are enabled by default
*/
void Button::set_enabled(bool const on)
{
	if (m_enabled == on)
		return;

	// disabled buttons should look different...
	if (on)
		m_enabled = true;
	else {
		if (m_pressed) {
			m_pressed = false;
			set_thinks(false);
			grab_mouse(false);
		}
		m_enabled = false;
		m_highlighted = false;
	}
}


/**
 * Redraw the button
*/
void Button::draw(RenderTarget & dst)
{
	// Draw the background
	if (!m_flat || m_draw_flat_background) {
		assert(m_pic_background);
		dst.fill_rect(Rect(Point(0, 0), get_w(), get_h()), RGBAColor(0, 0, 0, 255));
		dst.tile(Rect(Point(0, 0), get_w(), get_h()), m_pic_background, Point(get_x(), get_y()));
	}

	if (m_enabled && m_highlighted && !m_flat)
		dst.brighten_rect
			(Rect(Point(0, 0), get_w(), get_h()), MOUSE_OVER_BRIGHT_FACTOR);

	//  If we've got a picture, draw it centered
	if (m_pic_custom) {
		if (m_keep_image_size) {
			if (m_enabled) {
				//  ">> 1" is almost like "/ 2", but simpler for signed types (difference
				//  is that -1 >> 1 is -1 but -1 / 2 is 0).
				dst.blit(
							Point(
								(get_w() - static_cast<int32_t>(m_pic_custom->width())) >> 1,
								(get_h() - static_cast<int32_t>(m_pic_custom->height())) >> 1),
							m_pic_custom);
			} else {
				//  ">> 1" is almost like "/ 2", but simpler for signed types (difference
				//  is that -1 >> 1 is -1 but -1 / 2 is 0).
				dst.blit_monochrome(
							Point(
								(get_w() - static_cast<int32_t>(m_pic_custom->width())) >> 1,
								(get_h() - static_cast<int32_t>(m_pic_custom->height())) >> 1),
							m_pic_custom,
							RGBAColor(255, 255, 255, 127));
			}
		} else {
			const int max_image_w = get_w() - 2 * kButtonImageMargin;
			const int max_image_h = get_h() - 2 * kButtonImageMargin;
			double image_scale =
				std::min(1.,
							std::min(static_cast<double>(max_image_w) / m_pic_custom->width(),
										static_cast<double>(max_image_h) / m_pic_custom->height()));
			int blit_width = image_scale * m_pic_custom->width();
			int blit_height = image_scale * m_pic_custom->height();

			if (m_enabled) {
				dst.blitrect_scale(
					Rect((get_w() - blit_width) / 2, (get_h() - blit_height) / 2, blit_width, blit_height),
					m_pic_custom,
					Rect(0, 0, m_pic_custom->width(), m_pic_custom->height()),
					1.,
					BlendMode::UseAlpha);
			} else {
				dst.blitrect_scale_monochrome(
					Rect((get_w() - blit_width) / 2, (get_h() - blit_height) / 2, blit_width, blit_height),
					m_pic_custom,
					Rect(0, 0, m_pic_custom->width(), m_pic_custom->height()),
					RGBAColor(255, 255, 255, 127));
			}
		}

	} else if (m_title.length()) {
		//  Otherwise draw title string centered
		const Image* entry_text_im = UI::g_fh1->render(
												  as_uifont(m_title,
																UI_FONT_SIZE_SMALL,
																m_enabled ? UI_FONT_CLR_FG : UI_FONT_CLR_DISABLED));
		dst.blit(Point((get_w() - entry_text_im->width()) / 2, (get_h() - entry_text_im->height()) / 2),
					entry_text_im);
	}

	//  draw border
	//  a pressed but not highlighted button occurs when the user has pressed
	//  the left mouse button and then left the area of the button or the button
	//  stays pressed when it is pressed once
	RGBAColor black(0, 0, 0, 255);

	// m_permpressed is true, we invert the behaviour on m_pressed
	bool draw_pressed = m_permpressed ?    !(m_pressed && m_highlighted)
	                                  :     (m_pressed && m_highlighted);

	if (!m_flat) {
		assert(2 <= get_w());
		assert(2 <= get_h());
		//  button is a normal one, not flat
		if (!draw_pressed) {
			//  top edge
			dst.brighten_rect
				(Rect(Point(0, 0), get_w(), 2), BUTTON_EDGE_BRIGHT_FACTOR);
			//  left edge
			dst.brighten_rect
				(Rect(Point(0, 2), 2, get_h() - 2), BUTTON_EDGE_BRIGHT_FACTOR);
			//  bottom edge
			dst.fill_rect(Rect(Point(2, get_h() - 2), get_w() - 2, 1), black);
			dst.fill_rect(Rect(Point(1, get_h() - 1), get_w() - 1, 1), black);
			//  right edge
			dst.fill_rect(Rect(Point(get_w() - 2, 2), 1, get_h() - 2), black);
			dst.fill_rect(Rect(Point(get_w() - 1, 1), 1, get_h() - 1), black);
		} else {
			//  bottom edge
			dst.brighten_rect
				(Rect(Point(0, get_h() - 2), get_w(), 2),
				 BUTTON_EDGE_BRIGHT_FACTOR);
			//  right edge
			dst.brighten_rect
				(Rect(Point(get_w() - 2, 0), 2, get_h() - 2),
				 BUTTON_EDGE_BRIGHT_FACTOR);
			//  top edge
			dst.fill_rect(Rect(Point(0, 0), get_w() - 1, 1), black);
			dst.fill_rect(Rect(Point(0, 1), get_w() - 2, 1), black);
			//  left edge
			dst.fill_rect(Rect(Point(0, 0), 1, get_h() - 1), black);
			dst.fill_rect(Rect(Point(1, 0), 1, get_h() - 2), black);
		}
	} else {
		//  Button is flat, do not draw borders, instead, if it is pressed, draw
		//  a box around it.
		if (m_enabled && m_highlighted)
		{
			RGBAColor shade(100, 100, 100, 80);
			dst.fill_rect(Rect(Point(0, 0), get_w(), 2), shade);
			dst.fill_rect(Rect(Point(0, 2), 2, get_h() - 2), shade);
			dst.fill_rect(Rect(Point(0, get_h() - 2), get_w(), get_h()), shade);
			dst.fill_rect(Rect(Point(get_w() - 2, 0), get_w(), get_h()), shade);
		}
	}
}

void Button::think()
{
	assert(m_repeating);
	assert(m_pressed);
	Panel::think();

	if (m_highlighted) {
		uint32_t const time = SDL_GetTicks();
		if (m_time_nextact <= time) {
			m_time_nextact += MOUSE_BUTTON_AUTOREPEAT_TICK; //  schedule next tick
			if (m_time_nextact < time)
				m_time_nextact = time;
			play_click();
			sigclicked();
			clicked();
			//  The button may not exist at this point (for example if the button
			//  closed the dialog that it is part of). So member variables may no
			//  longer be accessed.
		}
	}
}

/**
 * Update highlighted status
*/
void Button::handle_mousein(bool const inside)
{
	bool oldhl = m_highlighted;

	m_highlighted = inside && m_enabled;

	if (oldhl == m_highlighted)
		return;

	if (m_highlighted)
		sigmousein();
	else
		sigmouseout();
}


/**
 * Update the pressed status of the button
*/
bool Button::handle_mousepress(uint8_t const btn, int32_t, int32_t) {
	if (btn != SDL_BUTTON_LEFT)
		return false;

	if (m_enabled) {
		grab_mouse(true);
		m_pressed = true;
		if (m_repeating) {
			m_time_nextact =
				SDL_GetTicks() + MOUSE_BUTTON_AUTOREPEAT_DELAY;
			set_thinks(true);
		}
	}
	return true;
}

bool Button::handle_mouserelease(uint8_t const btn, int32_t, int32_t) {
	if (btn != SDL_BUTTON_LEFT)
		return false;

	if (m_pressed) {
		m_pressed = false;
		set_thinks(false);
		grab_mouse(false);
		if (m_highlighted && m_enabled) {
			play_click();
			sigclicked();
			clicked();
			//  The button may not exist at this point (for example if the button
			//  closed the dialog that it is part of). So member variables may no
			//  longer be accessed.
		}
	}
	return true;
}

bool Button::handle_mousemove(const uint8_t, int32_t, int32_t, int32_t, int32_t) {
	return true; // We handle this always by lighting up
}

void Button::set_perm_pressed(bool state) {
	if (state != m_permpressed) {
		m_permpressed = state;
	}
}

void Button::set_flat(bool flat) {
	m_flat = flat;
}

void Button::set_draw_flat_background(bool set) {
	m_draw_flat_background = set;
}

}
