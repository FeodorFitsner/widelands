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

#include "mapview.h"

#include "graphic.h"
#include "interactive_base.h"
#include "interactive_player.h"
#include "map.h"
#include "mapviewpixelfunctions.h"
#include "overlay_manager.h"
#include "player.h"
#include "rendertarget.h"
#include "wlapplication.h"


Map_View::Map_View
(UI::Panel * parent, int x, int y, uint w, uint h, Interactive_Base & player)
:
UI::Panel               (parent, x, y, w, h),
m_intbase               (player),
m_viewpoint             (Point(0, 0)),
m_dragging              (false),
m_complete_redraw_needed(true)
{}


/*
===============
Map_View::warp_mouse_to_field

Moves the mouse cursor so that it is directly above the given field
===============
*/
void Map_View::warp_mouse_to_field(Coords c)
{
	const Map & map = intbase().egbase().map();
	Point p;
	MapviewPixelFunctions::get_save_pix(map, c, p.x, p.y);
	p -= m_viewpoint;

	//  If the user has scrolled the field outside the viewable area, he most
	//  surely doesn't want to jump there.
	if (p.x < get_w() and p.y < get_h()) {

		if (p.x <= 0) warp_mouse_to_field(Coords(c.x + map.get_width (), c.y));
		else if (p.y <= 0)
			warp_mouse_to_field(Coords(c.x, c.y + map.get_height()));
		else set_mouse_pos(p);
	}
}


/*
===============
Map_View::draw

This is the guts!! this function draws the whole
map the user can see. we spend a lot of time
in this function
===============
*/
void Map_View::draw(RenderTarget* dst)
{
	Editor_Game_Base & egbase = intbase().egbase();
   // Check if the view has changed in a game
	if
		(dynamic_cast<const Game * const>(&egbase)
		 and
		 dynamic_cast<Interactive_Player &>(intbase())
		 .get_player()->has_view_changed())
		m_complete_redraw_needed = true;

	egbase.map().overlay_manager().load_graphics();
	if
		(const Interactive_Player * const interactive_player =
		 dynamic_cast<const Interactive_Player * const>(&intbase()))
		dst->rendermap
		(egbase,
		 interactive_player->player(),
		 m_viewpoint,
		 m_complete_redraw_needed);
	else dst->rendermap(egbase, m_viewpoint, m_complete_redraw_needed);

   m_complete_redraw_needed = false;
}


/*
===============
Map_View::set_viewpoint

Set the viewpoint to the given pixel coordinates
===============
*/
void Map_View::set_viewpoint(Point vp)
{
	if (vp == m_viewpoint)
		return;

	MapviewPixelFunctions::normalize_pix(intbase().egbase().map(), vp);
	m_viewpoint = vp;

	warpview.call(m_viewpoint.x, m_viewpoint.y);

   m_complete_redraw_needed = true;
}


void Map_View::stop_dragging() {
	WLApplication::get()->set_mouse_lock(false);
	grab_mouse(false);
	m_dragging = false;
}

/**
 * Mousepressess and -releases on the map:
 * Right-press:   enable  dragging
 * Right-release: disable dragging
 * Left-press:    field action window
 */
bool Map_View::handle_mousepress(const Uint8 btn, int x, int y) {
	if (btn == SDL_BUTTON_LEFT) {
		track_sel(Point(x, y));

			fieldclicked.call();
	}
	else if (btn == SDL_BUTTON_RIGHT) {
			m_dragging = true;
			grab_mouse(true);
			WLApplication::get()->set_mouse_lock(true);
	}
	return true;
}
bool Map_View::handle_mouserelease(const Uint8 btn, int, int)
{if (btn == SDL_BUTTON_RIGHT and m_dragging) stop_dragging(); return true;}


/*
===============
Map_View::handle_mousemove

Scroll the view according to mouse movement.
===============
*/
bool Map_View::handle_mousemove
(const Uint8 state, int x, int y, int xdiff, int ydiff)
{
	if (m_dragging)
	{
		if (state & SDL_BUTTON(SDL_BUTTON_RIGHT))
			set_rel_viewpoint(Point(xdiff, ydiff));
		else stop_dragging();
	}

	if (not intbase().get_sel_freeze()) track_sel(Point(x, y));

	g_gr->update_fullscreen();
	return true;
}


/*
===============
Map_View::track_sel(int mx, int my)

Move the sel to the given mouse position.
Does not honour sel freeze.
===============
*/
void Map_View::track_sel(Point m) {
	m += m_viewpoint;
	m_intbase.set_sel_pos
		(MapviewPixelFunctions::calc_node_and_triangle
		 (intbase().egbase().map(), m.x, m.y));
}
