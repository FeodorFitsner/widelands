/*
 * Copyright (C) 2002-2004, 2006-2008, 2010 by the Widelands Development Team
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

#include "game_main_menu_save_game.h"

#include "io/filesystem/filesystem.h"
#include "constants.h"
#include "logic/game.h"
#include "game_io/game_loader.h"
#include "game_io/game_preload_data_packet.h"
#include "game_io/game_saver.h"
#include "interactive_gamebase.h"
#include "io/filesystem/layered_filesystem.h"
#include "profile/profile.h"

#include <boost/format.hpp>
using boost::format;

Interactive_GameBase & Game_Main_Menu_Save_Game::igbase() {
	return ref_cast<Interactive_GameBase, UI::Panel>(*get_parent());
}


Game_Main_Menu_Save_Game::Game_Main_Menu_Save_Game
	(Interactive_GameBase & parent, UI::UniqueWindow::Registry & registry)
:
#define WINDOW_WIDTH                                                        440
#define WINDOW_HEIGHT                                                       440
#define VMARGIN                                                               5
#define HMARGIN                                                               5
#define VSPACING                                                              5
#define HSPACING                                                              5
#define EDITBOX_HEIGHT                                                       20
#define BUTTON_HEIGHT                                                        20
#define LIST_WIDTH                                                          280
#define LIST_HEIGHT   (WINDOW_HEIGHT - 2 * VMARGIN - VSPACING - EDITBOX_HEIGHT)
#define EDITBOX_Y                    (WINDOW_HEIGHT - EDITBOX_HEIGHT - VMARGIN)
#define DESCRIPTION_X                         (VMARGIN + LIST_WIDTH + VSPACING)
#define DESCRIPTION_WIDTH              (WINDOW_WIDTH - DESCRIPTION_X - VMARGIN)
#define CANCEL_Y                      (WINDOW_HEIGHT - BUTTON_HEIGHT - VMARGIN)
#define DELETE_Y                          (CANCEL_Y - BUTTON_HEIGHT - VSPACING)
#define OK_Y                              (DELETE_Y - BUTTON_HEIGHT - VSPACING)
	UI::UniqueWindow
		(&parent, "save_game", &registry,
		 WINDOW_WIDTH, WINDOW_HEIGHT, _("Save Game")),
	m_ls     (this, HSPACING, VSPACING,  LIST_WIDTH, LIST_HEIGHT),
	m_editbox
		(*this, HSPACING, EDITBOX_Y, LIST_WIDTH, EDITBOX_HEIGHT),
	m_name_label
		(this, DESCRIPTION_X,  5, 0, 20, _("Map Name: "),  UI::Align_CenterLeft),
	m_name
		(this, DESCRIPTION_X, 20, 0, 20, " ",              UI::Align_CenterLeft),
	m_gametime_label
		(this, DESCRIPTION_X, 45, 0, 20, _("Game Time: "), UI::Align_CenterLeft),
	m_gametime
		(this, DESCRIPTION_X, 60, 0, 20, " ",              UI::Align_CenterLeft),
	m_button_ok
		(*this, DESCRIPTION_X, OK_Y, DESCRIPTION_WIDTH, BUTTON_HEIGHT),
	m_button_cancel
		(*this, DESCRIPTION_X, CANCEL_Y, DESCRIPTION_WIDTH, BUTTON_HEIGHT),
	m_button_delete
		(*this, DESCRIPTION_X, DELETE_Y, DESCRIPTION_WIDTH, BUTTON_HEIGHT),
	m_curdir(SaveHandler::get_base_dir())
{
	m_ls.selected.set(this, &Game_Main_Menu_Save_Game::selected);
	m_ls.double_clicked.set(this, &Game_Main_Menu_Save_Game::double_clicked);

	fill_list();

	center_to_parent();
	move_to_top();

	m_editbox.focus();
}


/**
 * called when a item is selected
 */
void Game_Main_Menu_Save_Game::selected(uint32_t) {
	std::string const & name = m_ls.get_selected();

	Widelands::Game_Loader gl(name, igbase().game());
	Widelands::Game_Preload_Data_Packet gpdp;
	gl.preload_game(gpdp); //  This has worked before, no problem

	{
		m_editbox.setText(FileSystem::FS_FilenameWoExt(name.c_str()));
	}
	m_button_ok.set_enabled(true);

	m_name.set_text(gpdp.get_mapname());
	char buf[200];
	uint32_t gametime = gpdp.get_gametime();
#define SPLIT_GAMETIME(unit, factor) \
   uint32_t const unit = gametime / factor; gametime %= factor;
	SPLIT_GAMETIME(days, 86400000);
	SPLIT_GAMETIME(hours, 3600000);
	SPLIT_GAMETIME(minutes, 60000);
	SPLIT_GAMETIME(seconds,  1000);
	sprintf
		(buf,
		 _("%02ud%02uh%02u'%02u\"%03u"),
		 days, hours, minutes, seconds, gametime);
	m_gametime.set_text(buf);
}

/**
 * An Item has been doubleclicked
 */
void Game_Main_Menu_Save_Game::double_clicked(uint32_t) {
	m_button_ok.clicked();
}

/*
 * fill the file list
 */
void Game_Main_Menu_Save_Game::fill_list() {
	m_ls.clear();
	filenameset_t m_gamefiles;

	//  Fill it with all files we find.
	g_fs->FindFiles(m_curdir, "*", &m_gamefiles, 0);

	Widelands::Game_Preload_Data_Packet gpdp;

	for
		(filenameset_t::iterator pname = m_gamefiles.begin();
		 pname != m_gamefiles.end();
		 ++pname)
	{
		char const * const name = pname->c_str();

		try {
			Widelands::Game_Loader gl(name, igbase().game());
			gl.preload_game(gpdp);
			m_ls.add(FileSystem::FS_FilenameWoExt(name).c_str(), name);
		} catch (_wexception const &) {} //  we simply skip illegal entries
	}

	if (m_ls.size())
		m_ls.select(0);
}

/*
 * The editbox was changed. Enable ok button
 */
void Game_Main_Menu_Save_Game::edit_box_changed() {
	m_button_ok.set_enabled(m_editbox.text().size());
}


bool Game_Main_Menu_Save_Game::EditBox::handle_key
	(bool const down, SDL_keysym const code)
{
	switch (code.sym) {
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
		if (down and text().size()) {
			play_click();
			ref_cast<Game_Main_Menu_Save_Game, UI::Panel>(*get_parent())
			.m_button_ok.clicked();
		}
		return true;
	default:
		break;
	}
	return UI::EditBox::handle_key(down, code);
}

static void dosave
	(Interactive_GameBase & igbase, std::string const & complete_filename)
{
	Widelands::Game & game = igbase.game();

	std::string error;
	if (!game.save_handler().save_game(game, complete_filename, &error)) {
		std::string s =
			_
			("Game Saving Error!\nSaved Game-File may be corrupt!\n\n"
			 "Reason given:\n");
		s += error;
		UI::WLMessageBox mbox
			(&igbase, _("Save Game Error!!"), s, UI::WLMessageBox::OK);
		mbox.run();
	}
}

struct SaveWarnMessageBox : public UI::WLMessageBox {
	SaveWarnMessageBox
		(Game_Main_Menu_Save_Game & parent, std::string const & filename)
		:
		UI::WLMessageBox
			(&parent,
			 _("Save Game Error!!"),
			 std::string(_("A File with the name "))
			 +
			 FileSystem::FS_Filename(filename.c_str())
			 +
			 _(" already exists. Overwrite?"),
			 YESNO),
		m_filename(filename)
	{}

	Game_Main_Menu_Save_Game & menu_save_game() {
		return ref_cast<Game_Main_Menu_Save_Game, UI::Panel>(*get_parent());
	}


	void pressedYes()
	{
		g_fs->Unlink(m_filename);
		dosave(menu_save_game().igbase(), m_filename);
		menu_save_game().die();
	}

	void pressedNo()
	{
		die();
	}

private:
	std::string const m_filename;
};


/*
===========
called when the ok button has been clicked
===========
*/
void Game_Main_Menu_Save_Game::Ok::clicked() {
	Game_Main_Menu_Save_Game & menu =
		ref_cast<Game_Main_Menu_Save_Game, UI::Panel>(*get_parent());
	Interactive_GameBase & igbase = menu.igbase();
	std::string const complete_filename =
		igbase.game().save_handler().create_file_name
			(menu.m_curdir, menu.m_editbox.text());

	//  Check if file exists. If it does, show a warning.
	if (g_fs->FileExists(complete_filename)) {
		new SaveWarnMessageBox(menu, complete_filename);
	} else {
		dosave(igbase, complete_filename);
		menu.die();
	}
}


struct DeletionMessageBox : public UI::WLMessageBox {
	DeletionMessageBox
		(Game_Main_Menu_Save_Game & parent, std::string const & filename)
		:
		UI::WLMessageBox
			(&parent,
			 _("File deletion"),
			 str
				 (format(_("Do you really want to delete the file %s?")) %
				  FileSystem::FS_Filename(filename.c_str())),
			 YESNO),
		m_filename(filename)
	{}

	void pressedYes()
	{
		g_fs->Unlink(m_filename);
		ref_cast<Game_Main_Menu_Save_Game, UI::Panel>(*get_parent()).fill_list();
		die();
	}

	void pressedNo()
	{
		die();
	}

private:
	std::string const m_filename;
};


/*
===========
called when the delete button has been clicked
===========
*/
void Game_Main_Menu_Save_Game::Delete::clicked() {
	Game_Main_Menu_Save_Game & menu =
		ref_cast<Game_Main_Menu_Save_Game, UI::Panel>(*get_parent());
	Interactive_GameBase & igbase = menu.igbase();
	std::string const complete_filename =
		igbase.game().save_handler().create_file_name
			(menu.m_curdir, menu.m_editbox.text());

	//  Check if file exists. If it does, let the user confirm the deletion.
	if (g_fs->FileExists(complete_filename))
		new DeletionMessageBox(menu, complete_filename);
}
