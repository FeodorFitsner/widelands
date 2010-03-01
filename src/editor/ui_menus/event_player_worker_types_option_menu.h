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

#ifndef EVENT_PLAYER_WORKER_TYPES_OPTION_MENU_H
#define EVENT_PLAYER_WORKER_TYPES_OPTION_MENU_H

#include "ui_basic/button.h"
#include "ui_basic/editbox.h"
#include "ui_basic/table.h"
#include "ui_basic/textarea.h"
#include "ui_basic/window.h"

#include "events/event_player_worker_types.h"

#include "i18n.h"

#include "ref_cast.h"

#include <string>
#include "container_iterate.h"

struct Editor_Interactive;
namespace Widelands {struct Tribe_Descr;}

struct Event_Player_Worker_Types_Option_Menu : public UI::Window {
	Event_Player_Worker_Types_Option_Menu
		(Editor_Interactive &, Widelands::Event_Player_Worker_Types &);

	void draw(RenderTarget &);

private:
	Editor_Interactive & eia();
	void change_player(bool up);

	Widelands::Event_Player_Worker_Types & m_event;
	Widelands::Player_Number                 m_player_number;
	struct Label_Name : public UI::Textarea {
		Label_Name(Event_Player_Worker_Types_Option_Menu & parent) :
			UI::Textarea(&parent, 5, 5, _("Name:"))
		{}
	} label_name;
	struct Name : public UI::EditBox {
		Name
			(Event_Player_Worker_Types_Option_Menu & parent,
			 std::string const                       & event_name)
			:
			UI::EditBox
				(&parent,
				 parent.label_name.get_x() + parent.label_name.get_w() + 5, 5,
				 parent.get_inner_w() - parent.label_name.get_w() - 15, 20)
		{
			setText(event_name);
		}
	} name;
	struct Label_Player : public UI::Textarea {
		Label_Player(Event_Player_Worker_Types_Option_Menu & parent) :
			UI::Textarea(&parent, 5, 30, _("Player:"))
		{}
	} label_player;
	struct Decrement_Player : public UI::Button {
		Decrement_Player
			(Event_Player_Worker_Types_Option_Menu & parent, bool enable);
		void clicked() {
			ref_cast<Event_Player_Worker_Types_Option_Menu, UI::Panel>
				(*get_parent())
			.change_player(false);
		}
	} decrement_player;
	struct Increment_Player : public UI::Button {
		Increment_Player
			(Event_Player_Worker_Types_Option_Menu & parent, bool enable);
		void clicked() {
			ref_cast<Event_Player_Worker_Types_Option_Menu, UI::Panel>
				(*get_parent())
			.change_player(true);
		}
	} increment_player;
	struct Clear_Selection : public UI::Button {
		Clear_Selection(Event_Player_Worker_Types_Option_Menu & parent) :
			UI::Button
				(&parent,
				 165, 30, 70, 20,
				 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
				 _("Clear"), _("Clear selection"))
		{}
		void clicked() {
			Event_Player_Worker_Types_Option_Menu & menu =
				ref_cast<Event_Player_Worker_Types_Option_Menu, UI::Panel>
					(*get_parent());
			for
				(wl_index_range<uint8_t> i(0,menu.table.size());
				 i;
				 ++i)
				menu.table.get_record(i.current).set_checked
					(Table::Selected, false);
		}
	} clear_selection;
	struct Invert_Selection : public UI::Button {
		Invert_Selection(Event_Player_Worker_Types_Option_Menu & parent) :
			UI::Button
				(&parent,
				 245, 30, 70, 20,
				 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
				 _("Invert"), _("Invert selection"))
		{}
		void clicked() {
			Event_Player_Worker_Types_Option_Menu & menu =
				ref_cast<Event_Player_Worker_Types_Option_Menu, UI::Panel>
					(*get_parent());
			for
				(wl_index_range<uint8_t> i(0, menu.table.size());
				 i;
				 ++i)
				menu.table.get_record(i.current).toggle(Table::Selected);
		}
	} invert_selection;
	struct Table : public UI::Table<uintptr_t const> {
		enum Cols {Selected, Icon, Name};
		Table
			(Event_Player_Worker_Types_Option_Menu & parent,
			 Widelands::Event_Player_Worker_Types  &);
		void fill
			(Widelands::Tribe_Descr                                 const &,
			 Widelands::Event_Player_Worker_Types::Worker_Types const &);
	} table;
	struct OK : public UI::Button {
		OK(Event_Player_Worker_Types_Option_Menu & parent) :
			UI::Button
				(&parent,
				 5, 335, 150, 20,
				 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
				 _("Ok"))
		{}
		void clicked();
	} ok;
	struct Cancel : public UI::Button {
		Cancel(Event_Player_Worker_Types_Option_Menu & parent) :
			UI::Button
				(&parent,
				 165, 335, 150, 20,
				 g_gr->get_picture(PicMod_UI, "pics/but1.png"),
				 _("Cancel"))
		{}
		void clicked() {
			ref_cast<Event_Player_Worker_Types_Option_Menu, UI::Panel>
				(*get_parent())
			.end_modal(0);
		}
	} cancel;
};

#endif
