/*
 * Copyright (C) 2002-2004, 2006-2010 by the Widelands Development Team
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

#include "minimap.h"

#include "i18n.h"
#include "interactive_player.h"
#include "logic/map.h"
#include "mapviewpixelconstants.h"

#include "graphic/graphic.h"
#include "graphic/rendertarget.h"
#include "graphic/render/gameview.h"


MiniMap::View::View
	(UI::Panel & parent, int8_t * flags,
	 int32_t const x, int32_t const y, uint32_t const, uint32_t const,
	 Interactive_Base & ibase)
:
	UI::Panel       (&parent, x, y, 10, 10),
	m_ibase       (ibase),
	m_viewx       (0),
	m_viewy       (0),
	m_pic_map_spot(g_gr->get_picture(PicMod_Game, "pics/map_spot.png")),
	m_flags       (flags)
{}


/** MiniMap::View::set_view_pos(int32_t x, int32_t y)
 *
 * Set the view point marker to a new position.
 *
 * Args: x, y  new view point coordinates, in screen coordinates
 */
void MiniMap::View::set_view_pos(const int32_t x, const int32_t y)
{
	m_viewx = x / TRIANGLE_WIDTH;
	m_viewy = y / TRIANGLE_HEIGHT;

	update();
}


void MiniMap::View::draw(RenderTarget & dst)
{
	GameView gameview(dst);

	gameview.renderminimap
		(m_ibase.egbase(),
		 m_ibase.get_player(),
		 (*m_flags) & (MiniMap::Zoom2) ?
		 	Point((m_viewx - get_w() / 4), (m_viewy - get_h() / 4)):
		 	Point((m_viewx - get_w() / 2), (m_viewy - get_h() / 2)),
		 *m_flags);
}


/*
===============
Left-press: warp the view point to the new position
===============
*/
bool MiniMap::View::handle_mousepress(const Uint8 btn, int32_t x, int32_t y) {
	if (btn != SDL_BUTTON_LEFT)
		return false;

	//  calculates the coordinates corresponding to the mouse position
	Widelands::Coords c;
	if (*m_flags & MiniMap::Zoom2)
		c = Widelands::Coords
			(m_viewx + 1 - (get_w() / 2 - x) / 2,
			 m_viewy + 1 - (get_h() / 2 - y) / 2);
	else
		c = Widelands::Coords
			(m_viewx + 1 - get_w() / 2 + x, m_viewy + 1 - get_h() / 2 + y);

	m_ibase.egbase().map().normalize_coords(c);

	ref_cast<MiniMap, UI::Panel>(*get_parent()).warpview.call
		(c.x * TRIANGLE_WIDTH, c.y * TRIANGLE_HEIGHT);

	return true;
}
bool MiniMap::View::handle_mouserelease(Uint8 const btn, int32_t, int32_t) {
	return btn == SDL_BUTTON_LEFT;
}

void MiniMap::View::set_zoom(int32_t z) {
	Widelands::Map const & map = m_ibase.egbase().map();
	set_size((map.get_width() * z), (map.get_height()) * z);
}


/*
==============================================================================

MiniMap

==============================================================================
*/

/*
===============
Initialize the minimap window. Dimensions will be set automatically
according to the map size.
A registry pointer is set to track the MiniMap object (only show one
minimap at a time).

reg, the registry pointer will be set by constructor and cleared by
destructor
===============
*/
inline uint32_t MiniMap::number_of_buttons_per_row() const {return 3;}
inline uint32_t MiniMap::number_of_button_rows    () const {return 2;}
inline uint32_t MiniMap::but_w                    () const {
	return m_view.get_w() / number_of_buttons_per_row();
}
inline uint32_t MiniMap::but_h                    () const {return 20;}
MiniMap::MiniMap(Interactive_Base & ibase, Registry * const registry)
:
	UI::UniqueWindow(&ibase, registry, 0, 0, _("Map")),
	m_view(*this, &registry->flags, 0, 0, 0, 0, ibase),

	button_terrn
		(this,
		 but_w() * 0, m_view.get_h() + but_h() * 0, but_w(), but_h(),
		 g_gr->get_no_picture(),
		 g_gr->get_picture(PicMod_UI, "pics/button_terrn.png"),
		 &MiniMap::toggle, *this, Terrn,
		 _("Terrain")),
	button_owner
		(this,
		 but_w() * 1, m_view.get_h() + but_h() * 0, but_w(), but_h(),
		 g_gr->get_no_picture(),
		 g_gr->get_picture(PicMod_UI, "pics/button_owner.png"),
		 &MiniMap::toggle, *this, Owner,
		 _("Owner")),
	button_flags
		(this,
		 but_w() * 2, m_view.get_h() + but_h() * 0, but_w(), but_h(),
		 g_gr->get_no_picture(),
		 g_gr->get_picture(PicMod_UI, "pics/button_flags.png"),
		 &MiniMap::toggle, *this, Flags,
		 _("Flags")),
	button_roads
		(this,
		 but_w() * 0, m_view.get_h() + but_h() * 1, but_w(), but_h(),
		 g_gr->get_no_picture(),
		 g_gr->get_picture(PicMod_UI, "pics/button_roads.png"),
		 &MiniMap::toggle, *this, Roads,
		 _("Roads")),
	button_bldns
		(this,
		 but_w() * 1, m_view.get_h() + but_h() * 1, but_w(), but_h(),
		 g_gr->get_no_picture(),
		 g_gr->get_picture(PicMod_UI, "pics/button_bldns.png"),
		 &MiniMap::toggle, *this, Bldns,
		 _("Buildings")),
	button_zoom
		(this,
		 but_w() * 2, m_view.get_h() + but_h() * 1, but_w(), but_h(),
		 g_gr->get_no_picture(),
		 g_gr->get_picture(PicMod_UI, "pics/button_zoom.png"),
		 &MiniMap::toggle, *this, Zoom2,
		 _("Zoom"))
{
	resize();

	if (get_usedefaultpos())
		center_to_parent();
}


void MiniMap::toggle(Layers const button) {
	*m_view.m_flags ^= button;
	if (button == Zoom2)
		resize();
}

void MiniMap::resize() {
	m_view.set_zoom(*m_view.m_flags & Zoom2 ? 2 : 1);
	set_inner_size
		(m_view.get_w(), m_view.get_h() + number_of_button_rows() * but_h());
	button_terrn.set_pos(Point(but_w() * 0, m_view.get_h() + but_h() * 0));
	button_terrn.set_size(but_w(), but_h());
	button_owner.set_pos(Point(but_w() * 1, m_view.get_h() + but_h() * 0));
	button_owner.set_size(but_w(), but_h());
	button_flags.set_pos(Point(but_w() * 2, m_view.get_h() + but_h() * 0));
	button_flags.set_size(but_w(), but_h());
	button_roads.set_pos(Point(but_w() * 0, m_view.get_h() + but_h() * 1));
	button_roads.set_size(but_w(), but_h());
	button_bldns.set_pos(Point(but_w() * 1, m_view.get_h() + but_h() * 1));
	button_bldns.set_size(but_w(), but_h());
	button_zoom .set_pos(Point(but_w() * 2, m_view.get_h() + but_h() * 1));
	button_zoom .set_size(but_w(), but_h());
	move_inside_parent();
}
