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

#include "ui_progresswindow.h"

#include "font_handler.h"
#include "i18n.h"

#define PROGRESS_FONT_COLOR_FG        RGBColor(128,128,255)
#define PROGRESS_FONT_COLOR_BG        RGBColor(64,64,0)
#define PROGRESS_FONT_COLOR PROGRESS_FONT_COLOR_FG, PROGRESS_FONT_COLOR_BG
#define PROGRESS_STATUS_RECT_PADDING  2
#define PROGRESS_STATUS_BORDER_X      2
#define PROGRESS_STATUS_BORDER_Y      2
#define PROGRESS_LABEL_POSITION_Y     90 /* in percents, from top */

namespace UI {

ProgressWindow::ProgressWindow() : m_xres(0), m_yres(0) {
	step(_("Preparing..."));
}

void ProgressWindow::draw_background(
		RenderTarget & rt, const uint xres, const uint yres) {
	
	m_label_center.x = xres / 2;
	m_label_center.y = yres * PROGRESS_LABEL_POSITION_Y / 100;
	
	// Load background graphics
	Rect wnd_rect(Point(0, 0), xres, yres);
	const uint pic_tile =
		g_gr->get_picture(PicMod_Menu, "pics/progress_bg.png");
	if (pic_tile > 0) {
		rt.tile(wnd_rect, pic_tile, Point(0, 0));
	}
	
	const uint pic_background =
		g_gr->get_picture(PicMod_Menu, "pics/progress.png");
	if (pic_background > 0) {
		uint w = 0;
		uint h = 0;
		g_gr->get_picture_size(pic_background, w, h);
		// center picture horizontally
		Point pt((xres - w) / 2, 0);
		rt.blitrect(pt, pic_background, wnd_rect);
	}
	
	const uint h = g_fh->get_fontheight (UI_FONT_SMALL);
	m_label_rectangle.x = xres / 4;
	m_label_rectangle.w = xres / 2;
	m_label_rectangle.y =
		m_label_center.y - h / 2 - PROGRESS_STATUS_RECT_PADDING;
	m_label_rectangle.h = h + 2 * PROGRESS_STATUS_RECT_PADDING;
	
	Rect border_rect = m_label_rectangle;
	border_rect.x -= PROGRESS_STATUS_BORDER_X;
	border_rect.y -= PROGRESS_STATUS_BORDER_Y;
	border_rect.w += 2 * PROGRESS_STATUS_BORDER_X;
	border_rect.h += 2 * PROGRESS_STATUS_BORDER_Y;
	
	rt.draw_rect(border_rect, PROGRESS_FONT_COLOR_FG);
	
	// remember last resolution
	m_xres = xres;
	m_yres = yres;
}

void ProgressWindow::step(const std::string & description) {
	RenderTarget & rt = *g_gr->get_render_target();

	const uint xres = g_gr->get_xres();
	const uint yres = g_gr->get_yres();
	// if resolution is changed, repaint the background
	if (xres != m_xres or yres != m_yres)
		draw_background(rt, xres, yres);

	rt.fill_rect(m_label_rectangle, PROGRESS_FONT_COLOR_BG);
	g_fh->draw_string
		(rt, UI_FONT_SMALL, PROGRESS_FONT_COLOR, m_label_center, description, Align_Center);
	g_gr->update_rectangle(m_label_rectangle);
	g_gr->refresh();
}

/**
 * Display a loader step description
 */
void ProgressWindow::stepf(const std::string & format, ...) {
	char buffer[1024];
	va_list va;
	va_start(va, format);
	vsnprintf(buffer, sizeof(buffer), format.c_str(), va);
	va_end(va);
	step (buffer);
}
};
