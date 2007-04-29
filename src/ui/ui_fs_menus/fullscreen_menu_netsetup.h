/*
 * Copyright (C) 2004, 2006-2007 by the Widelands Development Team
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

#ifndef __S__NETSETUP_H
#define __S__NETSETUP_H

#include "network_lan_promotion.h"

#include "ui_button.h"
#include "ui_textarea.h"
#include "ui_editbox.h"
#include "ui_table.h"

#include <list>
#include <string>

#include "fullscreen_menu_base.h"

class LAN_Open_Game;
struct LAN_Game_Info;

struct Fullscreen_Menu_NetSetup : public Fullscreen_Menu_Base {
		enum {
			CANCEL=0,
			HOSTGAME,
			JOINGAME,
			INTERNETGAME,
			HOSTGGZGAME,
			JOINGGZGAME
		};

		Fullscreen_Menu_NetSetup ();

		virtual void think();

		bool get_host_address (ulong&, ushort&);
		// return true if the selected or entered hostname is valid

		//bool is_internetgame();
		// return true if game should be played over GGZ

		void fill(std::list<std::string> tables);

private:
	UI::Textarea                                title;
	UI::Button<Fullscreen_Menu_NetSetup>        joingame;
	UI::Button<Fullscreen_Menu_NetSetup>        hostgame;
	UI::IDButton<Fullscreen_Menu_NetSetup, int> playinternet;
	UI::IDButton<Fullscreen_Menu_NetSetup, int> back;
	UI::Edit_Box                                hostname;
	UI::Edit_Box                                playername;
	UI::Table<const LAN_Open_Game * const>      opengames;
	LAN_Game_Finder                             discovery;
	UI::Button<Fullscreen_Menu_NetSetup>        networktype;
	bool                                        internetgame;

	void game_selected (uint);

		static void discovery_callback (int, const LAN_Open_Game*, void*);

		void game_opened (const LAN_Open_Game*);
		void game_closed (const LAN_Open_Game*);
		void game_updated (const LAN_Open_Game*);

	void update_game_info
		(UI::Table<const LAN_Open_Game * const>::Entry_Record &,
		 const LAN_Game_Info &);

	void toggle_networktype();
		void toggle_hostname();
	void clicked_joingame();
	void clicked_hostgame();
};

#endif
