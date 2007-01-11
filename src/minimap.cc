/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
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

 // just testing - philipp

#include "graphic.h"
#include "i18n.h"
#include "interactive_base.h"
#include "map.h"
#include "mapviewpixelconstants.h"
#include "minimap.h"
#include "rendertarget.h"

/*
==============================================================================

MiniMapView

==============================================================================
*/


/*
===============
MiniMapView::MiniMapView

Initialize the minimap object
===============
*/
MiniMap::View::View
(UI::Panel & parent,
 const  int x, const  int y,
 const uint w, const uint h,
 Interactive_Base & iabase)
:
UI::Panel       (&parent, x, y, 10, 10),
m_iabase      (iabase),
m_viewx       (0),
m_viewy       (0),
m_pic_map_spot(g_gr->get_picture(PicMod_Game, "pics/map_spot.png")),
flags         (MiniMap::Terrn)
{
	const Map & map = iabase.egbase().map();
	set_size(w ? w : map.get_width(), h ? h : map.get_height());
}


/** MiniMapView::set_view_pos(int x, int y)
 *
 * Set the view point marker to a new position.
 *
 * Args: x, y  new view point coordinates, in screen coordinates
 */
void MiniMap::View::set_view_pos(const int x, const int y)
{
	m_viewx = x / TRIANGLE_WIDTH;
	m_viewy = y / TRIANGLE_HEIGHT;

   update(0, 0, get_w(), get_h());
}

/*
===============
MiniMapView::draw

Redraw the view of the map
===============
*/
void MiniMap::View::draw(RenderTarget* dst)
{
	const std::vector<bool> * visibility = m_iabase.get_visibility();
	const Point p(m_viewx - get_w() / 2, m_viewy - get_h() / 2);
	if (visibility) dst->renderminimap(m_iabase.egbase(), *visibility, p, flags);
	else            dst->renderminimap(m_iabase.egbase(),              p, flags);
}


/*
===============
MiniMapView::handle_mouseclick

Left-press: warp the view point to the new position
===============
*/
bool MiniMap::View::handle_mousepress(const Uint8 btn, int x, int y) {
	if (btn != SDL_BUTTON_LEFT) return false;

		Coords c;

		// calculates the coordinates corresponding to the mouse position
      c.x = m_viewx + 1 - (get_w() / 2) + x;
      c.y = m_viewy + 1 - (get_h() / 2) + y;

	m_iabase.egbase().map().normalize_coords(&c);

	assert(dynamic_cast<const MiniMap * const>(get_parent()));
	static_cast<const MiniMap * const>(get_parent())->warpview.call
		(c.x * TRIANGLE_WIDTH, c.y * TRIANGLE_HEIGHT);

	return true;
}
bool MiniMap::View::handle_mouserelease(const Uint8 btn, int, int)
{return btn == SDL_BUTTON_LEFT;}


/*
==============================================================================

MiniMap

==============================================================================
*/

/*
===============
MiniMap::MiniMap

Initialize the minimap window. Dimensions will be set automatically
according to the map size.
A registry pointer is set to track the MiniMap object (only show one
minimap at a time).

reg, the registry pointer will be set by constructor and cleared by
destructor
===============
*/
inline uint MiniMap::number_of_buttons_per_row() const throw () {return 3;}
inline uint MiniMap::number_of_button_rows    () const throw () {return 2;}
inline uint MiniMap::but_w() const throw ()
{return m_view.get_w() / number_of_buttons_per_row();}
inline uint MiniMap::but_h() const throw () {return 20;}
MiniMap::MiniMap(Interactive_Base & iabase, UI::UniqueWindow::Registry * registry)
:
UI::UniqueWindow(&iabase, registry, 0, 0, _("Map")),
m_view(*this, 0, 0, 0, 0, iabase),

button_terrn
(this,
 but_w() * 0, m_view.get_h() + but_h() * 0, but_w(), but_h(),
 0,
 g_gr->get_picture(PicMod_UI, "pics/button_terrn.png"),
 &MiniMap::toggle, this, Terrn,
 _("Terrain")),

button_owner
(this,
 but_w() * 1, m_view.get_h() + but_h() * 0, but_w(), but_h(),
 0,
 g_gr->get_picture(PicMod_UI, "pics/button_owner.png"),
 &MiniMap::toggle, this, Owner,
 _("Owner")),

button_flags
(this,
 but_w() * 2, m_view.get_h() + but_h() * 0, but_w(), but_h(),
 0,
 g_gr->get_picture(PicMod_UI, "pics/button_flags.png"),
 &MiniMap::toggle, this, Flags,
 _("Flags")),

button_roads
(this,
 but_w() * 0, m_view.get_h() + but_h() * 1, but_w(), but_h(),
 0,
 g_gr->get_picture(PicMod_UI, "pics/button_roads.png"),
 &MiniMap::toggle, this, Roads,
 _("Roads")),

button_bldns
(this,
 but_w() * 1, m_view.get_h() + but_h() * 1, but_w(), but_h(),
 0,
 g_gr->get_picture(PicMod_UI, "pics/button_bldns.png"),
 &MiniMap::toggle, this, Bldns,
 _("Buildings"))

{
	set_inner_size
		(m_view.get_w(), m_view.get_h() + number_of_button_rows() * but_h());

	if (get_usedefaultpos()) center_to_parent();
}


void MiniMap::toggle(Layers button) {m_view.flags ^= button;}
