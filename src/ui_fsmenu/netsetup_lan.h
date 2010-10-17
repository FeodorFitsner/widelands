/*
 * Copyright (C) 2004, 2006-2009 by the Widelands Development Team
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

#ifndef FULLSCREEN_MENU_NETSETUP_LAN_H
#define FULLSCREEN_MENU_NETSETUP_LAN_H

#include "network/network_lan_promotion.h"

#include "ui_basic/button.h"
#include "ui_basic/textarea.h"
#include "ui_basic/editbox.h"
#include "ui_basic/table.h"

#include <list>
#include <string>
#include <cstring>

#include "base.h"

class Net_Open_Game;
struct Net_Game_Info;

struct Fullscreen_Menu_NetSetupLAN : public Fullscreen_Menu_Base {
	enum {
		CANCEL = 0,
		HOSTGAME,
		JOINGAME,
	};

	Fullscreen_Menu_NetSetupLAN ();

	virtual void think();

	/**
	 * \param[out] addr filled in with the IP address of the chosen server
	 * \param[out] port filled in with the port of the chosen server
	 * \return \c true if a valid server has been chosen. If \c false is
	 * returned, the values of \p addr and \p port are undefined.
	 */
	bool get_host_address (uint32_t & addr, uint16_t & port);

	/**
	 * \return the name chosen by the player
	 */
	std::string const & get_playername();

private:
	uint32_t                                    m_butx;
	uint32_t                                    m_butw;
	uint32_t                                    m_buth;
	uint32_t                                    m_lisw;
	uint32_t                                    m_fs;
	std::string                                 m_fn;
	UI::Textarea                                title, m_opengames;
	UI::Textarea                                m_playername, m_hostname;
	UI::Callback_Button                     joingame, hostgame, back, loadlasthost;
	UI::EditBox                                 playername;
	UI::EditBox                                 hostname;
	UI::Table<const Net_Open_Game * const>      opengames;
	LAN_Game_Finder                             discovery;

	void game_selected (uint32_t);
	void game_doubleclicked (uint32_t);

	static void discovery_callback (int32_t, Net_Open_Game const *, void *);

	void game_opened  (Net_Open_Game const *);
	void game_closed  (Net_Open_Game const *);
	void game_updated (Net_Open_Game const *);

	void update_game_info
		(UI::Table<const Net_Open_Game * const>::Entry_Record &,
		 const Net_Game_Info &);

	void change_hostname();
	void change_playername();
	void clicked_joingame();
	void clicked_hostgame();
	void clicked_lasthost();
};

#endif
