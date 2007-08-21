/*
 * Copyright (C) 2002-2007 by the Widelands Development Team
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

#ifndef __S__CAMPAIGN_SELECT_H
#define __S__CAMPAIGN_SELECT_H

#include "fullscreen_menu_base.h"

#include "ui_button.h"
#include "ui_listselect.h"
#include "ui_multilinetextarea.h"
#include "ui_textarea.h"
#include <string>

/*
 * Fullscreen Menu for all Campaigns
 */

/*
 * UI 1 - Selection of Campaign
 *
 */
struct Fullscreen_Menu_CampaignSelect : public Fullscreen_Menu_Base {
	Fullscreen_Menu_CampaignSelect();
	void clicked_back();
	void clicked_ok();
	void campaign_selected(uint);
	void double_clicked(uint);
	void fill_list();
	int get_campaign();

private:
	UI::Textarea					title;
	UI::Listselect<const char *>			list;
	UI::Textarea					label_campname;
	UI::Textarea					tacampname;
	UI::Textarea					label_difficulty;
	UI::Textarea					tadifficulty;
	UI::Textarea					label_campdescr;
	UI::Multiline_Textarea				tacampdescr;
	UI::Button<Fullscreen_Menu_CampaignSelect>	b_ok;
	UI::Button<Fullscreen_Menu_CampaignSelect>	back;

};

/*
 * UI 2 - Selection of a map
 *
 */
struct Fullscreen_Menu_CampaignMapSelect : public Fullscreen_Menu_Base {
	Fullscreen_Menu_CampaignMapSelect();
	void clicked_back();
	void clicked_ok();
	void map_selected(uint);
	void double_clicked(uint);
	void fill_list();
	std::string get_map();

private:
	UI::Textarea					title;
	UI::Listselect<const char *>			list;
	UI::Textarea					label_mapname;
	UI::Textarea					tamapname;
	UI::Textarea					label_author;
	UI::Textarea					taauthor;
	UI::Textarea					label_mapdescr;
	UI::Multiline_Textarea				tamapdescr;
	UI::Button<Fullscreen_Menu_CampaignMapSelect>	b_ok;
	UI::Button<Fullscreen_Menu_CampaignMapSelect>	back;

};
#endif // __S__CAMPAIGN_SELECT_H
