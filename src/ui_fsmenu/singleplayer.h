/*
 * Copyright (C) 2002, 2006, 2008 by the Widelands Development Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef WL_UI_FSMENU_SINGLEPLAYER_H
#define WL_UI_FSMENU_SINGLEPLAYER_H

#include "ui_fsmenu/main_menu.h"
#include "ui_basic/box.h"
#include "ui_basic/button.h"
#include "ui_basic/textarea.h"

/**
 * Fullscreen Menu for SinglePlayer.
 * Here you select what game you want to play.
 */
class FullscreenMenuSinglePlayer : public FullscreenMenuMainMenu {
public:
	FullscreenMenuSinglePlayer();

protected:
	void clicked_ok() override;

private:
	UI::Textarea title;
	UI::Box      vbox;
	UI::Button   new_game;
	UI::Button   campaign;
	UI::Button   load_game;
	UI::Button   back;
};

#endif  // end of include guard: WL_UI_FSMENU_SINGLEPLAYER_H
