/*
 * Copyright (C) 2002, 2007-2010 by the Widelands Development Team
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

#include "base.h"

#include "constants.h"
#include "io/filesystem/filesystem.h"
#include "graphic/graphic.h"
#include "log.h"
#include "profile/profile.h"
#include "graphic/rendertarget.h"
#include "wlapplication.h"
#include "wexception.h"

#include <cstdio>

/*
==============================================================================

Fullscreen_Menu_Base

==============================================================================
*/

/**
 * Initialize a pre-game menu
 *
 * Args: bgpic  name of the background picture
 */
Fullscreen_Menu_Base::Fullscreen_Menu_Base(char const * const bgpic)
	: UI::Panel(0, 0, 0, gr_x(), gr_y()),
	// Switch graphics mode if necessary
	m_xres(gr_x()), m_yres(gr_y())
{

	Section & s = g_options.pull_section("global");

#if USE_OPENGL
#define GET_BOOL_USE_OPENGL s.get_bool("opengl", false)
#else
#define GET_BOOL_USE_OPENGL false
#endif
	WLApplication::get()->init_graphics
		(m_xres, m_yres,
		 s.get_int("depth", 16),
		 s.get_bool("fullscreen", false),
		 GET_BOOL_USE_OPENGL);
#undef GET_BOOL_USE_OPENGL


	// Load background graphics
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "pics/%s", bgpic);
	m_pic_background = g_gr->get_picture(PicMod_Menu, buffer, false);
	if (m_pic_background == g_gr->get_no_picture())
		throw wexception
			("could not open splash screen; make sure that all data files are "
			 "installed");

	m_res_background = g_gr->get_no_picture();
	if (g_gr->caps().resize_surfaces)
		m_res_background = g_gr->get_resized_picture
			(m_pic_background, m_xres, m_yres, Graphic::ResizeMode_Loose);
}

Fullscreen_Menu_Base::~Fullscreen_Menu_Base() {
	if
		(m_res_background != g_gr->get_no_picture() and
		 m_res_background != m_pic_background)
		g_gr->free_picture_surface(m_res_background);
}


/**
 * Draw the background / splash screen
*/
void Fullscreen_Menu_Base::draw(RenderTarget & dst) {
	if (g_gr->caps().blit_resized)
		dst.blit(Rect(Point(0, 0), dst.get_w(), dst.get_h()), m_pic_background);
	else
		dst.blit(Point(0, 0), m_res_background);
}


uint32_t Fullscreen_Menu_Base::gr_x() {
	return g_options.pull_section("global").get_int("xres", XRES);
}

uint32_t Fullscreen_Menu_Base::gr_y() {
	return g_options.pull_section("global").get_int("yres", YRES);
}


uint32_t Fullscreen_Menu_Base::fs_small() {
	return UI_FONT_SIZE_SMALL * gr_y() / 600;
}

uint32_t Fullscreen_Menu_Base::fs_big() {
	return UI_FONT_SIZE_BIG * gr_y() / 600;
}

std::string Fullscreen_Menu_Base::ui_fn() {
	std::string style
		(g_options.pull_section("global").get_string
		 	("ui_font", UI_FONT_NAME_SERIF));
	if (style.empty() | (style == "serif"))
		return UI_FONT_NAME_SERIF;
	if (style == "sans")
		return UI_FONT_NAME_SANS;
	std::string const temp(g_fs->FS_CanonicalizeName("fonts/" + style));
	if (g_fs->FileExists(temp))
		return style;
	log
		("Could not find font file \"%s\"\n"
		 "Make sure the path is given relative to Widelands font directory. "
		 "Widelands will use standard font.\n",
		 temp.c_str());
	return UI_FONT_NAME;
}

