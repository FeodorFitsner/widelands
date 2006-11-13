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

#ifndef __S__GAME_OPTIONS_MENU_H
#define __S__GAME_OPTIONS_MENU_H

#include "interactive_player.h"
#include "ui_checkbox.h"
#include "ui_button.h"
#include "ui_textarea.h"
#include "ui_unique_window.h"

// The GameOptionsMenu is a rather dumb window with lots of buttons
class GameOptionsMenu : public UI::UniqueWindow {
public:
	GameOptionsMenu
		(Interactive_Player &,
		 UI::UniqueWindow::Registry &,
		 Interactive_Player::Game_Main_Menu_Windows &);

private:
	Interactive_Player	                      & m_player;
	Interactive_Player::Game_Main_Menu_Windows & m_windows;
	UI::Button<GameOptionsMenu> readme;
	UI::Button<GameOptionsMenu> license;
	UI::Button<GameOptionsMenu> authors;
	UI::Checkbox ingame_music;
	UI::Textarea ingame_music_label;
	UI::Checkbox ingame_sound;
	UI::Textarea ingame_sound_label;
	UI::Button<GameOptionsMenu> save_game;
	UI::Button<GameOptionsMenu> load_game;
	UI::Button<GameOptionsMenu> exit_game;

	/** Returns the horizontal/vertical spacing between buttons. */
	uint hspacing() const {return 5;};
	uint vspacing() const {return 5;};

	/** Returns the horizontal/vertical margin between edge and buttons. */
	uint hmargin() const {return 2 * hspacing();}
	uint vmargin() const {return 2 * vspacing();}

	/** Returns the width of a button in a row with nr_buttons buttons. */
	uint buttonw(const uint nr_buttons) const {
		return static_cast<const uint>
			((get_inner_w() * 1.45 - (nr_buttons + 3) * hspacing()) / nr_buttons);
	}

	/**
	 * Returns the x coordinate of the (left edge of) button number nr in a row
	 * with nr_buttons buttons.
	 */
	uint posx(const uint nr, const uint nr_buttons) const
	{return hmargin() + nr * (buttonw(nr_buttons) + hspacing());}

	void changed_ingame_music(bool);
	void changed_ingame_sound(bool);
	void clicked_readme   ();
	void clicked_license  ();
	void clicked_authors  ();
	void clicked_save_game();
	void clicked_load_game();
	void clicked_exit_game();
};

#endif
