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

#ifndef __S__EVENT_UNHIDE_AREA_H
#define __S__EVENT_UNHIDE_AREA_H

#include "editor_game_base.h"
#include "event_player_area.h"
#include "i18n.h"

/**
 * Makes an area seen to a player. duration determines how long time the area
 * will be in full vision for the player. Valid values are:
 * 0:                           Only gives a static view.
 * Editor_Game_Base::Forever(): Gives a full view forever.
 * others:                      Gives a full view during the specified duration.
 */
struct Event_Unhide_Area : public Event_Player_Area {
	friend struct Event_Unhide_Area_Option_Menu;
	Event_Unhide_Area()
		:
		Event_Player_Area
		(_("Unhide Area"), Player_Area<>(1, Area<>(Coords(0, 0), 5))),
		duration(1 << 14)
	{}

	const char * get_id() const {return "unhide_area";}

      State run(Game*);

	void Write(Section &, const Editor_Game_Base &) const;
	void Read (Section *,       Editor_Game_Base *);

private:
	Editor_Game_Base::Duration duration;
};

#endif
