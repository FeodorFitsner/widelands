/*
 * Copyright (C) 2002-2004, 2006-2007 by the Widelands Development Team
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

#ifndef OPTIONSMENU_H
#define OPTIONSMENU_H

#include "fullscreen_menu_base.h"

#include "ui_button.h"
#include "ui_checkbox.h"
#include "ui_listselect.h"
#include "ui_textarea.h"

#include <string>
#include <vector>

class Fullscreen_Menu_Options;
struct Section;

#define NUM_RESOLUTIONS 6

struct Options_Ctrl {
		struct Options_Struct {
			int xres;
			int yres;
			int depth;
			bool inputgrab;
			bool fullscreen;
			bool single_watchwin;
			bool show_warea;
			bool snap_windows_only_when_overlapping;
			bool dock_windows_to_edges;
         bool music;
         bool fx;
         std::string language;
			int autosave; // autosave interval in minutes
		};

		Options_Ctrl(Section *s);
		~Options_Ctrl();
		Options_Ctrl::Options_Struct options_struct(Section *s);
		void save_options();
	private:
		Fullscreen_Menu_Options *m_opt_dialog;
		Section *m_opt_section;
};

/**
 * Fullscreen Optionsmenu. A modal optionsmenu
 */

class Fullscreen_Menu_Options : public Fullscreen_Menu_Base {
public:
	Fullscreen_Menu_Options(Options_Ctrl::Options_Struct opt);
	Options_Ctrl::Options_Struct get_values();
   enum {
      om_cancel = 0,
      om_ok = 1
   };

private:
	UI::IDButton<Fullscreen_Menu_Options, int> m_cancel, m_apply;
	UI::Textarea                      m_title;
	UI::Checkbox                      m_fullscreen;
	UI::Textarea                      m_label_fullscreen;
	UI::Checkbox                      m_inputgrab;
	UI::Textarea                      m_label_inputgrab;
	UI::Checkbox                      m_music;
	UI::Textarea                      m_label_music;
	UI::Checkbox                      m_fx;
	UI::Textarea                      m_label_fx;
	UI::Listselect<void *>            m_reslist;
	UI::Textarea                      m_label_resolution;
	UI::Textarea                      m_label_language;
	UI::Listselect<std::string &>     m_language_list;
	UI::Textarea                      m_label_game_options;
	UI::Checkbox                      m_single_watchwin;
	UI::Textarea                      m_label_single_watchwin;
	UI::Checkbox                      m_show_workarea_preview;
	UI::Textarea                      m_label_show_workarea_preview;
	UI::Checkbox                      m_snap_windows_only_when_overlapping;
	UI::Textarea                      m_label_snap_windows_only_when_overlapping;
	UI::Checkbox                      m_dock_windows_to_edges;
	UI::Textarea                      m_label_dock_windows_to_edges;
	UI::Checkbox                      m_autosave;
	UI::Textarea                      m_label_autosave;

	struct res {
		int xres;
		int yres;
		int depth;
	};
	std::vector<res> m_resolutions;
};

#endif /* OPTIONSMENU_H */
