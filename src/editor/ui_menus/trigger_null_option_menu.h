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

#ifndef __S__TRIGGER_NULL_OPTION_MENU_H
#define __S__TRIGGER_NULL_OPTION_MENU_H

#include "ui_window.h"

class Editor_Interactive;
class Trigger_Null;
namespace UI {struct Edit_Box;};

/*
 * This is a modal box - The user must end this first
 * before it can return
 */
struct Trigger_Null_Option_Menu : public UI::Window {
      Trigger_Null_Option_Menu(Editor_Interactive*, Trigger_Null*);
      ~Trigger_Null_Option_Menu();

	bool handle_mousepress  (const Uint8 btn, int x, int y);
	bool handle_mouserelease(const Uint8 btn, int x, int y);

   private:
      void clicked(int);

      Trigger_Null* m_trigger;
      Editor_Interactive* m_parent;
      UI::Edit_Box* m_name;
};

#endif
