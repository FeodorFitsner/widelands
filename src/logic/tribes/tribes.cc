/*
 * Copyright (C) 2006-2014 by the Widelands Development Team
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

#include "logic/tribes/tribes.h"

#include "logic/game_data_error.h"

namespace Widelands {

Tribes::Tribes(EditorGameBase& egbase) :
	egbase_(egbase),
	buildings_(new DescriptionMaintainer<BuildingDescr>()),
	immovables_(new DescriptionMaintainer<ImmovableDescr>()),
	ships_(new DescriptionMaintainer<ShipDescr>()),
	wares_(new DescriptionMaintainer<WareDescr>()),
	workers_(new DescriptionMaintainer<WorkerDescr>()),
	tribes_(new DescriptionMaintainer<TribeDescr>()) {
}

void Tribes::add_constructionsite_type(const LuaTable& t) {
	buildings_->add(new ConstructionSiteDescr(t, egbase_));
}

void Tribes::add_dismantlesite_type(const LuaTable& t) {
	buildings_->add(new DismantleSiteDescr(t, egbase_));
}

void Tribes::add_militarysite_type(const LuaTable& t) {
	buildings_->add(new MilitarySiteDescr(t, egbase_));
}

void Tribes::add_productionsite_type(const LuaTable& t) {
	buildings_->add(new ProductionSiteDescr(t, egbase_));
}

void Tribes::add_trainingsite_type(const LuaTable& t) {
	buildings_->add(new TrainingSiteDescr(t, egbase_));
}

void Tribes::add_warehouse_type(const LuaTable& t) {
	buildings_->add(new WarehouseDescr(t, egbase_));
}

void Tribes::add_immovable_type(const LuaTable& t) {
	immovables_->add(new ImmovableDescr(t, egbase_.world(), MapObjectDescr::OwnerType::kTribe));
}

void Tribes::add_ship_type(const LuaTable& t) {
	ships_->add(new ShipDescr(t));
}

void Tribes::add_ware_type(const LuaTable& t) {
	wares_->add(new WareDescr(t));
}

void Tribes::add_carrier_type(const LuaTable& t) {
	workers_->add(new CarrierDescr(t, egbase_));
}

void Tribes::add_soldier_type(const LuaTable& t) {
	workers_->add(new SoldierDescr(t, egbase_));
}

void Tribes::add_worker_type(const LuaTable& t) {
	workers_->add(new WorkerDescr(t, egbase_));
}

void Tribes::add_tribe(const LuaTable& t) {
	tribes_->add(new TribeDescr(t, egbase_));
}

size_t Tribes::nrbuildings() const {
	return buildings_->size();
}

size_t Tribes::nrtribes() const {
	return tribes_->size();
}

size_t Tribes::nrwares() const {
	return wares_->size();
}

size_t Tribes::nrworkers() const {
	return workers_->size();
}


bool Tribes::ware_exists(const WareIndex& index) const {
	return wares_->get(index) != nullptr;
}
bool Tribes::worker_exists(const WareIndex& index) const {
	return workers_->get(index) != nullptr;
}
bool Tribes::building_exists(const std::string& buildingname) const {
	return buildings_->exists(buildingname) != nullptr;
}
bool Tribes::building_exists(const BuildingIndex& index) const {
	return buildings_->get(index) != nullptr;
}
bool Tribes::immovable_exists(int index) const {
	return immovables_->get(index) != nullptr;
}
bool Tribes::ship_exists(int index) const {
	return ships_->get(index) != nullptr;
}

BuildingIndex Tribes::safe_building_index(const std::string& buildingname) const {
	const BuildingIndex result = building_index(buildingname);
	if (result == INVALID_INDEX) {
		throw GameDataError("Unknown building type \"%s\"", buildingname.c_str());
	}
	return result;
}

int Tribes::safe_immovable_index(const std::string& immovablename) const {
	const int result = immovable_index(immovablename);
	if (result == -1) {
		throw GameDataError("Unknown immovable type \"%s\"", immovablename.c_str());
	}
	return result;
}

int Tribes::safe_ship_index(const std::string& shipname) const {
	const int result = ship_index(shipname);
	if (result == -1) {
		throw GameDataError("Unknown ship type \"%s\"", shipname.c_str());
	}
	return result;
}

int Tribes::safe_tribe_index(const std::string& tribename) const {
	const int result = tribe_index(tribename);
	if (result == -1) {
		throw GameDataError("Unknown tribe \"%s\"", tribename.c_str());
	}
	return result;
}

WareIndex Tribes::safe_ware_index(const std::string& warename) const {
	const WareIndex result = ware_index(warename);
	if (result == -1) {
		throw GameDataError("Unknown ware type \"%s\"", warename.c_str());
	}
	return result;
}

WareIndex Tribes::safe_worker_index(const std::string& workername) const {
	const WareIndex result = worker_index(workername);
	if (result == -1) {
		throw GameDataError("Unknown worker type \"%s\"", workername.c_str());
	}
	return result;
}


BuildingIndex Tribes::building_index(const std::string& buildingname) const {
	// NOCOM(GunChleoc): We have a mix of data types here (BuildingIndex is unsigned, WareIndex is signed).
	BuildingIndex result = INVALID_INDEX;
	int32_t index = buildings_->get_index(buildingname);
	if (index != -1) {
		result = static_cast<BuildingIndex>(index);
	}
	return result;
}

int Tribes::immovable_index(const std::string& immovablename) const {
	return immovables_->get_index(immovablename);
}

int Tribes::ship_index(const std::string& shipname) const {
	return ships_->get_index(shipname);
}

int Tribes::tribe_index(const std::string& tribename) const {
	return tribes_->get_index(tribename);
}


WareIndex Tribes::ware_index(const std::string& warename) const {
	return wares_->get_index(warename);
}

WareIndex Tribes::worker_index(const std::string& workername) const {
	return workers_->get_index(workername);
}


const BuildingDescr* Tribes::get_building_descr(BuildingIndex buildingindex) const {
	return buildings_->get(buildingindex);
}

const ImmovableDescr* Tribes::get_immovable_descr(int immovableindex) const {
	return immovables_->get(immovableindex);
}

const ShipDescr* Tribes::get_ship_descr(int shipindex) const {
	return ships_->get(shipindex);
}


const WareDescr* Tribes::get_ware_descr(WareIndex wareindex) const {
	return wares_->get(wareindex);
}

const WorkerDescr* Tribes::get_worker_descr(WareIndex workerindex) const {
	return workers_->get(workerindex);
}

const TribeDescr* Tribes::get_tribe_descr(int tribeindex) const {
	return tribes_->get(tribeindex);
}

void Tribes::set_ware_type_has_demand_check(const WareIndex& wareindex, const std::string& tribename) {
	wares_->get(wareindex)->set_has_demand_check(tribename);
}

void Tribes::set_worker_type_has_demand_check(const WareIndex& workerindex) {
	workers_->get(workerindex)->set_has_demand_check();
}


void Tribes::load_graphics()
{
	for (WareIndex i = 0; i < static_cast<WareIndex>(workers_->get_nitems()); ++i) {
		workers_->get(i)->load_graphics();
	}

	for (WareIndex i = 0; i < static_cast<WareIndex>(wares_->get_nitems()); ++i) {
		wares_->get(i)->load_graphics();
	}

	for (BuildingIndex i = 0; i < buildings_->get_nitems(); ++i) {
		buildings_->get(i)->load_graphics();
	}
}

void Tribes::post_load() {
	for (BuildingIndex i = 0; i < buildings_->get_nitems(); ++i) {
		BuildingDescr& building_descr = *buildings_->get(i);
		// NOCOM(GunChleoc): parse buildcost for buildings

		// Add consumers and producers to wares.
		if (upcast(ProductionSiteDescr, de, &building_descr)) {
			for (const WareAmount& ware_amount : de->inputs()) {
				wares_->get(ware_amount.first)->add_consumer(i);
			}
			for (const WareIndex& wareindex : de->output_ware_types()) {
				wares_->get(wareindex)->add_producer(i);
			}
		}
		// Register which buildings buildings can have been enhanced from
		const BuildingIndex& enhancement = building_descr.enhancement();
		if (building_exists(enhancement)) {
			buildings_->get(enhancement)->set_enhanced_from(i);
		}
	}
}

} // namespace Widelands
