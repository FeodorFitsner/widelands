/*
 * Copyright (C) 2002, 2006-2010 by the Widelands Development Team
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

#include "logic/tribe.h"

#include <algorithm>
#include <iostream>
#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "base/i18n.h"
#include "base/macros.h"
#include "graphic/graphic.h"
#include "helper.h"
#include "io/fileread.h"
#include "io/filesystem/disk_filesystem.h"
#include "io/filesystem/layered_filesystem.h"
#include "logic/carrier.h"
#include "logic/constructionsite.h"
#include "logic/dismantlesite.h"
#include "logic/editor_game_base.h"
#include "logic/game.h"
#include "logic/game_data_error.h"
#include "logic/immovable.h"
#include "logic/militarysite.h"
#include "logic/ship.h"
#include "logic/soldier.h"
#include "logic/trainingsite.h"
#include "logic/warehouse.h"
#include "logic/worker.h"
#include "logic/world/resource_description.h"
#include "logic/world/world.h"
#include "profile/profile.h"
#include "scripting/lua_interface.h"
#include "scripting/lua_table.h"


using namespace std;
/*
 * NOCOM todo
 icon = "images/atlanteans/icon.png",
 */

namespace Widelands {

TribeDescr::TribeDescr
	(const LuaTable& table, EditorGameBase& input_egbase)
	: name_(table.get_string("name")), egbase_(input_egbase)
{

	try {
		// Grab the localization textdomain.
		i18n::Textdomain td(std::string("tribes"));

		// NOCOM(GunChleoc) Use these? table.get_string("author");
		//table.get_string("descname"); // descriptive name
		//table.get_string("helptext"); // long description

		// NOCOM(GunChleoc): Fix these animations
		/*
		m_frontier_animation_id =
			g_gr->animations().load(path, root_conf.get_safe_section("frontier"));
		m_flag_animation_id =
			g_gr->animations().load(path, root_conf.get_safe_section("flag"));
		*/

		std::unique_ptr<LuaTable> items_table = table.get_table("animations");
		std::unique_ptr<LuaTable> anims_table = items_table->get_table("frontier");
		// NOCOM m_frontier_animation_id = g_gr->animations().load(anims_table->get_string("pictures"));
		anims_table = items_table->get_table("flag");
		// NOCOM m_flag_animation_id = g_gr->animations().load(anims_table->get_string("pictures"));
		// NOCOM(GunChleoc): And the hotspot + fps?

		items_table = table.get_table("wares_order");
		for (const int key : items_table->keys<int>()) {
			std::unique_ptr<LuaTable> column_table = items_table->get_table(key);
			vector<WareIndex> column;
			for (const int column_key : column_table->keys<int>()) {
				const std::string warename = column_table->get_string(column_key);
				try {
					WareIndex index = egbase_.tribes().safe_ware_index(warename);
					if (wares_.count(index) == 1) {
						throw new GameDataError("Duplicate definition of ware '%s'", warename.c_str());
					}
					wares_.insert(index);
					column.push_back(index);
					wares_order_coords_.resize(wares_.size());
					wares_order_coords_[index] =
							std::pair<uint32_t, uint32_t>(wares_order_.size(), column.size() - 1);
				} catch (const WException& e) {
					throw new GameDataError("Failed adding ware '%s to tribe '%s': %s",
													warename.c_str(), name_.c_str(), e.what());
				}
			}
			if (!column.empty()) wares_order_.push_back(column);
		}

		items_table = table.get_table("immovables");
		for (const std::string& key : items_table->keys<std::string>()) {
			const std::string immovablename = items_table->get_string(key);
			try {
				int index = egbase_.tribes().safe_immovable_index(immovablename);
				if (immovables_.count(index) == 1) {
					throw new GameDataError("Duplicate definition of immovable '%s'", immovablename.c_str());
				}
				immovables_.insert(index);
			} catch (const WException& e) {
				throw new GameDataError("Failed adding immovable '%s' to tribe '%s': %s",
												immovablename.c_str(), name_.c_str(), e.what());
			}
		}

		items_table = table.get_table("workers_order");
		for (const int key : items_table->keys<int>()) {
			std::unique_ptr<LuaTable> column_table = items_table->get_table(key);
			vector<WareIndex> column;
			for (const int column_key : column_table->keys<int>()) {
				const std::string workername = column_table->get_string(column_key);
				try {
					WareIndex index = egbase_.tribes().safe_worker_index(workername);
					if (workers_.count(index) == 1) {
						throw new GameDataError("Duplicate definition of worker '%s'", workername.c_str());
					}
					workers_.insert(index);
					if (egbase_.tribes().get_worker_descr(index)->buildcost().empty()) {
						worker_types_without_cost_.push_back(index);
					}
					column.push_back(index);
					workers_order_coords_.resize(workers_.size());
					workers_order_coords_[index] =
							std::pair<uint32_t, uint32_t>(workers_order_.size(), column.size() - 1);
				} catch (const WException& e) {
					throw new GameDataError("Failed adding worker '%s' to tribe '%s': %s",
													workername.c_str(), name_.c_str(), e.what());
				}
			}
			if (!column.empty()) workers_order_.push_back(column);
		}

		items_table = table.get_table("constructionsites");
		for (const int key : items_table->keys<int>()) {
			add_building(items_table->get_string(key));
		}

		items_table = table.get_table("dismantlesites");
		for (const int key : items_table->keys<int>()) {
			add_building(items_table->get_string(key));
		}

		items_table = table.get_table("militarysites");
		for (const int key : items_table->keys<int>()) {
			add_building(items_table->get_string(key));
		}

		items_table = table.get_table("trainingsites");
		for (const int key : items_table->keys<int>()) {
			add_building(items_table->get_string(key));
		}

		items_table = table.get_table("productionsites");
		for (const int key : items_table->keys<int>()) {
			add_building(items_table->get_string(key));
		}

		items_table = table.get_table("warehouses");
		for (const int key : items_table->keys<int>()) {
			add_building(items_table->get_string(key));
		}

		builder_ = add_special_worker(table.get_string("builder"));
		carrier_ = add_special_worker(table.get_string("carrier"));
		carrier2_ = add_special_worker(table.get_string("carrier2"));
		geologist_ = add_special_worker(table.get_string("geologist"));
		soldier_ = add_special_worker(table.get_string("soldier"));

		const std::string shipname = table.get_string("ship");
		try {
			ship_ = egbase_.tribes().safe_ship_index(shipname);
		} catch (const WException& e) {
			throw new GameDataError("Failed adding ship '%s' to tribe '%s': %s",
											shipname.c_str(), name_.c_str(), e.what());
		}

		try {
			// NOCOM(GunChleoc): Use the new init.lua in tribes/scripting
			const std::string path = "tribes/scripting/starting_conditions/";

			// Read initializations -- all scripts are initializations currently
			for (const std::string& script :
				  filter(g_fs->list_directory(path + name_),
							[](const string& fn) {return boost::ends_with(fn, ".lua");})) {
				std::unique_ptr<LuaTable> t = egbase_.lua().run_script(script);
				t->do_not_warn_about_unaccessed_keys();

				m_initializations.resize(m_initializations.size() + 1);
				Initialization& init = m_initializations.back();
				init.script = script;
				init.descname = t->get_string("name");
			}
		} catch (const std::exception & e) {
			throw GameDataError("tribe scripting: %s", e.what());
		}
	} catch (const WException & e) {
		throw GameDataError("tribe %s: %s", name_.c_str(), e.what());
	}
}


/*
 * does this tribe exist?
 */
bool TribeDescr::exists_tribe
	(const std::string & tribename, TribeBasicInfo * const info)
{
	const std::string initfile = "/tribes/" + tribename + ".lua";
	try {
		LuaInterface lua;
		std::unique_ptr<LuaTable> table(lua.run_script(initfile));
		if (info) {
			try {
				info->name = tribename;
				info->uiposition = table->has_key("uiposition") ? table->get_int("uiposition") : 0;

				// NOCOM(GunChleoc): Use the new init.lua in tribes/scripting
				const std::string path = "tribes/scripting/starting_conditions/";
					// Read initializations -- all scripts are initializations currently
				for (const std::string& script :
					  filter(g_fs->list_directory(path + tribename),
								[](const string& fn) {return boost::ends_with(fn, ".lua");})) {
					std::unique_ptr<LuaTable> t = lua.run_script(script);
					t->do_not_warn_about_unaccessed_keys();

					info->initializations.push_back(
								TribeBasicInfo::Initialization(script, t->get_string("name")));
				}
			} catch (const WException & e) {
				throw GameDataError
					("reading basic info for tribe \"%s\": %s",
					 tribename.c_str(), e.what());
			}
			return true;
		}
	} catch (LuaError& e) {
		throw GameDataError("Unable to verify if tribe exists from '%s': %s", initfile.c_str(), e.what());
	}
	return false;
}

struct TribeBasicComparator {
	bool operator()(const TribeBasicInfo & t1, const TribeBasicInfo & t2) {
		return t1.uiposition < t2.uiposition;
	}
};

/**
 * Fills the given string vector with the names of all tribes that exist.
 */
// NOCOM(GunChleoc) this stuff really belongs in Tribes, but it is used by manually_load_tribes.
// Need to have a look at the control flow.
std::vector<std::string> TribeDescr::get_all_tribenames() {
	std::vector<std::string> tribenames;

	//  get all tribes
	std::vector<TribeBasicInfo> tribes;
	FilenameSet m_tribes = g_fs->list_directory("tribes");
	for
		(FilenameSet::iterator pname = m_tribes.begin();
		 pname != m_tribes.end();
		 ++pname)
	{
		TribeBasicInfo info;
		if (TribeDescr::exists_tribe(pname->substr(7), &info))
			tribes.push_back(info);
	}

	std::sort(tribes.begin(), tribes.end(), TribeBasicComparator());
	for (const TribeBasicInfo& tribe : tribes) {
		tribenames.push_back(tribe.name);
	}
	return tribenames;
}

// NOCOM(GunChleoc) this stuff really belongs in Tribes, but it is used by manually_load_tribes.
// Need to have a look at the control flow.
std::vector<TribeBasicInfo> TribeDescr::get_all_tribe_infos() {
	std::vector<TribeBasicInfo> tribes;

	//  get all tribes
	FilenameSet m_tribes = g_fs->list_directory("tribes");
	for
		(FilenameSet::iterator pname = m_tribes.begin();
		 pname != m_tribes.end();
		 ++pname)
	{
		TribeBasicInfo info;
		if (TribeDescr::exists_tribe(pname->substr(7), &info))
			tribes.push_back(info);
	}

	std::sort(tribes.begin(), tribes.end(), TribeBasicComparator());
	return tribes;
}


/**
  * Access functions
  */

const std::string& TribeDescr::name() const {return name_;}

size_t TribeDescr::get_nrbuildings() const {return buildings_.size();}
size_t TribeDescr::get_nrwares() const {return wares_.size();}
size_t TribeDescr::get_nrworkers() const {return workers_.size();}

const std::set<BuildingIndex> TribeDescr::buildings() const {return buildings_;}
const std::set<WareIndex> TribeDescr::wares() const {return wares_;}
const std::set<WareIndex> TribeDescr::workers() const {return workers_;}

bool TribeDescr::has_building(const BuildingIndex& index) const {
	return buildings_.count(index) == 1;
}
bool TribeDescr::has_ware(const WareIndex& index) const {
	return wares_.count(index) == 1;
}
bool TribeDescr::has_worker(const WareIndex& index) const {
	return workers_.count(index) == 1;
}
bool TribeDescr::is_construction_material(const WareIndex& index) const {
	return construction_materials_.count(index) == 1;
}

BuildingIndex TribeDescr::building_index(const std::string & buildingname) const {
	return egbase_.tribes().building_index(buildingname);
}

int TribeDescr::immovable_index(const std::string & immovablename) const {
	return egbase_.tribes().immovable_index(immovablename);
}
WareIndex TribeDescr::ware_index(const std::string & warename) const {
	return egbase_.tribes().ware_index(warename);
}
WareIndex TribeDescr::worker_index(const std::string & workername) const {
	return egbase_.tribes().worker_index(workername);
}

BuildingIndex TribeDescr::safe_building_index(const std::string& buildingname) const {
	return egbase_.tribes().safe_building_index(buildingname);
}

WareIndex TribeDescr::safe_ware_index(const std::string & warename) const {
	return egbase_.tribes().safe_ware_index(warename);
}
WareIndex TribeDescr::safe_worker_index(const std::string& workername) const {
	return egbase_.tribes().safe_worker_index(workername);
}

WareDescr const * TribeDescr::get_ware_descr(const WareIndex& index) const {
	return egbase_.tribes().get_ware_descr(index);
}
WorkerDescr const* TribeDescr::get_worker_descr(const WareIndex& index) const {
	return egbase_.tribes().get_worker_descr(index);
}

BuildingDescr const * TribeDescr::get_building_descr(const BuildingIndex& index) const {
	return egbase_.tribes().get_building_descr(index);
}
ImmovableDescr const * TribeDescr::get_immovable_descr(int index) const {
	return egbase_.tribes().get_immovable_descr(index);
}

WareIndex TribeDescr::builder() const {
	assert(egbase_.tribes().worker_exists(builder_));
	return builder_;
}
WareIndex TribeDescr::carrier() const {
	assert(egbase_.tribes().worker_exists(carrier_));
	return carrier_;
}
WareIndex TribeDescr::carrier2() const {
	assert(egbase_.tribes().worker_exists(carrier2_));
	return carrier2_;
}
WareIndex TribeDescr::geologist() const {
	assert(egbase_.tribes().worker_exists(geologist_));
	return geologist_;
}
WareIndex TribeDescr::soldier() const {
	assert(egbase_.tribes().worker_exists(soldier_));
	return soldier_;
}
int TribeDescr::ship() const {
	assert(egbase_.tribes().ship_exists(ship_));
	return ship_;
}
const std::vector<WareIndex>& TribeDescr::worker_types_without_cost() const {
	return worker_types_without_cost_;
}

uint32_t TribeDescr::frontier_animation() const {
	return m_frontier_animation_id;
}

uint32_t TribeDescr::flag_animation() const {
	return m_flag_animation_id;
}

/*
==============
Find the best matching indicator for the given amount.
==============
*/
uint32_t TribeDescr::get_resource_indicator
	(ResourceDescription const * const res, uint32_t const amount) const
{
	if (!res || !amount) {
		int32_t idx = immovable_index("resi_none");
		if (idx == -1)
			throw GameDataError
				("tribe %s does not declare a resource indicator resi_none!",
				 name().c_str());
		return idx;
	}

	int32_t i = 1;
	int32_t num_indicators = 0;
	for (;;) {
		const std::string resi_filename = (boost::format("resi_%s%i") % res->name().c_str() % i).str();
		if (immovable_index(resi_filename) == -1)
			break;
		++i;
		++num_indicators;
	}

	if (!num_indicators)
		throw GameDataError
			("tribe %s does not declare a resource indicator for resource %s",
			 name().c_str(),
			 res->name().c_str());

	int32_t bestmatch =
		static_cast<int32_t>
			((static_cast<float>(amount) / res->max_amount())
			 *
			 num_indicators);
	if (bestmatch > num_indicators)
		throw GameDataError
			("Amount of %s is %i but max amount is %i",
			 res->name().c_str(),
			 amount,
			 res->max_amount());
	if (static_cast<int32_t>(amount) < res->max_amount())
		bestmatch += 1; // Resi start with 1, not 0

	return immovable_index((boost::format("resi_%s%i")
										 % res->name().c_str()
										 % bestmatch).str());
}


void TribeDescr::resize_ware_orders(size_t maxLength) {
	bool need_resize = false;

	//check if we actually need to resize
	for (WaresOrder::iterator it = wares_order_.begin(); it != wares_order_.end(); ++it) {
		if (it->size() > maxLength) {
			need_resize = true;
		  }
	 }

	//resize
	if (need_resize) {

		//build new smaller wares_order
		WaresOrder new_wares_order;
		for (WaresOrder::iterator it = wares_order_.begin(); it != wares_order_.end(); ++it) {
			new_wares_order.push_back(std::vector<Widelands::WareIndex>());
			for (std::vector<Widelands::WareIndex>::iterator it2 = it->begin(); it2 != it->end(); ++it2) {
				if (new_wares_order.rbegin()->size() >= maxLength) {
					new_wares_order.push_back(std::vector<Widelands::WareIndex>());
				}
				new_wares_order.rbegin()->push_back(*it2);
				wares_order_coords_[*it2].first = new_wares_order.size() - 1;
				wares_order_coords_[*it2].second = new_wares_order.rbegin()->size() - 1;
			}
		}

		//remove old array
		wares_order_.clear();
		wares_order_ = new_wares_order;
	}
}

void TribeDescr::add_building(const std::string& buildingname) {
	try {
		BuildingIndex index = egbase_.tribes().safe_building_index(buildingname);
		if (buildings_.count(index) == 1) {
			throw new GameDataError("Duplicate definition of building '%s'", buildingname.c_str());
		}
		buildings_.insert(index);

		// Register construction materials
		for (std::pair<WareIndex, uint8_t> build_cost : get_building_descr(index)->buildcost()) {
			if (!is_construction_material(build_cost.first)) {
				construction_materials_.emplace(build_cost.first);
			}
		}
		for (std::pair<WareIndex, uint8_t> enhancement_cost :
			  get_building_descr(index)->enhancement_cost()) {
			if (!is_construction_material(enhancement_cost.first)) {
				construction_materials_.emplace(enhancement_cost.first);
			}
		}

	} catch (const WException& e) {
		throw new GameDataError("Failed adding building '%s' to tribe '%s': %s",
										buildingname.c_str(), name_.c_str(), e.what());
	}
}

WareIndex TribeDescr::add_special_worker(const std::string& workername) {
	WareIndex worker;
	try {
		worker = egbase_.tribes().safe_worker_index(workername);
	} catch (const WException& e) {
		throw new GameDataError("Failed adding special worker '%s' to tribe '%s': %s",
										workername.c_str(), name_.c_str(), e.what());
	}
	return worker;
}

}
