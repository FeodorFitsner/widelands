/*
 * Copyright (C) 2008 by the Widelands Development Team
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

#ifndef WL_UI_FSMENU_EDITOR_H
#define WL_UI_FSMENU_EDITOR_H

#include "ui_fsmenu/main_menu.h"
#include "ui_basic/button.h"
#include "ui_basic/textarea.h"

/**
 * Fullscreen Menu for Editor.
 * Here you select what game you want to play.
 */
class FullscreenMenuEditor : public FullscreenMenuMainMenu {
public:
	FullscreenMenuEditor();

	enum class MenuTarget: int32_t {
		kBack = UI::Panel::dying_code,
		kNewMap,
		kLoadMap
	};

private:
	UI::Textarea title;
	UI::Button   new_map;
	UI::Button   load_map;
	UI::Button   back;
};

#endif  // end of include guard: WL_UI_FSMENU_EDITOR_H
