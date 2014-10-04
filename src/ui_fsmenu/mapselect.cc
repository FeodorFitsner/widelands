/*
 * Copyright (C) 2002, 2006-2013 by the Widelands Development Team
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
 */

#include "ui_fsmenu/mapselect.h"

#include <cstdio>
#include <memory>

#include <boost/format.hpp>

#include "base/deprecated.h"
#include "base/i18n.h"
#include "base/log.h"
#include "base/wexception.h"
#include "graphic/graphic.h"
#include "io/filesystem/layered_filesystem.h"
#include "logic/editor_game_base.h"
#include "logic/game_controller.h"
#include "logic/game_settings.h"
#include "map_io/widelands_map_loader.h"
#include "profile/profile.h"
#include "ui_basic/box.h"
#include "ui_basic/checkbox.h"
#include "wui/text_constants.h"


using Widelands::WidelandsMapLoader;

FullscreenMenuMapSelect::FullscreenMenuMapSelect
		(GameSettingsProvider * const settings, GameController * const ctrl) :
	FullscreenMenuBase("choosemapmenu.jpg"),

// Values for alignment and size
	m_butx (get_w() * 71 / 100),
	m_butw (get_w() / 4),
	m_buth (get_h() * 9 / 200),
	m_margin_left(get_w() *  47 / 2500),
	m_margin_right(get_w() - m_butx - m_butw),
	m_maplisty(get_h() * 17 / 50),
	m_maplistw(get_w() * 711 / 1250),
	m_nr_players_width(35),
	m_padding (5),
	m_space   (25),
	m_description_column_x(get_w() * 75 / 100),

// Text labels
	m_title
		(this,
		 get_w() / 2, get_h() / 25,
		 _("Choose a map"),
		 UI::Align_HCenter),
	m_label_load_map_as_scenario
		(this,
		 m_margin_left + 25, m_maplisty - 30,
		 _("Load map as scenario"),
		 UI::Align_Left),
	m_label_name
		(this,
		 m_description_column_x - m_padding, m_maplisty,
		 _("Name:"),
		 UI::Align_Right),
	m_name (this, m_description_column_x, m_maplisty, get_w() - m_description_column_x - m_margin_right, 35),
	m_label_author
		(this,
		 m_description_column_x - m_padding, m_name.get_y() + m_name.get_h() + m_padding,
		 _("Author:"),
		 UI::Align_Right),
	m_author (this, m_description_column_x, m_label_author.get_y()),
	m_label_size
		(this,
		 m_description_column_x - m_padding, m_author.get_y() + m_author.get_h() + m_padding,
		 _("Size:"),
		 UI::Align_Right),
	m_size (this, m_description_column_x, m_label_size.get_y()),
	m_label_nr_players
		(this,
		 m_description_column_x - m_padding, m_size.get_y() + m_size.get_h() + m_padding,
		 _("Players:"),
		 UI::Align_Right),
	m_nr_players (this, m_description_column_x, m_label_nr_players.get_y()),
	m_descr
		(this,
		 m_margin_left + m_maplistw + 15,
		 m_nr_players.get_y() + m_nr_players.get_h() + 2 * m_padding,
		 get_w() - m_margin_left - m_maplistw - m_margin_right - 15,
		 get_h() * 6 / 20),

// Buttons
	m_back
		(this, "back",
		 m_butx, get_h() * 17 / 20, m_butw, m_buth,
		 g_gr->images().get("pics/but0.png"),
		 _("Back"), std::string(), true, false),
	m_ok
		(this, "ok",
		 m_butx, get_h() * 9 / 10, m_butw, m_buth,
		 g_gr->images().get("pics/but2.png"),
		 _("OK"), std::string(), false, false),

// Checkbox
	m_load_map_as_scenario (this, Point (m_margin_left, m_maplisty - 30)),

// Map table
	m_table
		(this,
		 m_margin_left, m_maplisty,
		 m_maplistw, get_h() * 6083 / 10000),
	m_curdir("maps"),
	m_basedir("maps"),

	m_settings(settings),
	m_ctrl(ctrl)
{
	m_back.sigclicked.connect(boost::bind(&FullscreenMenuMapSelect::end_modal, boost::ref(*this), 0));
	m_ok.sigclicked.connect(boost::bind(&FullscreenMenuMapSelect::ok, boost::ref(*this)));

	m_title.set_textstyle(ts_big());

	/** TRANSLATORS: Column title for number of players in map list */
	m_table.add_column(m_nr_players_width, _("Pl."), "", UI::Align_HCenter);
	m_table.add_column
		(m_table.get_w() - m_nr_players_width, _("Map Name"), "", UI::Align_Left);
	m_table.set_column_compare
		(1,
		 boost::bind
		 (&FullscreenMenuMapSelect::compare_maprows, this, _1, _2));
	m_table.set_sort_column(0);
	m_load_map_as_scenario.set_state(false);
	m_load_map_as_scenario.set_enabled(false);

	m_table.selected.connect(boost::bind(&FullscreenMenuMapSelect::map_selected, this, _1));
	m_table.double_clicked.connect(boost::bind(&FullscreenMenuMapSelect::double_clicked, this, _1));

	UI::Box* vbox = new UI::Box(this, m_margin_left, m_maplisty - 150, UI::Box::Horizontal, 25, get_w());
	m_show_all_maps = _add_tag_checkbox(vbox, "blumba", _("Show all maps"));
	m_tags_checkboxes.clear(); // Remove this again, it is a special tag checkbox
	m_show_all_maps->set_state(true);
	vbox->set_size(get_w() - 2 * m_margin_left, 25);

	vbox = new UI::Box(this,
							 m_margin_left, vbox->get_y() + vbox->get_h() + m_padding,
							 UI::Box::Horizontal, 25, get_w());
	_add_tag_checkbox(vbox, "official", _("Official Map"));
	_add_tag_checkbox(vbox, "seafaring", _("Seafaring Map"));
	_add_tag_checkbox(vbox, "unbalanced", _("Unbalanced"));
	vbox->set_size(get_w() - 2 * m_margin_left, 25);

	vbox = new UI::Box(this,
							 m_margin_left, vbox->get_y() + vbox->get_h() + m_padding,
							 UI::Box::Horizontal, 25, get_w());
	_add_tag_checkbox(vbox, "ffa", _("Free for all"));
	_add_tag_checkbox(vbox, "1v1", _("1v1"));
	vbox->set_size(get_w() - 2 * m_margin_left, 25);

	vbox = new UI::Box(this,
							 m_margin_left, vbox->get_y() + vbox->get_h() + m_padding,
							 UI::Box::Horizontal, 25, get_w());
	_add_tag_checkbox(vbox, "2teams", _("Teams of 2"));
	_add_tag_checkbox(vbox, "3teams", _("Teams of 3"));
	_add_tag_checkbox(vbox, "4teams", _("Teams of 4"));
	vbox->set_size(get_w() - 2 * m_margin_left, 25);

	m_scenario_types = m_settings->settings().multiplayer ? Map::MP_SCENARIO : Map::SP_SCENARIO;
	if (m_scenario_types) {
		m_load_map_as_scenario.set_visible(true);
		m_label_load_map_as_scenario.set_visible(true);
	} else {
		m_load_map_as_scenario.set_visible(false);
		m_label_load_map_as_scenario.set_visible(false);
	}

	m_table.focus();
	fill_list();
}

void FullscreenMenuMapSelect::think()
{
	if (m_ctrl)
		m_ctrl->think();
}


bool FullscreenMenuMapSelect::compare_maprows
	(uint32_t rowa, uint32_t rowb)
{
	const MapData & r1 = m_maps_data[m_table[rowa]];
	const MapData & r2 = m_maps_data[m_table[rowb]];

	if (!r1.width && !r2.width) {
		return r1.name < r2.name;
	} else if (!r1.width && r2.width) {
		return true;
	} else if (r1.width && !r2.width) {
		return false;
	}
	return r1.name < r2.name;
}

bool FullscreenMenuMapSelect::is_scenario()
{
	return m_load_map_as_scenario.get_state();
}

MapData const * FullscreenMenuMapSelect::get_map() const
{
	if (!m_table.has_selection())
		return nullptr;
	return &m_maps_data[m_table.get_selected()];
}


void FullscreenMenuMapSelect::ok()
{
	const MapData & mapdata = m_maps_data[m_table.get_selected()];

	if (!mapdata.width) {
		m_curdir = mapdata.filename;
		fill_list();
	} else
		end_modal(1 + is_scenario());
}


/**
 * Called when a different entry in the listbox gets selected.
 * When this happens, the information display at the right needs to be
 * refreshed.
 */
void FullscreenMenuMapSelect::map_selected(uint32_t)
{
	const MapData & map = m_maps_data[m_table.get_selected()];

	if (map.width) {
		char buf[256];

		// Translate the map data
		i18n::Textdomain td("maps");
		m_name      .set_text(_(map.name));
		m_author    .set_text(map.author);
		sprintf(buf, "%-4ux%4u", map.width, map.height);
		m_size      .set_text(buf);
		sprintf(buf, "%i", map.nrplayers);
		m_nr_players.set_text(buf);
		m_descr     .set_text(_(map.description) + (map.hint.empty() ? "" : (std::string("\n") + _(map.hint))));
		m_load_map_as_scenario.set_enabled(map.scenario);
	} else {
		// Directory
		m_name      .set_text(_("(directory)"));
		m_author    .set_text(std::string());
		m_size      .set_text(std::string());
		m_nr_players.set_text(std::string());
		m_descr     .set_text(std::string());
		m_load_map_as_scenario.set_enabled(false);
	}
	m_ok.set_enabled(true);
	m_load_map_as_scenario.set_state(false); // reset
}

/**
 * listbox got double clicked
 */
void FullscreenMenuMapSelect::double_clicked(uint32_t) {
	ok();
}

/**
 * Fill the list with maps that can be opened.
 *
 *
 * At first, only the subdirectories are added to the list, then the normal
 * files follow. This is done to make navigation easier.
 *
 * To make things more difficult, we have to support compressed and uncompressed
 * map files here - the former are files, the latter are directories. Care must
 * be taken to sort uncompressed maps (which look like and really are
 * directories) with the files.
 *
 * The search starts in \ref m_curdir ("..../maps") and there is no possibility
 * to move further up. If the user moves down into subdirectories, we insert an
 * entry to move back up.
 *
 * \note special case is, if this is a multiplayer game on a dedicated server and
 * the client wants to change the map - in that case the maps available on the server are shown.
 */
void FullscreenMenuMapSelect::fill_list()
{
	m_maps_data.clear();
	m_table.clear();

	if (m_settings->settings().maps.empty()) {
		// This is the normal case

		//  Fill it with all files we find in all directories.
		FilenameSet files = g_fs->list_directory(m_curdir);

		int32_t ndirs = 0;

		//If we are not at the top of the map directory hierarchy (we're not talking
		//about the absolute filesystem top!) we manually add ".."
		if (m_curdir != m_basedir) {
			MapData map;
	#ifndef _WIN32
			map.filename = m_curdir.substr(0, m_curdir.rfind('/'));
	#else
			map.filename = m_curdir.substr(0, m_curdir.rfind('\\'));
	#endif
			m_maps_data.push_back(map);
			UI::Table<uintptr_t const>::EntryRecord & te =
				m_table.add(m_maps_data.size() - 1);

			te.set_string(0, "");
			std::string parent_string =
				(boost::format("\\<%s\\>") % _("parent")).str();
			te.set_picture
				(1,  g_gr->images().get("pics/ls_dir.png"),
				parent_string);

			++ndirs;
		}

		//Add subdirectories to the list (except for uncompressed maps)
		for
			(FilenameSet::iterator pname = files.begin();
			pname != files.end();
			++pname)
		{
			char const * const name = pname->c_str();
			if (!strcmp(FileSystem::fs_filename(name), "."))
				continue;
			// Upsy, appeared again. ignore
			if (!strcmp(FileSystem::fs_filename(name), ".."))
				continue;
			if (!g_fs->is_directory(name))
				continue;
			if (WidelandsMapLoader::is_widelands_map(name))
				continue;

			MapData dir;
			dir.filename = name;

			m_maps_data.push_back(dir);
			UI::Table<uintptr_t const>::EntryRecord & te = m_table.add(m_maps_data.size() - 1);

			te.set_string(0, "");
			te.set_picture
				(1,  g_gr->images().get("pics/ls_dir.png"),
				FileSystem::fs_filename(name));

			++ndirs;
		}

		//Add map files(compressed maps) and directories(uncompressed)
		{
			Widelands::Map map; //  MapLoader needs a place to put its preload data

			for (const std::string& filename : files) {

				std::unique_ptr<Widelands::MapLoader> ml = map.get_correct_loader(filename);
				if (!ml)
					continue;

				try {
					map.set_filename(filename);
					ml->preload_map(true);

					MapData mapdata;
					mapdata.filename    = filename;
					mapdata.name        = map.get_name();
					mapdata.author      = map.get_author();
					mapdata.description = map.get_description();
					mapdata.hint        = map.get_hint();
					mapdata.nrplayers   = map.get_nrplayers();
					mapdata.width       = map.get_width();
					mapdata.height      = map.get_height();
					mapdata.scenario    = map.scenario_types() & m_scenario_types;
					mapdata.tags        = map.get_tags();

					if (!mapdata.width || !mapdata.height)
						continue;

					bool has_all_tags = true;
					for (std::set<uint32_t>::const_iterator it = m_req_tags.begin(); it != m_req_tags.end(); ++it)
						has_all_tags &= mapdata.tags.count(m_tags_ordered[*it]);
					if (!has_all_tags)
						continue;


					m_maps_data.push_back(mapdata);
					UI::Table<uintptr_t const>::EntryRecord & te = m_table.add(m_maps_data.size() - 1);

					char buf[256];
					sprintf(buf, "(%i)", mapdata.nrplayers);
					te.set_string(0, buf);
					i18n::Textdomain td("maps");
					te.set_picture
						(1,  g_gr->images().get
						 (dynamic_cast<WidelandsMapLoader*>(ml.get()) ?
							  (mapdata.scenario ? "pics/ls_wlscenario.png" : "pics/ls_wlmap.png") :
						"pics/ls_s2map.png"),
						_(mapdata.name));
				} catch (const std::exception & e) {
					log
						("Mapselect: Skip %s due to preload error: %s\n",
						filename.c_str(), e.what());
				} catch (...) {
					log("Mapselect: Skip %s due to unknown exception\n", filename.c_str());
				}
			}
		}
	} else {
		//client changing maps on dedicated server
		for (uint16_t i = 0; i < m_settings->settings().maps.size(); ++i) {
			Widelands::Map map; //  MapLoader needs a place to put its preload data
			i18n::Textdomain td("maps");
			MapData mapdata;

			const DedicatedMapInfos & dmap = m_settings->settings().maps.at(i);
			const std::string& filename = dmap.path;
			std::unique_ptr<Widelands::MapLoader> ml(map.get_correct_loader(filename));
			try {
				if (!ml)
					throw wexception("Not useable!");

				map.set_filename(filename);
				ml->preload_map(true);

				mapdata.filename    = filename;
				mapdata.name        = map.get_name();
				mapdata.author      = map.get_author();
				mapdata.description = map.get_description();
				mapdata.hint        = map.get_hint();
				mapdata.nrplayers   = map.get_nrplayers();
				mapdata.width       = map.get_width();
				mapdata.height      = map.get_height();
				mapdata.scenario    = map.scenario_types() & m_scenario_types;

				if (mapdata.nrplayers != dmap.players || mapdata.scenario != dmap.scenario)
					throw wexception("Not useable!");

				if (!mapdata.width || !mapdata.height)
					throw wexception("Not useable!");

				// Finally write the entry to the list
				m_maps_data.push_back(mapdata);
				UI::Table<uintptr_t const>::EntryRecord & te = m_table.add(m_maps_data.size() - 1);

				char buf[256];
				sprintf(buf, "(%i)", mapdata.nrplayers);
				te.set_string(0, buf);
				te.set_picture
					(1, g_gr->images().get((mapdata.scenario ? "pics/ls_wlscenario.png" : "pics/ls_wlmap.png")),
					 mapdata.name.c_str());

			} catch (...) {
				log("Mapselect: Skipped reading locale data for file %s - not valid.\n", filename.c_str());

				// Fill in the data we got from the dedicated server
				mapdata.filename    = filename;
				mapdata.name        = filename.substr(5, filename.size() - 1);
				mapdata.author      = _("unknown");
				mapdata.description = _("This map file is not present in your filesystem."
							" The data shown here was sent by the server.");
				mapdata.hint        = "";
				mapdata.nrplayers   = dmap.players;
				mapdata.width       = 1;
				mapdata.height      = 0;
				mapdata.scenario    = dmap.scenario;

				// Finally write the entry to the list
				m_maps_data.push_back(mapdata);
				UI::Table<uintptr_t const>::EntryRecord & te = m_table.add(m_maps_data.size() - 1);

				char buf[256];
				sprintf(buf, "(%i)", mapdata.nrplayers);
				te.set_string(0, buf);
				te.set_picture
					(1, g_gr->images().get
					 ((mapdata.scenario ? "pics/ls_wlscenario.png" : "pics/ls_wlmap.png")), mapdata.name.c_str());
			}
		}
	}

	m_table.sort();

	if (m_table.size())
		m_table.select(0);
}

/*
 * Add a tag to the checkboxes
 */
UI::Checkbox * FullscreenMenuMapSelect::_add_tag_checkbox
	(UI::Box * box, std::string tag, std::string displ_name)
{
	int32_t id = m_tags_ordered.size();
	m_tags_ordered.push_back(tag);

	UI::Checkbox * cb = new UI::Checkbox(box, Point(0, 0));
	cb->changedto.connect
		(boost::bind(&FullscreenMenuMapSelect::_tagbox_changed, this, id, _1));

	box->add(cb, UI::Box::AlignLeft, true);
	UI::Textarea * ta = new UI::Textarea(box, displ_name, UI::Align_CenterLeft);
	box->add_space(m_padding);
	box->add(ta, UI::Box::AlignLeft);
	box->add_space(m_space);

	m_tags_checkboxes.push_back(cb);

	return cb;
}

/*
 * One of the tagboxes has changed
 */
void FullscreenMenuMapSelect::_tagbox_changed(int32_t id, bool to) {
	if (id == 0) { // Show all maps checbox
		if (to) {
			for (UI::Checkbox * checkbox : m_tags_checkboxes) {
				checkbox->set_state(false);
			}
		}
	} else { // Any tag
		if (to)
			m_req_tags.insert(id);
		else
			m_req_tags.erase(id);
	}
	if (m_req_tags.empty())
		m_show_all_maps->set_state(true);
	else
		m_show_all_maps->set_state(false);

	fill_list();
}
