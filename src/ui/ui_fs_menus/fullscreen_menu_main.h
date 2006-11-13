/*
 * Copyright (C) 2002, 2006 by the Widelands Development Team
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

#ifndef __S__MAINMENUE_H
#define __S__MAINMENUE_H

#include "fullscreen_menu_base.h"
#include "ui_button.h"
#include "ui_textarea.h"

/**
 * This runs the main menu. There, you can select
 * between different playmodes, exit and so on.
*/
class Fullscreen_Menu_Main : public Fullscreen_Menu_Base {
	UI::IDButton<Fullscreen_Menu_Main, int> singleplayer;
	UI::IDButton<Fullscreen_Menu_Main, int> multiplayer;
	UI::IDButton<Fullscreen_Menu_Main, int> options;
	UI::IDButton<Fullscreen_Menu_Main, int> editor;
	UI::IDButton<Fullscreen_Menu_Main, int> readme;
	UI::IDButton<Fullscreen_Menu_Main, int> license;
	UI::IDButton<Fullscreen_Menu_Main, int> exit;
	UI::Textarea                            version;
	UI::Textarea                            copyright;
public:
	Fullscreen_Menu_Main();
   enum {
      mm_singleplayer,
      mm_multiplayer,
      mm_options,
      mm_editor,
      mm_readme,
      mm_license,
      mm_exit
   };
};

#endif /* __S__MAINMENUE_H */
