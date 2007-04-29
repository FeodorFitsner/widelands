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

#ifndef __S__EDITOR_SET_STARTING_POS_TOOL_H
#define __S__EDITOR_SET_STARTING_POS_TOOL_H

#include "editor_tool.h"

#include <string>

// How much place should be left around a player position
// where no other player can start
#define MIN_PLACE_AROUND_PLAYERS 24
#define STARTING_POS_HOTSPOT_Y 55



/// Sets the starting position of players.
struct Editor_Set_Starting_Pos_Tool : public Editor_Tool {
	Editor_Set_Starting_Pos_Tool();

	int handle_click_impl(Map &, const Node_and_Triangle<>, Editor_Interactive &);
	const char * get_sel_impl() const throw ()
	{return m_current_sel_pic.size() ? m_current_sel_pic.c_str() : 0;}

      // tool functions
	Player_Number get_current_player() const throw ();
      void set_current_player(int i);

private:
      std::string m_current_sel_pic;
};

int Editor_Tool_Set_Starting_Pos_Callback(const TCoords<FCoords>, void *, int);

#endif
