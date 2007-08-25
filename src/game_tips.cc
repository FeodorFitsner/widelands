/*
 * Copyright (C) 2007 by the Widelands Development Team
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

#include "game_tips.h"

#include "fileread.h"
#include "font_handler.h"
#include "i18n.h"
#include "profile.h"

#define GAMETIPS_DEFAULT_INTERVAL     5
#define GAMETIPS_DEFAULT_TEXTAREA_W   250
#define GAMETIPS_DEFAULT_TEXTAREA_H   150
#define GAMETIPS_TEXTAREA_COLOR_BG    RGBColor(196,196,255)
#define GAMETIPS_FONT_COLOR_FG        RGBColor(0,0,32)
#define GAMETIPS_TEXTAREA_BORDER      2
#define GAMETIPS_DEFAULT_PADDING      15

GameTips::GameTips(UI::ProgressWindow & progressWindow)
	: m_lastUpdated(0), m_updateAfter(0),
	  m_progressWindow(progressWindow), m_registered(false),
	  m_lastTip(0) {

    // Loading texts-locals, for translating the tips
    i18n::grab_textdomain("texts");

	// Skip, if no triggers saved
	// Load the TrueType Font
	std::string filename = "txts/gametips";

	try {
		Profile prof(filename.c_str(), NULL);
		Section* s = prof.get_section("global");
		if (NULL != s) {
			const char *text = s->get_string("background", NULL);
			if (NULL != text)
				m_background_picture = text;
			m_width = s->get_int("width", GAMETIPS_DEFAULT_TEXTAREA_W);
			m_height = s->get_int("height", GAMETIPS_DEFAULT_TEXTAREA_H);
			m_pading_l = s->get_int("padding-left", GAMETIPS_DEFAULT_PADDING);
			m_pading_r = s->get_int("padding-right", GAMETIPS_DEFAULT_PADDING);
			m_pading_t = s->get_int("padding-top", GAMETIPS_DEFAULT_PADDING);
			m_pading_b = s->get_int("padding-bottom", GAMETIPS_DEFAULT_PADDING);
			m_font_size = s->get_int("text-size", UI_FONT_SIZE_SMALL);
			m_bgcolor = color_from_hex(s->get_string("background-color", NULL),
									GAMETIPS_TEXTAREA_COLOR_BG);
			m_color   = color_from_hex(s->get_string("text-color", NULL),
									GAMETIPS_FONT_COLOR_FG);
		}

		while(( s = prof.get_next_section(0)) ) {


			const char *text = s->get_string("text", NULL);
			if (NULL == text)
				continue;

			Tip tip;
			tip.text = text;
			tip.interval = s->get_int("sec", GAMETIPS_DEFAULT_INTERVAL);
			m_tips.push_back (tip);
		}
	}
	catch(std::exception &) {
		// just ignore - tips do not impact game
		return;
	}

	if (m_tips.size() > 0) {
		// add visualization only if any tips are loaded
		m_progressWindow.add_visualization(this);
		m_registered = true;
		m_lastTip = m_tips.size();
	}

    // Now drop textdomain, to fall back to previous one.
    i18n::release_textdomain();

}

GameTips::~GameTips() {
	stop();
}

// conver 2-char hex string into uint
const uint GameTips::colorvalue_from_hex(const char c1, const char c2) {
	uint ret = 0;
	if (c1 >= '0' && c1 <= '9')
		ret += (c1 - '0') * 16;
	else if (c1 >= 'a' && c1 <= 'f')
		ret += (c1 - 'a' + 0xa) * 16;
	else if (c1 >= 'A' && c1 <= 'F')
		ret += (c1 - 'A' + 0xa) * 16;
	else
		return 0x100;

	if (c2 >= '0' && c2 <= '9')
		ret += (c2 - '0');
	else if (c2 >= 'a' && c2 <= 'f')
		ret += (c2 - 'a' + 0xa);
	else if (c2 >= 'A' && c2 <= 'F')
		ret += (c2 - 'A' + 0xa);
	else
		return 0x100;

	return ret;
}

// convert CSS hex color value to RGBColor
const RGBColor GameTips::color_from_hex(
		const char * hexcode,
		const RGBColor & def)
{
	if (NULL == hexcode)
		return def;
	const int len = strlen(hexcode);
	if (len < 3)
		return def;

	// skip leading '#'
	if (hexcode[0] == '#')
		hexcode++;

	uint red   = 0x100;
	uint green = 0x100;
	uint blue  = 0x100;
	if (len == 3) {
		// #fde -> #ffddee (by CSS standard)
		red   = colorvalue_from_hex (hexcode[0], hexcode[0]);
		green = colorvalue_from_hex (hexcode[1], hexcode[1]);
		blue  = colorvalue_from_hex (hexcode[2], hexcode[2]);
	} else if (len == 6) {
		red   = colorvalue_from_hex (hexcode[0], hexcode[1]);
		green = colorvalue_from_hex (hexcode[2], hexcode[3]);
		blue  = colorvalue_from_hex (hexcode[4], hexcode[5]);
	}

	// if anything went wrong, return default
	if (0x100 == red || 0x100 == green || 0x100 == blue)
		return def;

	return RGBColor (red, green, blue);
}

void GameTips::update(bool repaint) {
	Uint32 ticks = SDL_GetTicks();
	if (ticks >= (m_lastUpdated + m_updateAfter)) {
		const uint next = rand() % m_tips.size();
		if (next == m_lastTip)
			m_lastTip = (next + 1) % m_tips.size();
		else
			m_lastTip = next;
		show_tip(next);
		m_lastUpdated = SDL_GetTicks();
		m_updateAfter = m_tips[next].interval * 1000;
	} else if (repaint) {
		show_tip(m_lastTip);
	}
}

void GameTips::stop() {
	if (m_registered) {
		m_progressWindow.remove_visualization(this);
		m_registered = false;
	}
}

void GameTips::show_tip(int index) {
	RenderTarget & rt = *g_gr->get_render_target();
	const uint xres = g_gr->get_xres();
	const uint yres = g_gr->get_yres();
	Rect tips_area;
	Rect text_area;

	// try to load a background
	const uint pic_background =
		g_gr->get_picture(PicMod_Menu, m_background_picture.c_str());
	if (pic_background > 0) {
		uint w = 0;
		uint h = 0;
		g_gr->get_picture_size(pic_background, w, h);
		// center picture
		Point pt((xres - w) / 2,
				 (yres - h) / 2);
		tips_area = Rect(pt, w, h);
		rt.blit(pt, pic_background);
	} else {
		// if no background picture, just draw a rectangle
		Point pt((xres - m_width) / 2 + GAMETIPS_TEXTAREA_BORDER,
				 (yres - m_height) / 2 + GAMETIPS_TEXTAREA_BORDER);
		tips_area = Rect(pt, m_width - 2 * GAMETIPS_TEXTAREA_BORDER,
						 m_height - 2 * GAMETIPS_TEXTAREA_BORDER);
		rt.fill_rect(tips_area, m_bgcolor);

		tips_area.x -= GAMETIPS_TEXTAREA_BORDER;
		tips_area.y -= GAMETIPS_TEXTAREA_BORDER;
		tips_area.w += 2 * GAMETIPS_TEXTAREA_BORDER;
		tips_area.h += 2 * GAMETIPS_TEXTAREA_BORDER;
		rt.draw_rect(tips_area, m_bgcolor);
	}

	text_area = tips_area;
	text_area.x += m_pading_l;
	text_area.w -= m_pading_l + m_pading_r;
	text_area.y += m_pading_t;
	text_area.h -= m_pading_t + m_pading_b;
	Point center(text_area.x + text_area.w / 2,
				 text_area.y + text_area.h / 2);

	g_fh->draw_string(rt, UI_FONT_NAME, m_font_size,
					  m_color, m_bgcolor,//RGBColor(107,87,55),
					  center, m_tips[index].text.c_str(),
					  Align_Center, text_area.w);
	g_gr->update_rectangle(tips_area);
}
