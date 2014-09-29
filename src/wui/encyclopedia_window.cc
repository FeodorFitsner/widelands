/*
 * Copyright (C) 2002-2004, 2006-2011 by the Widelands Development Team
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
 *
 */

#include "wui/encyclopedia_window.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <typeinfo>
#include <vector>

#include "base/i18n.h"
#include "base/macros.h"
#include "economy/economy.h"
#include "graphic/graphic.h"
#include "logic/building.h"
#include "logic/player.h"
#include "logic/production_program.h"
#include "logic/productionsite.h"
#include "logic/tribe.h"
#include "logic/warelist.h"
#include "ui_basic/table.h"
#include "ui_basic/unique_window.h"
#include "ui_basic/window.h"
#include "wui/interactive_player.h"

#define WINDOW_WIDTH std::min(700, g_gr->get_xres() - 40)
#define WINDOW_HEIGHT std::min(550, g_gr->get_yres() - 40)
constexpr uint32_t quantityColumnWidth = 100;
constexpr uint32_t wareColumnWidth = 250;
#define PRODSITE_GROUPS_WIDTH (WINDOW_WIDTH - wareColumnWidth - quantityColumnWidth - 10)

using namespace Widelands;

inline InteractivePlayer & EncyclopediaWindow::iaplayer() const {
	return ref_cast<InteractivePlayer, UI::Panel>(*get_parent());
}


EncyclopediaWindow::EncyclopediaWindow
	(InteractivePlayer & parent, UI::UniqueWindow::Registry & registry)
:
	UI::UniqueWindow
		(&parent, "encyclopedia",
		 &registry,
		 WINDOW_WIDTH, WINDOW_HEIGHT,
		 _("Tribal Ware Encyclopedia")),
	wares            (this, 5, 5, WINDOW_WIDTH - 10, WINDOW_HEIGHT - 250),
	prodSites        (this, 5, WINDOW_HEIGHT - 150, PRODSITE_GROUPS_WIDTH, 145),
	condTable
		(this,
		 PRODSITE_GROUPS_WIDTH + 5, WINDOW_HEIGHT - 150, WINDOW_WIDTH - PRODSITE_GROUPS_WIDTH - 5, 145),
	descrTxt         (this, 5, WINDOW_HEIGHT - 240, WINDOW_WIDTH - 10, 80, "")
{
	wares.selected.connect(boost::bind(&EncyclopediaWindow::wareSelected, this, _1));

	prodSites.selected.connect(boost::bind(&EncyclopediaWindow::prodSiteSelected, this, _1));
	condTable.add_column
			/** TRANSLATORS: Column title in the Tribal Wares Encyclopedia */
			(wareColumnWidth, ngettext("Consumed Ware Type", "Consumed Ware Types", 0));
	condTable.add_column (quantityColumnWidth, _("Quantity"));

	fillWares();

	if (get_usedefaultpos())
		center_to_parent();
}

void EncyclopediaWindow::fillWares() {
	const TribeDescr & tribe = iaplayer().player().tribe();
	WareIndex const nr_wares = tribe.get_nrwares();
	std::vector<Ware> ware_vec;

	for (WareIndex i = 0; i < nr_wares; ++i) {
		WareDescr const * ware = tribe.get_ware_descr(i);
		Ware w(i, ware);
		ware_vec.push_back(w);
	}

	std::sort(ware_vec.begin(), ware_vec.end());

	for (uint32_t i = 0; i < ware_vec.size(); i++) {
		Ware cur = ware_vec[i];
		wares.add(cur.m_descr->descname().c_str(), cur.m_i, cur.m_descr->icon());
	}
}

void EncyclopediaWindow::wareSelected(uint32_t) {
	const TribeDescr & tribe = iaplayer().player().tribe();
	selectedWare = tribe.get_ware_descr(wares.get_selected());

	descrTxt.set_text(selectedWare->helptext());

	prodSites.clear();
	condTable.clear();

	bool found = false;

	BuildingIndex const nr_buildings = tribe.get_nrbuildings();
	for (BuildingIndex i = 0; i < nr_buildings; ++i) {
		const BuildingDescr & descr = *tribe.get_building_descr(i);
		if (upcast(ProductionSiteDescr const, de, &descr)) {

			if
				((descr.is_buildable() || descr.is_enhanced())
				 &&
				 de->output_ware_types().count(wares.get_selected()))
			{
				prodSites.add(de->descname().c_str(), i, de->get_icon());
				found = true;
			}
		}
	}
	if (found)
		prodSites.select(0);

}

void EncyclopediaWindow::prodSiteSelected(uint32_t) {
	assert(prodSites.has_selection());
	size_t no_of_wares = 0;
	condTable.clear();
	const TribeDescr & tribe = iaplayer().player().tribe();

	const ProductionSiteDescr::Programs & programs =
		ref_cast<ProductionSiteDescr const, BuildingDescr const>
			(*tribe.get_building_descr(prodSites.get_selected()))
		.programs();

	//  TODO(unknown): This needs reworking. A program can indeed produce iron even if
	//  the program name is not any of produce_iron, smelt_iron, prog_iron
	//  or work. What matters is whether the program has a statement such
	//  as "produce iron" or "createware iron". The program name is not
	//  supposed to have any meaning to the game logic except to uniquely
	//  identify the program.
	//  Only shows information from the first program that has a name indicating
	//  that it produces the considered ware type.
	std::map<std::string, ProductionProgram *>::const_iterator programIt =
		programs.find(std::string("produce_") + selectedWare->name());

	if (programIt == programs.end())
		programIt = programs.find(std::string("smelt_")  + selectedWare->name());

	if (programIt == programs.end())
		programIt = programs.find(std::string("smoke_")  + selectedWare->name());

	if (programIt == programs.end())
		programIt = programs.find(std::string("mine_")   + selectedWare->name());

	if (programIt == programs.end())
		programIt = programs.find("work");

	if (programIt != programs.end()) {
		const ProductionProgram::Actions & actions =
			programIt->second->actions();

		for (const ProductionProgram::Action * temp_action : actions) {
			if (upcast(ProductionProgram::ActConsume const, action, temp_action)) {
				const ProductionProgram::ActConsume::Groups & groups =
					action->groups();

				for (const ProductionProgram::WareTypeGroup& temp_group : groups) {
					const std::set<WareIndex> & ware_types = temp_group.first;
					assert(ware_types.size());
					std::vector<std::string> ware_type_descnames;
					for (const WareIndex& ware_index : ware_types) {
						ware_type_descnames.push_back(tribe.get_ware_descr(ware_index)->descname());
					}
					no_of_wares = no_of_wares + ware_types.size();

					std::string ware_type_names =
							i18n::localize_item_list(ware_type_descnames, i18n::ConcatenateWith::OR);

					//  Make sure to detect if someone changes the type so that it
					//  needs more than 3 decimal digits to represent.
					static_assert(sizeof(temp_group.second) == 1, "Number is too big for 3 char string.");
					char amount_string[4]; //  Space for 3 digits + terminator.
					sprintf(amount_string, "%u", temp_group.second);

					//  picture only of first ware type in group
					UI::Table<uintptr_t>::EntryRecord & tableEntry =
						condTable.add(0);
					tableEntry.set_picture
						(0, tribe.get_ware_descr(*ware_types.begin())->icon(), ware_type_names);
					tableEntry.set_string (1, amount_string);
					condTable.set_sort_column(0);
					condTable.sort();
				}
			}
		}
	}
	condTable.set_column_title(0,	ngettext("Consumed Ware Type", "Consumed Ware Types", no_of_wares));
}
