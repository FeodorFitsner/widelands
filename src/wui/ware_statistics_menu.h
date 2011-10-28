/*
 * Copyright (C) 2002-2004, 2006-2008 by the Widelands Development Team
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

#ifndef WARE_STATISTICS_MENU_H
#define WARE_STATISTICS_MENU_H

#include "ui_basic/unique_window.h"
#include "plot_area.h"

struct Interactive_Player;
struct WUIPlot_Area;

struct Ware_Statistics_Menu : public UI::UniqueWindow {
	Ware_Statistics_Menu(Interactive_Player &, UI::UniqueWindow::Registry &);

private:
	Interactive_Player * m_parent;
	WUIPlot_Area       * m_plot_production;
	WUIPlot_Area       * m_plot_consumption;

	void clicked_help();
	void cb_changed_to(int32_t, bool);
	void set_time(WUIPlot_Area::TIME);
};

#endif
