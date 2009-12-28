/*
 * Copyright (C) 2002-2004, 2006-2009 by the Widelands Development Team
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

#include "event_allow_retreat_change.h"

#include "logic/game.h"
#include "logic/player.h"
#include "profile/profile.h"

#include "i18n.h"
#include "log.h"

namespace Widelands {

void Event_Allow_Retreat_Change::Write
	(Section & s, Editor_Game_Base & egbase) const
{
	s.set_string("type",  "allow_retreat_change");
	Event_Player_Allowed_Retreat_Change::Write(s, egbase);
}


char const * Event_Allow_Retreat_Change::action_name() const {
	return _("Allow");
}


Event::State Event_Allow_Retreat_Change::run(Game & game) {
	game.player(m_player_number).allow_retreat_change(true);
	return m_state = DONE;
}

}