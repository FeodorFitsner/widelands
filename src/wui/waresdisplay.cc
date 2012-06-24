/*
 * Copyright (C) 2003, 2006-2009 by the Widelands Development Team
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

#include "waresdisplay.h"

#include "logic/editor_game_base.h"
#include "graphic/font.h"
#include "graphic/font_handler.h"
#include "i18n.h"
#include "logic/player.h"
#include "graphic/font.h"
#include "graphic/rendertarget.h"
#include "logic/tribe.h"
#include "logic/worker.h"

#include "wexception.h"

#include <cstdio>
#include <boost/lexical_cast.hpp>

AbstractWaresDisplay::AbstractWaresDisplay
	(UI::Panel * const parent,
	 int32_t const x, int32_t const y,
	 Widelands::Tribe_Descr const & tribe,
	 Widelands::WareWorker type,
	 bool selectable,
	 boost::function<void(Widelands::Ware_Index, bool)> callback_function,
	 bool horizontal)
	:
	// Size is set when add_warelist is called, as it depends on the m_type.
	UI::Panel(parent, x, y, 0, 0),
	m_tribe (tribe),

	m_type (type),
	m_curware
		(this,
		 0, get_inner_h() - 25, get_inner_w(), 20,
		 _("Stock"), UI::Align_Center),

	m_selected
		(m_type == Widelands::wwWORKER ? m_tribe.get_nrworkers()
	                          : m_tribe.get_nrwares(), false),
	m_hidden
		(m_type == Widelands::wwWORKER ? m_tribe.get_nrworkers()
	                          : m_tribe.get_nrwares(), false),
	m_selectable(selectable),
	m_horizontal(horizontal),
	m_callback_function(callback_function)
{
	//resize the configuration of our wares if they won't fit in the current window
	int number = (g_gr->get_yres() - 160) / (WARE_MENU_PIC_HEIGHT + 8 + 3);
	const_cast<Widelands::Tribe_Descr &>(m_tribe).resize_ware_orders(number);

	// Find out geometry from icons_order
	unsigned int columns = icons_order().size();
	unsigned int rows = 0;
	for (unsigned int i = 0; i < icons_order().size(); i++)
		if (icons_order()[i].size() > rows)
			rows = icons_order()[i].size();
	if (m_horizontal) {
		unsigned int s = columns;
		columns = rows;
		rows = s;
	}

	// 25 is height of m_curware text
	set_desired_size
		(columns * (WARE_MENU_PIC_WIDTH  + 3) + 1,
		 rows * (WARE_MENU_PIC_HEIGHT + 8 + 3) + 1 + 25);
}


bool AbstractWaresDisplay::handle_mousemove
	(Uint8, int32_t const x, int32_t const y, int32_t, int32_t)
{
	Widelands::Ware_Index const index = ware_at_point(x, y);

	m_curware.set_text
		(index ?
		 (m_type == Widelands::wwWORKER ?
		  m_tribe.get_worker_descr(index)->descname()
		  :
		  m_tribe.get_ware_descr  (index)->descname())
		 .c_str()
		 :
		 "");
	return true;
}

bool AbstractWaresDisplay::handle_mousepress
	(Uint8 btn, int32_t const x, int32_t const y)
{
	if (btn == SDL_BUTTON_LEFT) {
		Widelands::Ware_Index ware = ware_at_point(x, y);
		if (!ware)
			return false;

		if (m_selectable) {
			toggle_ware(ware);
		}
		return true;
	}

	return UI::Panel::handle_mousepress(btn, x, y);
}

/**
 * Returns the index of the ware under the given coordinates, or
 * WareIndex::Null() if the given point is outside the range.
 */
Widelands::Ware_Index AbstractWaresDisplay::ware_at_point(int32_t x, int32_t y) const
{
	if (x < 0 || y < 0)
		return Widelands::Ware_Index::Null();


	unsigned int i = x / (WARE_MENU_PIC_WIDTH + 4);
	unsigned int j = y / (WARE_MENU_PIC_HEIGHT + 8 + 3);
	if (m_horizontal) {
		unsigned int s = i;
		i = j;
		j = s;
	}
	if (i < icons_order().size() && j < icons_order()[i].size()) {
		Widelands::Ware_Index ware = icons_order()[i][j];
		if (not m_hidden[ware]) {
			return ware;
		}
	}

	return Widelands::Ware_Index::Null();
}


void AbstractWaresDisplay::layout()
{
	m_curware.set_pos(Point(0, get_inner_h() - 25));
	m_curware.set_size(get_inner_w(), 20);
}

void WaresDisplay::remove_all_warelists() {
	m_warelists.clear();
}


void AbstractWaresDisplay::draw(RenderTarget & dst)
{
	Widelands::Ware_Index number =
		m_type == Widelands::wwWORKER ?
		m_tribe.get_nrworkers() :
		m_tribe.get_nrwares();

	uint8_t totid = 0;
	for
		(Widelands::Ware_Index id = Widelands::Ware_Index::First();
		 id < number;
		 ++id, ++totid)
	{
		if (m_hidden[id]) continue;

		draw_ware(dst, id);
	}
}

Widelands::Tribe_Descr::WaresOrder const & AbstractWaresDisplay::icons_order() const
{
	switch (m_type) {
		case Widelands::wwWARE:
			return m_tribe.wares_order();
			break;
		case Widelands::wwWORKER:
			return m_tribe.workers_order();
			break;
	}
	throw wexception("Invalid m_type %d", m_type);
}

Widelands::Tribe_Descr::WaresOrderCoords const & AbstractWaresDisplay::icons_order_coords() const
{
	switch (m_type) {
		case Widelands::wwWARE:
			return m_tribe.wares_order_coords();
			break;
		case Widelands::wwWORKER:
			return m_tribe.workers_order_coords();
			break;
	}
	throw wexception("Invalid m_type %d", m_type);
}


Point AbstractWaresDisplay::ware_position(Widelands::Ware_Index id) const
{
	Point p(2, 2);
	if (m_horizontal) {
		p.x += icons_order_coords()[id].second  * (WARE_MENU_PIC_WIDTH + 3);
		p.y += icons_order_coords()[id].first * (WARE_MENU_PIC_HEIGHT + 3 + 8);
	} else {
		p.x += icons_order_coords()[id].first  * (WARE_MENU_PIC_WIDTH + 3);
		p.y += icons_order_coords()[id].second * (WARE_MENU_PIC_HEIGHT + 3 + 8);
	}
	return p;
}

/*
===============
WaresDisplay::draw_ware [virtual]

Draw one ware icon + additional information.
===============
*/
void AbstractWaresDisplay::draw_ware
	(RenderTarget & dst,
	 Widelands::Ware_Index id)
{
	Point p = ware_position(id);

	//  draw a background
	const PictureID picid =
		g_gr->get_picture
			(PicMod_Game,
			 ware_selected(id) ?  "pics/ware_list_bg_selected.png"
			                   :  "pics/ware_list_bg.png");
	uint32_t w, h;
	g_gr->get_picture_size(picid, w, h);

	dst.blit(p, picid);

	const Point pos = p + Point((w - WARE_MENU_PIC_WIDTH) / 2, 1);
	// Draw it
	dst.blit
		(pos,
		 m_type == Widelands::wwWORKER ?
		 m_tribe.get_worker_descr(id)->icon()
		 :
		 m_tribe.get_ware_descr  (id)->icon());
	dst.fill_rect
		(Rect(pos + Point(0, WARE_MENU_PIC_HEIGHT), WARE_MENU_PIC_WIDTH, 8),
		 info_color_for_ware(id));

	UI::g_fh->draw_text
		(dst, UI::TextStyle::ui_ultrasmall(),
		 p + Point(WARE_MENU_PIC_WIDTH, WARE_MENU_PIC_HEIGHT - 4),
		 info_for_ware(id),
		 UI::Align_Right);
}

// Wares highlighting/selecting
void AbstractWaresDisplay::select_ware(Widelands::Ware_Index ware)
{
	if (m_selected[ware])
		return;

	m_selected[ware] = true;
	update();
}

void AbstractWaresDisplay::unselect_ware(Widelands::Ware_Index ware)
{
	if (!m_selected[ware])
		return;

	m_selected[ware] = false;
	update();
}

bool AbstractWaresDisplay::ware_selected(Widelands::Ware_Index ware) {
	return m_selected[ware];
}

// Wares hiding
void AbstractWaresDisplay::hide_ware(Widelands::Ware_Index ware)
{
	if (m_hidden[ware])
		return;

	m_hidden[ware] = true;
	update();
}

void AbstractWaresDisplay::unhide_ware(Widelands::Ware_Index ware)
{
	if (!m_hidden[ware])
		return;

	m_hidden[ware] = false;
	update();
}

bool AbstractWaresDisplay::ware_hidden(Widelands::Ware_Index ware) {
	return m_hidden[ware];
}

WaresDisplay::WaresDisplay
	(UI::Panel * const parent,
	 int32_t const x, int32_t const y,
	 Widelands::Tribe_Descr const & tribe,
	 Widelands::WareWorker type,
	 bool selectable)
: AbstractWaresDisplay(parent, x, y, tribe, type, selectable)
{}

RGBColor AbstractWaresDisplay::info_color_for_ware(Widelands::Ware_Index const /* ware */) {
	return RGBColor(0, 0, 0);
}

WaresDisplay::~WaresDisplay()
{
	remove_all_warelists();
}

std::string WaresDisplay::info_for_ware(Widelands::Ware_Index ware) {
	uint32_t totalstock = 0;
	for
		(Widelands::Ware_Index i = Widelands::Ware_Index::First();
		 i.value() < m_warelists.size();
		 ++i)
		totalstock += m_warelists[i]->stock(ware);
	return boost::lexical_cast<std::string>(totalstock);
}

/*
===============
add a ware list to be displayed in this WaresDisplay
===============
*/
void WaresDisplay::add_warelist
	(Widelands::WareList const & wares)
{
	//  If you register something twice, it is counted twice. Not my problem.
	m_warelists.push_back(&wares);
}


/*
====================================================
struct BuildcostDisplay
====================================================
*/

BuildcostDisplay::BuildcostDisplay
	(UI::Panel * const parent,
	 const int32_t x, const int32_t y,
	 int32_t columns,
	 Widelands::Building_Descr const * building)
	:
	UI::Panel (parent, x, y, 0, 0), m_columns(columns)
{
	set_building(building);
}

BuildcostDisplay::~BuildcostDisplay()
{}

void BuildcostDisplay::set_building(Widelands::Building_Descr const * building) {
	m_building = building;
	if (m_building) {
		int32_t c = m_building->buildcost().size();
		set_desired_size
			((c < m_columns ? c : m_columns) * (WARE_MENU_PIC_WIDTH + 4) + 1,
			 ((c / m_columns) + (c % m_columns != 0)) * (WARE_MENU_PIC_HEIGHT + 3 + 8) + 1);
		set_visible(true);
	} else {
		set_desired_size(0, 0);
		set_visible(false);
	}
}

void BuildcostDisplay::draw(RenderTarget & dst)
{
	if (not m_building)
		return;

	Point p = Point(2, 2);
	Widelands::Tribe_Descr const & tribe = m_building->tribe();

	Widelands::Buildcost const & cost = m_building->buildcost();
	Widelands::Buildcost::const_iterator c;

	Widelands::Tribe_Descr::WaresOrder::iterator i;
	std::vector<Widelands::Ware_Index>::iterator j;
	Widelands::Tribe_Descr::WaresOrder order = tribe.wares_order();

	for (i = order.begin(); i != order.end(); i++)
		for (j = i->begin(); j != i->end(); j++)
			if ((c = cost.find(*j)) != cost.end()) {
				//  draw a background
				const PictureID picid =
				g_gr->get_picture (PicMod_Game, "pics/ware_list_bg.png");
				uint32_t w, h;
				g_gr->get_picture_size(picid, w, h);

				dst.blit(p, picid);

				const Point pos = p + Point((w - WARE_MENU_PIC_WIDTH) / 2, 1);
				// Draw it
				dst.blit
				(pos,
				 tribe.get_ware_descr(c->first)->icon());
				dst.fill_rect
				(Rect(pos + Point(0, WARE_MENU_PIC_HEIGHT), WARE_MENU_PIC_WIDTH, 8),
				 RGBColor(0, 0, 0));

				UI::g_fh->draw_text
				(dst, UI::TextStyle::ui_ultrasmall(),
				 p + Point(WARE_MENU_PIC_WIDTH, WARE_MENU_PIC_HEIGHT - 4),
				 boost::lexical_cast<std::string, uint32_t>(c->second),
				 UI::Align_Right);

				p.x += (WARE_MENU_PIC_WIDTH + 4);
				if (p.x >= (m_columns * (WARE_MENU_PIC_WIDTH + 4))) {
					p.x = 2;
					p.y += (WARE_MENU_PIC_HEIGHT + 3 + 8);
				}
			}
}



