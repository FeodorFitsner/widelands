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

#ifndef __S__EVENT_CONQUER_AREA_H
#define __S__EVENT_CONQUER_AREA_H

#include "event_player_area.h"
#include "i18n.h"

/// Shows a message box.
struct Event_Conquer_Area : public Event_Player_Area {
	friend struct Event_Conquer_Area_Option_Menu;
	Event_Conquer_Area()
		:
		Event_Player_Area
		(_("Conquer Area"), Player_Area<>(0, Area<>(Coords(0, 0), 5)))
	{}



	const char * get_id() const {return "conquer_area";}

      State run(Game*);
};

#endif
