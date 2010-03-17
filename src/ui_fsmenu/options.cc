/*
 * Copyright (C) 2002-2004, 2006-2009 by Widelands Development Team
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

#define DEFINE_LANGUAGES  // So that the language array gets defined

#include "options.h"

#include "constants.h"
#include "io/filesystem/layered_filesystem.h"
#include "graphic/graphic.h"
#include "i18n.h"
#include "languages.h"
#include "profile/profile.h"
#include "save_handler.h"
#include "sound/sound_handler.h"
#include "wlapplication.h"

#include <libintl.h>

#include <cstdio>
#include <iostream>

Fullscreen_Menu_Options::Fullscreen_Menu_Options
		(Options_Ctrl::Options_Struct opt)
	:
	Fullscreen_Menu_Base("optionsmenu.jpg"),

// Values for alignment and size
	m_vbutw(m_yres * 333 / 10000),
	m_butw (m_xres / 4),
	m_buth (m_yres * 9 / 200),
	m_fs   (fs_small()),
	m_fn   (ui_fn()),

// Buttons
	m_advanced_options
		(this,
		 m_xres * 9 / 80, m_yres * 19 / 20, m_butw, m_buth,
		 g_gr->get_picture(PicMod_UI, "pics/but2.png"),
		 &Fullscreen_Menu_Options::advanced_options, *this,
		 _("Advanced Options"), std::string(), true, false,
		 m_fn, m_fs),
	m_cancel
		(this,
		 m_xres * 51 / 80, m_yres * 19 / 20, m_butw, m_buth,
		 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
		 &Fullscreen_Menu_Options::end_modal, *this, om_cancel,
		 _("Cancel"), std::string(), true, false,
		 m_fn, m_fs),
	m_apply
		(this,
		 m_xres * 3 / 8, m_yres * 19 / 20, m_butw, m_buth,
		 g_gr->get_picture(PicMod_UI, "pics/but2.png"),
		 &Fullscreen_Menu_Options::end_modal, *this, om_ok,
		 _("Apply"), std::string(), true, false,
		 m_fn, m_fs),

// Spinboxes
	m_sb_maxfps
		(this,
		 m_xres / 2, m_yres * 3833 / 10000, m_xres / 5, m_vbutw,
		 opt.maxfps, 0, 100, "",
		 g_gr->get_picture(PicMod_UI, "pics/but1.png")),
	m_sb_autosave
		(this,
		 m_xres * 6767 / 10000, m_yres * 8167 / 10000, m_xres / 4, m_vbutw,
		 opt.autosave / 60, 0, 100, _("min."),
		 g_gr->get_picture(PicMod_UI, "pics/but1.png"), true),

	m_sb_remove_replays
		(this,
		 m_xres * 6767 / 10000, m_yres * 8631 / 10000, m_xres / 4, m_vbutw,
		 opt.remove_replays, 0, 365, _("days"),
		 g_gr->get_picture(PicMod_UI, "pics/but1.png"), true),

// Title
	m_title
		(this,
		 m_xres / 2, m_yres / 40,
		 _("General Options"), UI::Align_HCenter),

// First options block 'general options'
	m_fullscreen (this, Point(m_xres * 3563 / 10000, m_yres * 1667 / 10000)),
	m_label_fullscreen
		(this,
		 m_xres * 1969 / 5000, m_yres * 1833 / 10000,
		 _("Fullscreen"), UI::Align_VCenter),

	m_inputgrab (this, Point(m_xres * 3563 / 10000, m_yres * 2167 / 10000)),
	m_label_inputgrab
		(this,
		 m_xres * 1969 / 5000, m_yres * 2333 / 10000,
		 _("Grab Input"), UI::Align_VCenter),

	m_music (this, Point(m_xres * 3563 / 10000, m_yres * 2667 / 10000)),
	m_label_music
		(this,
		 m_xres * 1969 / 5000, m_yres * 2833 / 10000,
		 _("Enable Music"), UI::Align_VCenter),

	m_fx (this, Point(m_xres * 3563 / 10000, m_yres * 3167 / 10000)),
	m_label_fx
		(this,
		 m_xres * 1969 / 5000, m_yres * 3333 / 10000,
		 _("Enable Sound"), UI::Align_VCenter),

	m_label_maxfps
		(this,
		 m_xres * 3563 / 10000, m_yres * 2 / 5,
		 _("Maximum FPS:"), UI::Align_VCenter),

	m_reslist
		(this,
		 m_xres      / 10, m_yres * 1667 / 10000,
		 m_xres * 19 / 80, m_yres * 2833 / 10000,
		 UI::Align_Left, true),
	m_label_resolution
		(this,
		 m_xres * 1063 / 10000, m_yres * 1417 / 10000,
		 _("In-game resolution"), UI::Align_VCenter),

	m_label_language
		(this,
		 m_xres * 133 / 200, m_yres * 1417 / 10000,
		 _("Language"), UI::Align_VCenter),
	m_language_list
		(this,
		 m_xres * 6563 / 10000, m_yres * 1667 / 10000,
		 m_xres *   21 /    80, m_yres * 2833 / 10000,
		 UI::Align_Left, true),

// Title 2
	m_label_game_options
		(this,
		 m_xres / 2, m_yres / 2,
		 _("In-game Options"), UI::Align_HCenter),

// Second options block 'In-game options'
	m_single_watchwin (this, Point(m_xres * 19 / 200, m_yres * 5833 / 10000)),
	m_label_single_watchwin
		(this,
		 m_xres * 1313 / 10000, m_yres * 3 / 5,
		 _("Use single Watchwindow Mode"), UI::Align_VCenter),

	m_auto_roadbuild_mode (this, Point(m_xres * 19 / 200, m_yres * 63 / 100)),
	m_label_auto_roadbuild_mode
		(this,
		 m_xres * 1313 / 10000, m_yres * 6467 / 10000,
		 _("Start roadbuilding after placing flag"), UI::Align_VCenter),

	m_show_workarea_preview
		(this, Point(m_xres * 19 / 200, m_yres * 6767 / 10000)),
	m_label_show_workarea_preview
		(this,
		 m_xres * 1313 / 10000, m_yres * 6933 / 10000,
		 _("Show buildings area preview"), UI::Align_VCenter),

	m_snap_windows_only_when_overlapping
		(this, Point(m_xres * 19 / 200, m_yres * 7233 / 10000)),
	m_label_snap_windows_only_when_overlapping
		(this,
		 m_xres * 1313 / 10000, m_yres * 37 / 50,
		 _("Snap windows only when overlapping"), UI::Align_VCenter),

	m_dock_windows_to_edges (this, Point(m_xres * 19 / 200, m_yres * 77 / 100)),
	m_label_dock_windows_to_edges
		(this,
		 m_xres * 1313 / 10000, m_yres * 7867 / 10000,
		 _("Dock windows to edges"), UI::Align_VCenter),

	m_label_autosave
		(this,
		 m_xres * 1313 / 10000, m_yres * 8333 / 10000,
		 _("Save game automatically every"), UI::Align_VCenter),
	m_label_remove_replays
		(this,
		 m_xres * 1313 / 10000, m_yres * 8799 / 10000,
		 _("Remove Replays older than:"), UI::Align_VCenter),

	os(opt)
{
	m_sb_autosave     .add_replacement(0, _("Off"));

	m_sb_maxfps       .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_sb_autosave     .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_sb_remove_replays.set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_title           .set_font(m_fn, fs_big(), UI_FONT_CLR_FG);
	m_label_fullscreen.set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_fullscreen      .set_state(opt.fullscreen);
	m_label_inputgrab .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_inputgrab       .set_state(opt.inputgrab);
	m_label_music     .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_music           .set_state(opt.music);
	m_music           .set_enabled(not g_sound_handler.m_lock_audio_disabling);
	m_label_fx        .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_fx              .set_state(opt.fx);
	m_fx              .set_enabled(not g_sound_handler.m_lock_audio_disabling);
	m_label_maxfps    .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_label_resolution.set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_reslist         .set_font(m_fn, m_fs);
	m_label_language  .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_language_list   .set_font(m_fn, m_fs);

	m_label_game_options             .set_font(m_fn, fs_big(), UI_FONT_CLR_FG);
	m_label_single_watchwin          .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_single_watchwin                .set_state(opt.single_watchwin);
	m_label_auto_roadbuild_mode      .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_auto_roadbuild_mode            .set_state(opt.auto_roadbuild_mode);
	m_label_show_workarea_preview    .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_show_workarea_preview          .set_state(opt.show_warea);
	m_label_snap_windows_only_when_overlapping.set_font
		(m_fn, m_fs, UI_FONT_CLR_FG);
	m_snap_windows_only_when_overlapping.set_state
		(opt.snap_windows_only_when_overlapping);
	m_label_dock_windows_to_edges    .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_dock_windows_to_edges          .set_state(opt.dock_windows_to_edges);
	m_label_autosave                 .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_label_remove_replays           .set_font(m_fn, m_fs, UI_FONT_CLR_FG);

	//  GRAPHIC_TODO: this shouldn't be here List all resolutions
	SDL_PixelFormat & fmt = *SDL_GetVideoInfo()->vfmt;
	fmt.BitsPerPixel = 16;
	if
		(SDL_Rect const * const * const modes =
		 	SDL_ListModes(&fmt, SDL_SWSURFACE | SDL_FULLSCREEN))
		for (uint32_t i = 0; modes[i]; ++i)
			if (640 <= modes[i]->w and 480 <= modes[i]->h) {
				res const this_res = {modes[i]->w, modes[i]->h, 16};
			if
				(m_resolutions.empty()
				 or
				 this_res.xres != m_resolutions.rbegin()->xres
				 or
				 this_res.yres != m_resolutions.rbegin()->yres)
				m_resolutions.push_back(this_res);
			}
	fmt.BitsPerPixel = 32;
	if
		(SDL_Rect const * const * const modes =
		 	SDL_ListModes(&fmt, SDL_SWSURFACE | SDL_FULLSCREEN))
		for (uint32_t i = 0; modes[i]; ++i)
			if (640 <= modes[i]->w and 480 <= modes[i]->h) {
				res const this_res = {modes[i]->w, modes[i]->h, 32};
				if
					(m_resolutions.empty()
					 or
					 this_res.xres != m_resolutions.rbegin()->xres
					 or
					 this_res.yres != m_resolutions.rbegin()->yres)
					m_resolutions.push_back(this_res);
			}

	bool did_select_a_res = false;
	for (uint32_t i = 0; i < m_resolutions.size(); ++i) {
		char buf[32];
		sprintf
			(buf, "%ix%i %i bit", m_resolutions[i].xres,
			 m_resolutions[i].yres, m_resolutions[i].depth);
		const bool selected =
			m_resolutions[i].xres  == opt.xres and
			m_resolutions[i].yres  == opt.yres and
			m_resolutions[i].depth == opt.depth;
		did_select_a_res |= selected;
		m_reslist.add(buf, 0, g_gr->get_no_picture(), selected);
	}
	if (not did_select_a_res)
		m_reslist.select(m_reslist.size() - 1);

	available_languages[0].name = _("System default language");
	for (uint32_t i = 0; i < NR_LANGUAGES; ++i)
		m_language_list.add
			(available_languages[i].name.c_str(),
			 available_languages[i].abbrev,
			 g_gr->get_no_picture(),
			 available_languages[i].abbrev == opt.language);
}

void Fullscreen_Menu_Options::advanced_options() {
	Fullscreen_Menu_Advanced_Options aom(os);
	if (aom.run() == Fullscreen_Menu_Advanced_Options::om_ok) {
		os = aom.get_values();
		end_modal(om_restart);
	}
}


Options_Ctrl::Options_Struct Fullscreen_Menu_Options::get_values() {
	const uint32_t res_index = m_reslist.selection_index();

	// Write all data from UI elements
	os.xres                  = m_resolutions[res_index].xres;
	os.yres                  = m_resolutions[res_index].yres;
	os.depth                 = m_resolutions[res_index].depth;
	os.inputgrab             = m_inputgrab.get_state();
	os.fullscreen            = m_fullscreen.get_state();
	os.single_watchwin       = m_single_watchwin.get_state();
	os.auto_roadbuild_mode   = m_auto_roadbuild_mode.get_state();
	os.show_warea            = m_show_workarea_preview.get_state();
	os.snap_windows_only_when_overlapping
		= m_snap_windows_only_when_overlapping.get_state();
	os.dock_windows_to_edges = m_dock_windows_to_edges.get_state();
	os.music                 = m_music.get_state();
	os.fx                    = m_fx.get_state();
	if (m_language_list.has_selection())
		os.language      = m_language_list.get_selected();
	os.autosave              = m_sb_autosave.getValue();
	os.maxfps                = m_sb_maxfps.getValue();
	os.remove_replays        = m_sb_remove_replays.getValue();

	return os;
}


/**
 * The advanced option menu
 */
Fullscreen_Menu_Advanced_Options::Fullscreen_Menu_Advanced_Options
	(Options_Ctrl::Options_Struct const opt)
	:
	Fullscreen_Menu_Base("optionsmenu.jpg"),

// Values for alignment and size
	m_vbutw (m_yres * 333 / 10000),
	m_butw  (m_xres / 4),
	m_buth  (m_yres * 9 / 200),
	m_fs    (fs_small()),
	m_fn    (ui_fn()),

// Buttons
	m_cancel
		(this,
		 m_xres * 41 / 80, m_yres * 19 / 20, m_butw, m_buth,
		 g_gr->get_picture(PicMod_UI, "pics/but0.png"),
		 &Fullscreen_Menu_Advanced_Options::end_modal, *this, om_cancel,
		 _("Cancel"), std::string(), true, false,
		 m_fn, m_fs),
	m_apply
		(this,
		 m_xres / 4,   m_yres * 19 / 20, m_butw, m_buth,
		 g_gr->get_picture(PicMod_UI, "pics/but2.png"),
		 &Fullscreen_Menu_Advanced_Options::end_modal, *this, om_ok,
		 _("Apply"), std::string(), true, false,
		 m_fn, m_fs),

// Spinboxes
	m_sb_speed
		(this,
		 m_xres * 18 / 25, m_yres * 63 / 100, m_xres / 4, m_vbutw,
		 opt.speed_of_new_game / 1000, 0, 100, _("x"),
		 g_gr->get_picture(PicMod_UI, "pics/but1.png")),
	m_sb_dis_panel
		(this,
		 m_xres * 18 / 25, m_yres * 6768 / 10000, m_xres / 4, m_vbutw,
		 opt.panel_snap_distance, 0, 100, _("px."),
		 g_gr->get_picture(PicMod_UI, "pics/but1.png")),
	m_sb_dis_border
		(this,
		 m_xres * 18 / 25, m_yres * 7235 / 10000, m_xres / 4, m_vbutw,
		 opt.border_snap_distance, 0, 100, _("px."),
		 g_gr->get_picture(PicMod_UI, "pics/but1.png")),


// Title
	m_title
		(this,
		 m_xres / 2, m_yres / 40,
		 _("Advanced Options"), UI::Align_HCenter),

// First options block
	m_ui_font_list
		(this,
		 m_xres / 10, m_yres * 1667 / 10000,
		 m_xres /  4, m_yres * 2833 / 10000,
		 UI::Align_Left, true),
	m_label_ui_font
		(this,
		 m_xres * 1063 / 10000, m_yres * 1417 / 10000,
		 _("Main menu font:"), UI::Align_VCenter),
	m_message_sound
		(this, Point(m_xres * 29 / 80, m_yres * 171 / 1000)),
	m_label_message_sound
		(this,
		 m_xres * 4 / 10, m_yres * 1883 / 10000,
		 _("Play a sound at message arrival."),
		 UI::Align_VCenter),

// Second options block
	m_nozip (this, Point(m_xres * 19 / 200, m_yres * 5833 / 10000)),
	m_label_nozip
		(this,
		 m_xres * 1313 / 10000, m_yres * 3 / 5,
		 _("Do not zip widelands data files (maps, replays and savegames)."),
		 UI::Align_VCenter),
	m_label_speed
		(this,
		 m_xres * 1313 / 10000, m_yres * 6467 / 10000,
		 _("Speed of a new game:"), UI::Align_VCenter),
	m_label_snap_dis_panel
		(this,
		 m_xres * 1313 / 10000, m_yres * 6933 / 10000,
		 _("Distance for windows to snap to other panels:"), UI::Align_VCenter),
	m_label_snap_dis_border
		(this,
		 m_xres * 1313 / 10000, m_yres * 37 / 50,
		 _("Distance for windows to snap to borders:"), UI::Align_VCenter),
	m_hw_improvements (this, Point(m_xres * 19 / 200, m_yres * 7715 / 10000)),
	m_label_hw_improvements
		(this,
		 m_xres * 1313 / 10000, m_yres * 7865 / 10000,
		 _("Graphics experimental improvements."),
		 UI::Align_VCenter),
	m_double_buffer (this, Point(m_xres * 19 / 200, m_yres * 8181 / 10000)),
	m_label_double_buffer
		(this,
		 m_xres * 1313 / 10000, m_yres * 8331 / 10000,
		 _("Graphics double buffering."),
		 UI::Align_VCenter),

	m_remove_syncstreams (this, Point(m_xres * 19 / 200, m_yres * 8649 / 10000)),
	m_label_remove_syncstreams
		(this,
		 m_xres * 1313 / 10000, m_yres * 8799 / 10000,
		 _("Remove Syncstream dumps on startup"), UI::Align_VCenter),

	os(opt)
{
	m_title                .set_font(m_fn, fs_big(), UI_FONT_CLR_FG);
	m_label_message_sound  .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_message_sound        .set_state(opt.message_sound);
	m_label_nozip          .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_nozip                .set_state(opt.nozip);
	m_hw_improvements      .set_state(opt.hw_improvements);
	m_double_buffer        .set_state(opt.double_buffer);
	m_label_speed          .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_label_snap_dis_border.set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_label_snap_dis_panel .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_label_hw_improvements.set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_label_double_buffer  .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_label_remove_syncstreams.set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_remove_syncstreams   .set_state(opt.remove_syncstreams);
	m_sb_speed             .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_sb_dis_border        .set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_sb_dis_panel         .set_font(m_fn, m_fs, UI_FONT_CLR_FG);

	m_sb_speed.add_replacement(0, _("Pause"));

	m_label_ui_font.set_font(m_fn, m_fs, UI_FONT_CLR_FG);
	m_ui_font_list .set_font(m_fn, m_fs);

	// Fill the font list.
	{ // For use of string ui_font take a look at fullscreen_menu_base.cc
		bool did_select_a_font = false;
		bool cmpbool = !strcmp("serif", opt.ui_font.c_str());
		did_select_a_font = cmpbool;
		m_ui_font_list.add
			(_("FreeSerif (Default)"), "serif", g_gr->get_no_picture(), cmpbool);
		cmpbool = !strcmp("sans", opt.ui_font.c_str());
		did_select_a_font |= cmpbool;
		m_ui_font_list.add
			("FreeSans", "sans", g_gr->get_no_picture(), cmpbool);
		cmpbool = !strcmp(UI_FONT_NAME_WIDELANDS, opt.ui_font.c_str());
		did_select_a_font |= cmpbool;
		m_ui_font_list.add
			("Widelands", UI_FONT_NAME_WIDELANDS, g_gr->get_no_picture(), cmpbool);

		// Fill with all left *.ttf files we find in fonts
		filenameset_t files;
		g_fs->FindFiles("fonts/", "*.ttf", &files);

		for
			(filenameset_t::iterator pname = files.begin();
			 pname != files.end();
			 ++pname)
		{
			char const * const path = pname->c_str();
			char const * const name = FileSystem::FS_Filename(path);
			if (!strcmp(name, UI_FONT_NAME_SERIF))
				continue;
			if (!strcmp(name, UI_FONT_NAME_SANS))
				continue;
			if (g_fs->IsDirectory(name))
				continue;
			cmpbool = !strcmp(name, opt.ui_font.c_str());
			did_select_a_font |= cmpbool;
			m_ui_font_list.add
				(name, name, g_gr->get_no_picture(), cmpbool);
		}

		if (!did_select_a_font)
			m_ui_font_list.select(0);
	}
}

Options_Ctrl::Options_Struct Fullscreen_Menu_Advanced_Options::get_values() {
	// Write all remaining data from UI elements
	os.message_sound        = m_message_sound.get_state();
	os.nozip                = m_nozip.get_state();
	os.hw_improvements      = m_hw_improvements.get_state();
	os.double_buffer        = m_double_buffer.get_state();
	os.ui_font              = m_ui_font_list.get_selected();
	os.speed_of_new_game    = m_sb_speed.getValue() * 1000;
	os.panel_snap_distance  = m_sb_dis_panel.getValue();
	os.border_snap_distance = m_sb_dis_border.getValue();
	os.remove_syncstreams   = m_remove_syncstreams.get_state();
	return os;
}


/**
 * Handles communication between window class and options
 */
Options_Ctrl::Options_Ctrl(Section & s)
: m_opt_section(s), m_opt_dialog(new Fullscreen_Menu_Options(options_struct()))
{
	handle_menu();
}

Options_Ctrl::~Options_Ctrl() {
	delete m_opt_dialog;
}

void Options_Ctrl::handle_menu()
{
	int32_t i = m_opt_dialog->run();
	if (i != Fullscreen_Menu_Options::om_cancel)
		save_options();
	if (i == Fullscreen_Menu_Options::om_restart) {
		delete m_opt_dialog;
		m_opt_dialog = new Fullscreen_Menu_Options(options_struct());
		handle_menu(); // Restart general options menu
	}
}

Options_Ctrl::Options_Struct Options_Ctrl::options_struct() {
	Options_Struct opt;
	opt.xres                = m_opt_section.get_int
		("xres",                 640);
	opt.yres                = m_opt_section.get_int
		("yres",                 480);
	opt.depth               = m_opt_section.get_int
		("depth",                 16);
	opt.inputgrab           = m_opt_section.get_bool
		("inputgrab",          false);
	opt.fullscreen          = m_opt_section.get_bool
		("fullscreen",         false);
	opt.single_watchwin     = m_opt_section.get_bool
		("single_watchwin",    false);
	opt.auto_roadbuild_mode = m_opt_section.get_bool
		("auto_roadbuild_mode", true);
	opt.show_warea          = m_opt_section.get_bool
		("workareapreview",    false);
	opt.snap_windows_only_when_overlapping
		= m_opt_section.get_bool
			("snap_windows_only_when_overlapping",      false);
	opt.dock_windows_to_edges
		= m_opt_section.get_bool
			("dock_windows_to_edges",                   false);
	opt.language              =  m_opt_section.get_string
		("language",         "");
	opt.music                 = !m_opt_section.get_bool
		("disable_music",   false);
	opt.fx                    = !m_opt_section.get_bool
		("disable_fx",      false);
	opt.autosave
		= m_opt_section.get_int
			("autosave",        DEFAULT_AUTOSAVE_INTERVAL * 60);
	opt.maxfps                =  m_opt_section.get_int
		("maxfps",              25);

	opt.message_sound         =  m_opt_section.get_bool
		("sound_at_message", true);
	opt.nozip                 =  m_opt_section.get_bool
		("nozip",            false);
	opt.hw_improvements       =  m_opt_section.get_bool
		("hw_improvements",  false);
	opt.double_buffer         =  m_opt_section.get_bool
		("double_buffer",    false);
	opt.ui_font               =  m_opt_section.get_string
		("ui_font",     "serif");
	opt.speed_of_new_game     =  m_opt_section.get_int
		("speed_of_new_game", 1000);
	opt.border_snap_distance  =  m_opt_section.get_int
		("border_snap_distance", 0);
	opt.panel_snap_distance   =  m_opt_section.get_int
		("panel_snap_distance",  0);
	opt.remove_replays        = m_opt_section.get_int
		("remove_replays", 0);
	opt.remove_syncstreams    = m_opt_section.get_bool
		("remove_syncstreams", true);
	return opt;
}

void Options_Ctrl::save_options() {
	Options_Ctrl::Options_Struct opt = m_opt_dialog->get_values();
	m_opt_section.set_int ("xres",                  opt.xres);
	m_opt_section.set_int ("yres",                  opt.yres);
	m_opt_section.set_bool("fullscreen",            opt.fullscreen);
	m_opt_section.set_bool("inputgrab",             opt.inputgrab);
	m_opt_section.set_bool("single_watchwin",       opt.single_watchwin);
	m_opt_section.set_bool("auto_roadbuild_mode",   opt.auto_roadbuild_mode);
	m_opt_section.set_bool("workareapreview",       opt.show_warea);
	m_opt_section.set_bool
		("snap_windows_only_when_overlapping",
		 opt.snap_windows_only_when_overlapping);
	m_opt_section.set_bool("dock_windows_to_edges", opt.dock_windows_to_edges);
	m_opt_section.set_int ("depth",                 opt.depth);
	m_opt_section.set_bool("disable_music",        !opt.music);
	m_opt_section.set_bool("disable_fx",           !opt.fx);
	m_opt_section.set_string("language",            opt.language);
	m_opt_section.set_int("autosave",               opt.autosave * 60);
	m_opt_section.set_int("maxfps",                 opt.maxfps);

	m_opt_section.set_bool("sound_at_message",      opt.message_sound);
	m_opt_section.set_bool("nozip",                 opt.nozip);
	m_opt_section.set_bool("hw_improvements",       opt.hw_improvements);
	m_opt_section.set_bool("double_buffer",         opt.double_buffer);
	m_opt_section.set_string("ui_font",             opt.ui_font);
	m_opt_section.set_int("speed_of_new_game",      opt.speed_of_new_game);
	m_opt_section.set_int("border_snap_distance",   opt.border_snap_distance);
	m_opt_section.set_int("panel_snap_distance",    opt.panel_snap_distance);

	m_opt_section.set_int("remove_replays",         opt.remove_replays);
	m_opt_section.set_bool("remove_syncstreams",    opt.remove_syncstreams);

	WLApplication::get()->set_input_grab(opt.inputgrab);
	i18n::set_locale(opt.language);
	g_sound_handler.set_disable_music(!opt.music);
	g_sound_handler.set_disable_fx(!opt.fx);
}
