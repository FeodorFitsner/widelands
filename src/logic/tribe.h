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
 *
 */

#ifndef WL_LOGIC_TRIBE_H
#define WL_LOGIC_TRIBE_H

#include <map>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "graphic/animation.h"
#include "logic/building.h"
#include "logic/description_maintainer.h"
#include "logic/editor_game_base.h"
#include "logic/immovable.h"
#include "logic/ship.h"
#include "logic/tribe_basic_info.h"
#include "logic/tribes/tribes.h"
#include "logic/ware_descr.h"
#include "logic/worker.h"

namespace Widelands {

class EditorGameBase;
class ResourceDescription;
class WareDescr;
class Warehouse;
class WorkerDescr;
class World;
struct BuildingDescr;
struct Event;

/*
Tribes
------

Every player chooses a tribe. A tribe has distinct properties such as the
buildings it can build and the associated graphics.
Two players can choose the same tribe.
*/
class TribeDescr {
public:
	TribeDescr(const LuaTable& table, EditorGameBase& egbase);

	//  Static function to check for tribes.
	static bool exists_tribe
		(const std::string & name, TribeBasicInfo * info = nullptr);
	static std::vector<std::string> get_all_tribenames();
	static std::vector<TribeBasicInfo> get_all_tribe_infos();


	const std::string& name() const;

	// NOCOM(GunChleoc): Look at the usage, ranged-bases for loops now?
	size_t get_nrbuildings() const;
	size_t get_nrwares() const;
	size_t get_nrworkers() const;

	const std::set<BuildingIndex> buildings() const;
	const std::set<WareIndex> wares() const;
	const std::set<WareIndex> workers() const;

	bool has_building(const BuildingIndex& index) const;
	bool has_ware(const WareIndex& index) const;
	bool has_worker(const WareIndex& index) const;
	bool is_construction_material(const WareIndex& ware_index) const;

	BuildingIndex building_index(const std::string & buildingname) const;
	int immovable_index(const std::string & immovablename) const;
	WareIndex ware_index(const std::string & warename) const;
	WareIndex worker_index(const std::string & workername) const;

	/// Return the given building or die trying
	BuildingIndex safe_building_index(const std::string& buildingname) const;
	/// Return the given ware or die trying
	WareIndex safe_ware_index(const std::string & warename) const;
	/// Return the given worker or die trying
	WareIndex safe_worker_index(const std::string & workername) const;

	BuildingDescr const * get_building_descr(const BuildingIndex& index) const;
	ImmovableDescr const * get_immovable_descr(int index) const;
	WareDescr const * get_ware_descr(const WareIndex& index) const;
	WorkerDescr const * get_worker_descr(const WareIndex& index) const;

	WareIndex builder() const;
	WareIndex carrier() const;
	WareIndex carrier2() const;
	WareIndex soldier() const;
	int ship() const;
	const std::vector<WareIndex>& worker_types_without_cost() const;

	uint32_t frontier_animation() const;
	uint32_t flag_animation() const;

	uint32_t get_resource_indicator
		(const ResourceDescription * const res, const uint32_t amount) const;

	struct Initialization {
		std::string script;
		std::string descname;
	};

	// Returns the initalization at 'index' (which must not be out of bounds).
	const Initialization& initialization(const uint8_t index) const {
		return m_initializations.at(index);
	}

	using WaresOrder = std::vector<std::vector<Widelands::WareIndex>>;
	using WaresOrderCoords = std::vector<std::pair<uint32_t, uint32_t>>;
	const WaresOrder & wares_order() const {return wares_order_;}
	const WaresOrderCoords & wares_order_coords() const {
		return wares_order_coords_;
	}

	const WaresOrder & workers_order() const {return workers_order_;}
	const WaresOrderCoords & workers_order_coords() const {
		return workers_order_coords_;
	}

	void resize_ware_orders(size_t maxLength);

private:
	// Helper function for adding a building type
	void add_building(const std::string& buildingname);
	WareIndex add_special_worker(const std::string& workername);

	const std::string name_;
	EditorGameBase& egbase_;

	uint32_t m_frontier_animation_id;
	uint32_t m_flag_animation_id;

	std::set<BuildingIndex>           buildings_;
	std::set<int>                     immovables_;  // The player immovables
	std::set<WareIndex>               workers_;
	std::set<WareIndex>               wares_;
	std::set<WareIndex>               construction_materials_; // The wares that are used by construction sites
	// Special units
	WareIndex                         builder_;  // The builder for this tribe
	WareIndex                         carrier_;  // The basic carrier for this tribe
	WareIndex                         carrier2_; // Additional carrier for busy roads
	WareIndex                         soldier_;  // The soldier that this tribe uses
	int                               ship_;     // The ship that this tribe uses
	std::vector<WareIndex>            worker_types_without_cost_;
	// Order and positioning of wares in the warehouse display
	WaresOrder                        wares_order_;
	WaresOrderCoords                  wares_order_coords_;
	WaresOrder                        workers_order_;
	WaresOrderCoords                  workers_order_coords_;

	std::vector<Initialization> m_initializations;

	DISALLOW_COPY_AND_ASSIGN(TribeDescr);
};

}

#endif  // end of include guard: WL_LOGIC_TRIBE_H
