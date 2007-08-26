/*
 * Copyright (C) 2007 by the Widelands Development Team
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

#ifndef __S__LOAD_REPLAY_H
#define __S__LOAD_REPLAY_H

#include "filesystem.h"
#include "fullscreen_menu_base.h"

#include "ui_button.h"
#include "ui_listselect.h"
#include "ui_textarea.h"

class Game;

/**
 * Select a replay from a list of replays.
 */
class Fullscreen_Menu_LoadReplay : public Fullscreen_Menu_Base {
public:
	Fullscreen_Menu_LoadReplay(Game*);
	~Fullscreen_Menu_LoadReplay();

	const std::string& filename() {return m_filename;}

	void clicked_ok();
	void replay_selected(uint);
	void double_clicked(uint);
	void fill_list();

private:
	Game* m_game;

	UI::IDButton<Fullscreen_Menu_LoadReplay, int> m_back;
	UI::Button<Fullscreen_Menu_LoadReplay> m_ok;
	UI::Listselect<std::string> m_list;
	UI::Textarea m_title;
	std::string m_filename;
};


#endif
