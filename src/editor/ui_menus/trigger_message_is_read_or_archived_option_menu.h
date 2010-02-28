/*
 * Copyright (C) 2002-2004, 2006-2010 by the Widelands Development Team
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

#ifndef TRIGGER_MESSAGE_IS_READ_OR_ARCHIVED_OPTION_MENU_H
#define TRIGGER_MESSAGE_IS_READ_OR_ARCHIVED_OPTION_MENU_H

#include "ui_basic/editbox.h"
#include "ui_basic/textarea.h"
#include "ui_basic/window.h"

#include "i18n.h"

#include "ref_cast.h"

struct Editor_Interactive;
namespace Widelands {struct Trigger_Message_Is_Read_Or_Archived;}

struct Trigger_Message_Is_Read_Or_Archived_Option_Menu : public UI::Window {
	Trigger_Message_Is_Read_Or_Archived_Option_Menu
		(Editor_Interactive &, Widelands::Trigger_Message_Is_Read_Or_Archived &);

private:
	Editor_Interactive & eia();
	void absolute_time_clickedto(bool);

	Widelands::Trigger_Message_Is_Read_Or_Archived & m_trigger;
	UI::Textarea                                     label_name;
	struct Name : public UI::EditBox {
		Name
			(Trigger_Message_Is_Read_Or_Archived_Option_Menu & parent,
			 std::string                               const & trigger_name)
			:
			UI::EditBox
				(&parent,
				 parent.label_name.get_x() + parent.label_name.get_w() + 5, 5,
				 parent.get_inner_w() - parent.label_name.get_w() - 15, 20)
		{
			setText(trigger_name);
		}
	} name;
	struct OK : public UI::Button {
		OK(Trigger_Message_Is_Read_Or_Archived_Option_Menu & parent) :
			UI::Button
				(&parent,
				 5, 30, 150, 20,
				 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
				 _("Ok"))
		{}
		void clicked();
	} ok;
	struct Cancel : public UI::Button {
		Cancel(Trigger_Message_Is_Read_Or_Archived_Option_Menu & parent) :
			UI::Button
				(&parent,
				 165, 30, 150, 20,
				 g_gr->get_picture(PicMod_UI, "pics/but1.png"),
				 _("Cancel"))
		{}
		void clicked() {
			ref_cast<Trigger_Message_Is_Read_Or_Archived_Option_Menu, UI::Panel>
				(*get_parent())
			.end_modal(0);
		}
	} cancel;
};

#endif