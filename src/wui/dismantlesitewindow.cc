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


#include "buildingwindow.h"
#include "logic/dismantlesite.h"
#include "ui_basic/progressbar.h"
#include "ui_basic/tabpanel.h"

static const char pic_tab_wares[] = "pics/menu_tab_wares.png";

/**
 * Status window for dismantle sites.
 */
struct DismantleSite_Window : public Building_Window {
	DismantleSite_Window
		(Interactive_GameBase        & parent,
		 Widelands::DismantleSite &,
		 UI::Window *                & registry);

	virtual void think();

private:
	UI::Progress_Bar * m_progress;
};


DismantleSite_Window::DismantleSite_Window
	(Interactive_GameBase        & parent,
	 Widelands::DismantleSite & cs,
	 UI::Window *                & registry)
	: Building_Window(parent, cs, registry)
{
	UI::Box & box = *new UI::Box(get_tabs(), 0, 0, UI::Box::Vertical);

	// Add the progress bar
	m_progress =
		new UI::Progress_Bar
			(&box,
			 0, 0,
			 UI::Progress_Bar::DefaultWidth, UI::Progress_Bar::DefaultHeight,
			 UI::Progress_Bar::Horizontal);
	m_progress->set_total(1 << 16);
	box.add(m_progress, UI::Box::AlignCenter);

	box.add_space(8);

	// Add the wares queue
#if 0 // TODO: SirVer
	for (uint32_t i = 0; i < cs.get_nrwaresqueues(); ++i)
		Building_Window::create_ware_queue_panel(&box, cs, cs.get_waresqueue(i));
#endif
	get_tabs()->add("wares", g_gr->get_picture(PicMod_UI, pic_tab_wares), &box);
}


/*
===============
Make sure the window is redrawn when necessary.
===============
*/
void DismantleSite_Window::think()
{
	Building_Window::think();

	Widelands::DismantleSite const & cs =
		ref_cast<Widelands::DismantleSite, Widelands::Building>(building());

	// m_progress->set_state(cs.get_built_per64k());
}


/*
===============
Create the status window describing the site.
===============
*/
void Widelands::DismantleSite::create_options_window
	(Interactive_GameBase & parent, UI::Window * & registry)
{
	new DismantleSite_Window(parent, *this, registry);
}
