/*
 * Copyright (C) 2002-4 by the Widelands Development Team
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

#ifndef __S__TRIGGER_TIME_OPTION_MENU_H
#define __S__TRIGGER_TIME_OPTION_MENU_H

#include <string>
#include "ui_window.h"

class Editor_Interactive;
class UIButton;
class Trigger_Time;
class UITextarea;
class UIEdit_Box;

/*
 * This is a modal box - The user must end this first
 * before it can return
 */
class Trigger_Time_Option_Menu : public UIWindow {
   public:
      Trigger_Time_Option_Menu(Editor_Interactive*, Trigger_Time*);
      ~Trigger_Time_Option_Menu();

      bool handle_mouseclick(uint btn, bool down, int mx, int my);

   private:
      void clicked(int);
      void update(void);

      Trigger_Time* m_trigger;
      Editor_Interactive* m_parent;
      UIEdit_Box* m_name;
      UITextarea* m_textareas[6];
      int         m_values[6];
};

#endif
