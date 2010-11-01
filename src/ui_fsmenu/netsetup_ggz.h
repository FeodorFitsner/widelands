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

#ifndef FULLSCREEN_MENU_NETSETUP_GGZ_H
#define FULLSCREEN_MENU_NETSETUP_GGZ_H

#include "network/network_ggz.h"

#include "network/network_lan_promotion.h"

#include "base.h"
#include "ui_basic/button.h"
#include "ui_basic/editbox.h"
#include "ui_basic/listselect.h"
#include "ui_basic/spinbox.h"
#include "ui_basic/table.h"
#include "ui_basic/textarea.h"
#include "wui/gamechatpanel.h"

#include <string>
#include <cstring>
#include <vector>

class Net_Open_Game;
struct Net_Game_info;

struct Fullscreen_Menu_NetSetupGGZ : public Fullscreen_Menu_Base {
	enum {
		CANCEL = 0,
		HOSTGAME,
		JOINGAME
	};

	Fullscreen_Menu_NetSetupGGZ (const char *, const char *, bool);

	virtual void think();

	/// \returns the maximum number of players that may connect
	int32_t get_maxplayers() {
		return maxplayers.getValue();
	}

private:
	uint32_t                                    m_butx;
	uint32_t                                    m_butw;
	uint32_t                                    m_buth;
	uint32_t                                    m_lisw;
	int32_t                                     m_namechange;
	uint32_t                                    m_fs;
	std::string                                 m_fn;
	UI::Textarea                                title, m_users, m_opengames;
	UI::Textarea                                m_servername;
	UI::Textarea                                m_maxplayers;
	UI::SpinBox                                 maxplayers;
	UI::Callback_Button                     joingame, hostgame, back;
	UI::EditBox                                 servername;
	UI::Table<const Net_Player * const>         usersonline;
	UI::Listselect<Net_Open_Game>               opengames;
	GameChatPanel                               chat;

	// Login information
	const char * nickname;
	const char * password;
	bool         reg;
	bool         tried_login;

	void fillServersList(std::vector<Net_Game_Info> const &);
	void fillUserList   (std::vector<Net_Player> const &);

	void connectToMetaserver();

	void user_doubleclicked (uint32_t);
	void server_selected (uint32_t);
	void server_doubleclicked (uint32_t);

	void change_servername();
	void clicked_joingame();
	void clicked_hostgame();
	void clicked_lasthost();

	bool compare_usertype(unsigned int rowa, unsigned int rowb);
};

#endif
