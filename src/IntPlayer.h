/*
 * Copyright (C) 2002 by The Widelands Development Team
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

#ifndef __S__INTPLAYER_H
#define __S__INTPLAYER_H

#include "ui.h"

class Game;
class Player;
class MiniMap;
class Map_View;
class Window;

struct UniqueWindow {
	Window	*window;
	int		x, y;
	
	inline UniqueWindow() : window(0), x(-1), y(-1) { }
};

/** class Interactive_Player
 *
 * This is the interactive player. this one is
 * responsible to show the correct map
 * to the player and draws the user interface,
 * cares for input and so on.
 */
class Interactive_Player : public Panel {
	public:
		Interactive_Player(Game *g, uchar pln);
		~Interactive_Player(void);

		void start();
		void exit_game_btn();
		void main_menu_btn();
		void minimap_btn();
		void toggle_buildhelp();

		void field_action();
		
		void move_view_to(int fx, int fy);
		void warp_mouse_to_field(Coords c);

		void think();

		bool handle_key(bool down, int code, char c);
		
		inline Game *get_game() { return m_game; }
		inline uchar get_player_number(void) { return m_player_number; }
		inline bool get_ignore_shadow() { return m_ignore_shadow; }
		Player *get_player();
		
		inline const Coords &get_fieldsel() const { return m_fieldsel; }
		inline bool get_fieldsel_freeze() const { return m_fieldsel_freeze; }
		void set_fieldsel(Coords c);
		void set_fieldsel_freeze(bool yes);
		
		static int get_xres();
		static int get_yres();

	private:
		void mainview_move(int x, int y);
		void minimap_warp(int x, int y);

		Game		*m_game;
		uchar		m_player_number;
		bool		m_ignore_shadow; // if true, draw unseen territory
		
		Map_View*	main_mapview;
		
		UniqueWindow	m_minimap;
		UniqueWindow	m_fieldaction;
		
		Coords	m_fieldsel; // the selected field below the mouse
		bool		m_fieldsel_freeze; // don't change m_fieldsel even if mouse moves
};


#endif // __S__INTPLAYER_H
