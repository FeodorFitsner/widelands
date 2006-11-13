/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
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

#ifndef __S__EDITOR_PLAYER_MENU_ALLOWED_BUILDINGS_MENU_H
#define __S__EDITOR_PLAYER_MENU_ALLOWED_BUILDINGS_MENU_H

#include "ui_button.h"
#include "ui_listselect.h"
#include "ui_textarea.h"
#include "ui_unique_window.h"

class Player;

/*
 * Let's the user choose which buildings should be available
 * for this player for this scenario. Used to throttle AI and
 * to advance technology slowly through the missions
 */
struct Editor_Player_Menu_Allowed_Buildings_Menu : public UI::UniqueWindow {
      Editor_Player_Menu_Allowed_Buildings_Menu(UI::Panel* parent, Player* player, UI::UniqueWindow::Registry* );
      virtual ~Editor_Player_Menu_Allowed_Buildings_Menu();

   private:
      Player* m_player;
	UI::Textarea              m_allowed_label, m_forbidden_label;
	UI::Listselect<uintptr_t> m_allowed,       m_forbidden;
	UI::IDButton<Editor_Player_Menu_Allowed_Buildings_Menu, const bool>
		m_forbid_button, m_allow_button;
	void allowed_selected        (uint);
	void forbidden_selected      (uint);
	void allowed_double_clicked  (uint);
	void forbidden_double_clicked(uint);
	void clicked                 (const bool allow);
      void update();
};

#endif
