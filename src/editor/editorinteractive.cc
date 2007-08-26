/*
 * Copyright (C) 2002-2003, 2006-2007 by the Widelands Development Team
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

#include "editor.h"
#include "editorinteractive.h"
#include "editor_delete_immovable_tool.h"
#include "editor_event_menu.h"
#include "editor_objectives_menu.h"
#include "editor_variables_menu.h"
#include "editor_main_menu.h"
#include "editor_main_menu_load_map.h"
#include "editor_main_menu_save_map.h"
#include "editor_player_menu.h"
#include "editor_tool_menu.h"
#include "editor_toolsize_menu.h"
#include "graphic.h"
#include "i18n.h"
#include "interactive_base.h"
#include "keycodes.h"
#include "map.h"
#include "overlay_manager.h"
#include "player.h"
#include "tribe.h"
#include "ui_button.h"
#include "ui_modal_messagebox.h"
#include "wlapplication.h"


Editor_Interactive::Editor_Interactive(Editor & e) :
Interactive_Base(e), m_editor(e)
{

   // Disable debug. it is no use for editor
#ifndef DEBUG
   set_display_flag(Interactive_Base::dfDebug, false);
#else
   set_display_flag(Interactive_Base::dfDebug, true);
#endif

	fieldclicked.set(this, &Editor_Interactive::map_clicked);

   // user interface buttons
   int x = (get_w() - (7*34)) >> 1;
   int y = get_h() - 34;

	new UI::Button<Editor_Interactive>
		(this,
		 x, y, 34, 34,
		 2,
		 g_gr->get_picture(PicMod_Game, "pics/menu_toggle_menu.png"),
		 &Editor_Interactive::toggle_mainmenu, this,
		 _("Menu"));

	new UI::Button<Editor_Interactive>
		(this,
		 x + 34, y, 34, 34,
		 2,
		 g_gr->get_picture(PicMod_Game, "pics/editor_menu_toggle_tool_menu.png"),
		 &Editor_Interactive::tool_menu_btn, this,
		 _("Tool"));

	new UI::Button<Editor_Interactive>
		(this,
		 x + 68, y, 34, 34,
		 2,
		 g_gr->get_picture(PicMod_Game, "pics/editor_menu_set_toolsize_menu.png"),
		 &Editor_Interactive::toolsize_menu_btn, this,
		 _("Toolsize"));

	new UI::Button<Editor_Interactive>
		(this,
		 x + 102, y, 34, 34,
		 2,
		 g_gr->get_picture(PicMod_Game, "pics/menu_toggle_minimap.png"),
		 &Editor_Interactive::toggle_minimap, this,
		 _("Minimap"));

   new UI::Button<Editor_Interactive>
		(this,
		 x + 136, y, 34, 34,
		 2,
		 g_gr->get_picture(PicMod_Game, "pics/menu_toggle_buildhelp.png"),
		 &Editor_Interactive::toggle_buildhelp, this,
		 _("Buildhelp"));

	new UI::Button<Editor_Interactive>
		(this,
		 x + 170, y, 34, 43,
		 2,
		 g_gr->get_picture(PicMod_Game, "pics/editor_menu_player_menu.png"),
		 &Editor_Interactive::toggle_playermenu, this,
		 _("Players"));

   new UI::Button<Editor_Interactive>
		(this,
		 x + 204, y, 34, 34,
		 2,
		 g_gr->get_picture(PicMod_Game, "pics/menu_toggle_event_menu.png"),
		 &Editor_Interactive::toggle_eventmenu, this,
		 _("Events"));

   new UI::Button<Editor_Interactive>
		(this,
		 x + 238, y, 34, 34,
		 2,
		 g_gr->get_picture(PicMod_Game, "pics/menu_toggle_variables_menu.png"),
		 &Editor_Interactive::toggle_variablesmenu, this,
		 _("Variables"));

	new UI::Button<Editor_Interactive>
		(this,
		 x + 272, y, 34, 34,
		 2,
		 g_gr->get_picture(PicMod_Game, "pics/menu_toggle_objectives_menu.png"),
		 &Editor_Interactive::toggle_objectivesmenu, this,
		 _("Objectives"));

   // Load all tribes into memory
   std::vector<std::string> tribes;
	Tribe_Descr::get_all_tribenames(tribes);
   uint i=0;
   for(i=0; i<tribes.size(); i++)
		e.manually_load_tribe(tribes[i].c_str());

   m_need_save=false;
   m_ctrl_down=false;

	select_tool(tools.increase_height, Editor_Tool::First);
}

/// Restore default sel.
Editor_Interactive::~Editor_Interactive() {unset_sel_picture();}


/// Called just before the editor starts, after postload, init and gfxload.
void Editor_Interactive::start()
{egbase().map().overlay_manager().show_buildhelp(true);}


void Editor_Interactive::exit() {
	if (m_need_save) {
		UI::Modal_Message_Box mmb
			(this,
			 _("Map unsaved"),
			 _("The Map is unsaved, do you really want to quit?"),
			 UI::Modal_Message_Box::YESNO);
		if (mmb.run() == 0) return;
	}
   end_modal(0);
}

void Editor_Interactive::toggle_mainmenu() {
	if (m_mainmenu.window) delete m_mainmenu.window;
	else new Editor_Main_Menu(this, &m_mainmenu);
}


void Editor_Interactive::toggle_objectivesmenu() {
	if (m_objectivesmenu.window) delete m_objectivesmenu.window;
	else new Editor_Objectives_Menu(this, &m_objectivesmenu);
}


void Editor_Interactive::toggle_variablesmenu() {
	if (m_variablesmenu.window) delete m_variablesmenu.window;
	else new Editor_Variables_Menu(this, &m_variablesmenu);
}


void Editor_Interactive::toggle_eventmenu() {
	if (m_eventmenu.window) delete m_eventmenu.window;
	else new Editor_Event_Menu(this, &m_eventmenu);
}

void Editor_Interactive::map_clicked() {
	tools.current()
		.handle_click(tools.use_tool, egbase().map(), get_sel_pos(), *this);
	need_complete_redraw();
	set_need_save(true);
}

/// Needed to get freehand painting tools (hold down mouse and move to edit).
void Editor_Interactive::set_sel_pos(const Node_and_Triangle<> sel) {
	const bool target_changed = tools.current().operates_on_triangles() ?
		sel.triangle != get_sel_pos().triangle : sel.node != get_sel_pos().node;
	Interactive_Base::set_sel_pos(sel);
	if (target_changed and SDL_GetMouseState(0, 0) & SDL_BUTTON(SDL_BUTTON_LEFT))
		map_clicked();
}


void Editor_Interactive::toggle_buildhelp(void)
{egbase().map().overlay_manager().toggle_buildhelp();}


void Editor_Interactive::tool_menu_btn() {
	if (m_toolmenu.window) delete m_toolmenu.window;
	else new Editor_Tool_Menu(*this, m_toolmenu);
}


void Editor_Interactive::toggle_playermenu() {
	if (m_playermenu.window) delete m_playermenu.window;
	else {
		select_tool(tools.set_starting_pos, Editor_Tool::First);
		new Editor_Player_Menu(*this, &m_playermenu);
	}

}


void Editor_Interactive::toolsize_menu_btn() {
	if (m_toolsizemenu.window) delete m_toolsizemenu.window;
	else new Editor_Toolsize_Menu(this, &m_toolsizemenu);
}


bool Editor_Interactive::handle_key(bool down, int code, char) {
   if(code==KEY_LCTRL || code==KEY_RCTRL) m_ctrl_down=down;

	if (down) {
      // only on down events
		switch (code) {
			// Sel radius
         case KEY_1:
			set_sel_radius(0);
            return true;
         case KEY_2:
			set_sel_radius(1);
            return true;
         case KEY_3:
			set_sel_radius(2);
            return true;
         case KEY_4:
			set_sel_radius(3);
            return true;
         case KEY_5:
			set_sel_radius(4);
            return true;
         case KEY_6:
			set_sel_radius(5);
            return true;
         case KEY_7:
			set_sel_radius(6);
            return true;
         case KEY_8:
			set_sel_radius(7);
            return true;
         case KEY_9:
			set_sel_radius(8);
            return true;
         case KEY_0:
			set_sel_radius(9);
            return true;

         case KEY_LSHIFT:
         case KEY_RSHIFT:
			if (tools.use_tool == Editor_Tool::First)
				select_tool(tools.current(), Editor_Tool::Second);
            return true;

         case KEY_LALT:
         case KEY_RALT:
         case KEY_MODE:
			if (tools.use_tool == Editor_Tool::First)
				select_tool(tools.current(), Editor_Tool::Third);
            return true;

         case KEY_SPACE:
            toggle_buildhelp();
            return true;

         case KEY_c:
            set_display_flag(Interactive_Base::dfShowCensus,
                  !get_display_flag(Interactive_Base::dfShowCensus));
            return true;

         case KEY_e:
            toggle_eventmenu();
            return true;

         case KEY_f:
            if( down )
               g_gr->toggle_fullscreen();
            return true;

         case KEY_h:
            toggle_mainmenu();
            return true;

         case KEY_i:
			select_tool(tools.info, Editor_Tool::First);
            return true;

         case KEY_m:
            toggle_minimap();
            return true;

         case KEY_l:
            if(m_ctrl_down)
               new Main_Menu_Load_Map(this);
            return true;

         case KEY_p:
            toggle_playermenu();
            return true;

         case KEY_s:
            if(m_ctrl_down)
               new Main_Menu_Save_Map(this);
            return true;

         case KEY_t:
            tool_menu_btn();
            return true;


		}
	} else {
      // key up events
		switch (code) {
         case KEY_LSHIFT:
         case KEY_RSHIFT:
         case KEY_LALT:
         case KEY_RALT:
         case KEY_MODE:
			if (tools.use_tool != Editor_Tool::First)
				select_tool(tools.current(), Editor_Tool::First);
            return true;
		}
	}
   return false;
}


void Editor_Interactive::select_tool
(Editor_Tool & primary, const Editor_Tool::Tool_Index which)
{
	if (which == Editor_Tool::First and &primary != tools.current_pointer) {
		Map & map = egbase().map();
      // A new tool has been selected. Remove all
      // registered overlay callback functions
		map.overlay_manager().register_overlay_callback_function(0, 0);
		map.recalc_whole_map();

	}
	tools.current_pointer = &primary;
   tools.use_tool=which;

	if (const char * sel_pic = primary.get_sel(which)) set_sel_picture(sel_pic);
	else                                             unset_sel_picture();
	set_sel_triangles(primary.operates_on_triangles());
}

/*
 * Reference functions
 *
 *  data is either a pointer to a trigger, event
 *  or a tribe (for buildings)
 */
void Editor_Interactive::reference_player_tribe
(const Player_Number player, const void * const data)
{
	assert(0 < player);
	assert    (player <= m_editor.map().get_nrplayers());

   Player_References r;
   r.player=player;
   r.object=data;

   m_player_tribe_references.push_back(r);
}

/*
 * unreference !once!, if referenced many times, this
 * will leace a reference
 */
void Editor_Interactive::unreference_player_tribe
(const Player_Number player, const void * const data)
{
	assert(player <= m_editor.map().get_nrplayers());
   assert(data);

   int i=0;
   if(player>0) {
      for(i=0; i<static_cast<int>(m_player_tribe_references.size()); i++)
         if(m_player_tribe_references[i].player==player && m_player_tribe_references[i].object==data) break;

      m_player_tribe_references.erase(m_player_tribe_references.begin() + i);
	} else {
      // Player is invalid, remove all references from this object
      for(i=0; i<static_cast<int>(m_player_tribe_references.size()); i++) {
         if(m_player_tribe_references[i].object==data) {
            m_player_tribe_references.erase(m_player_tribe_references.begin() + i); i=-1;
			}
		}
	}
}

bool Editor_Interactive::is_player_tribe_referenced(int player) {
	assert(0 < player);
	assert    (player <= m_editor.map().get_nrplayers());

   uint i=0;
   for(i=0; i<m_player_tribe_references.size(); i++)
         if(m_player_tribe_references[i].player==player) return true;

   return false;
}
