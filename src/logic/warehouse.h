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

#ifndef WL_LOGIC_WAREHOUSE_H
#define WL_LOGIC_WAREHOUSE_H

#include "base/macros.h"
#include "base/wexception.h"
#include "economy/request.h"
#include "logic/attackable.h"
#include "logic/building.h"
#include "logic/soldiercontrol.h"
#include "logic/wareworker.h"

class InteractivePlayer;
class Profile;

namespace Widelands {

class EditorGameBase;
class PortDock;
class Request;
struct Requirements;
class Soldier;
struct TribeDescr;
class WareInstance;
struct WareList;


/*
Warehouse
*/
struct WarehouseSupply;

struct WarehouseDescr : public BuildingDescr {
	WarehouseDescr
		(char const * name, char const * descname,
		 const std::string & directory, Profile &, Section & global_s,
		 const TribeDescr &);

	WarehouseDescr(const LuaTable& t);

	~WarehouseDescr() override {}

	Building & create_object() const override;

	uint32_t get_conquers() const override {return m_conquers;}

	uint32_t get_heal_per_second        () const {
		return m_heal_per_second;
	}

private:
	int32_t m_conquers;
	uint32_t m_heal_per_second;
	DISALLOW_COPY_AND_ASSIGN(WarehouseDescr);
};


class Warehouse : public Building, public Attackable, public SoldierControl {
	friend class PortDock;
	friend class MapBuildingdataPacket;

	MO_DESCR(WarehouseDescr)

public:
	/**
	 * Each ware and worker type has an associated per-warehouse
	 * stock policy that defines whether it will be stocked by this
	 * warehouse.
	 *
	 * \note The values of this enum are written directly into savegames,
	 * so be careful when changing them.
	 */
	enum StockPolicy {
		/**
		 * The default policy allows stocking wares without any special priority.
		 */
		SP_Normal = 0,

		/**
		 * As long as there are warehouses with this policy for a ware, all
		 * available unstocked supplies will be transferred to warehouses
		 * with this policy.
		 */
		SP_Prefer = 1,

		/**
		 * If a ware has this stock policy, no more of this ware will enter
		 * the warehouse.
		 */
		SP_DontStock = 2,

		/**
		 * Like \ref SP_DontStock, but in addition, existing stock of this ware
		 * will be transported out of the warehouse over time.
		 */
		SP_Remove = 3,
	};

	Warehouse(const WarehouseDescr &);
	virtual ~Warehouse();

	void load_finish(EditorGameBase &) override;

	/// Called only when the oject is logically created in the simulation. If
	/// called again, such as when the object is loaded from a savegame, it will
	/// cause bugs.
	///
	/// * Calls Building::init.
	/// * Creates an idle_request for each ware and worker type.
	/// * Sets a next_spawn time for each buildable worker type without cost
	///   that the owning player is allowed to create and schedules act for for
	///   the spawn.
	/// * Schedules act for military stuff (and sets m_next_military_act).
	/// * Sees the area (since a warehouse is considered to be always occupied).
	/// * Conquers land if the the warehouse type is configured to do that.
	/// * Sends a message to the player about the creation of this warehouse.
	/// * Sets up @ref PortDock for ports
	void init(EditorGameBase &) override;

	void cleanup(EditorGameBase &) override;

	void destroy(EditorGameBase &) override;

	void act(Game & game, uint32_t data) override;

	void set_economy(Economy *) override;

	const WareList & get_wares() const;
	const WareList & get_workers() const;

	/**
	 * Returns a vector of all incorporated workers. These are the workers
	 * that are still present in the game, not just a stock figure.
	 */
	Workers get_incorporated_workers();

	void insert_wares  (WareIndex, uint32_t count);
	void remove_wares  (WareIndex, uint32_t count);
	void insert_workers(WareIndex, uint32_t count);
	void remove_workers(WareIndex, uint32_t count);

	/* SoldierControl implementation */
	std::vector<Soldier *> present_soldiers() const override;
	std::vector<Soldier *> stationed_soldiers() const override {
		return present_soldiers();
	}
	uint32_t min_soldier_capacity() const override {return 0;}
	uint32_t max_soldier_capacity() const override {return 4294967295U;}
	uint32_t soldier_capacity() const override {return max_soldier_capacity();}
	void set_soldier_capacity(uint32_t /* capacity */) override {
		throw wexception("Not implemented for a Warehouse!");
	}
	void drop_soldier(Soldier &) override {
		throw wexception("Not implemented for a Warehouse!");
	}
	int outcorporate_soldier(EditorGameBase &, Soldier &) override;
	int incorporate_soldier(EditorGameBase &, Soldier& soldier) override;

	bool fetch_from_flag(Game &) override;

	uint32_t count_workers(const Game &, WareIndex, const Requirements &);
	Worker & launch_worker(Game &, WareIndex, const Requirements &);

	// Adds the worker to the inventory. Takes ownership and might delete
	// 'worker'.
	void incorporate_worker(EditorGameBase&, Worker* worker);

	WareInstance & launch_ware(Game &, WareIndex);
	void do_launch_ware(Game &, WareInstance &);

	// Adds the ware to our inventory. Takes ownership and might delete 'ware'.
	void incorporate_ware(EditorGameBase&, WareInstance* ware);

	bool can_create_worker(Game &, WareIndex) const;
	void     create_worker(Game &, WareIndex);

	uint32_t get_planned_workers(Game &, WareIndex index) const;
	void plan_workers(Game &, WareIndex index, uint32_t amount);
	std::vector<uint32_t> calc_available_for_worker
		(Game &, WareIndex index) const;

	void enable_spawn(Game &, uint8_t worker_types_without_cost_index);
	void disable_spawn(uint8_t worker_types_without_cost_index);

	// Begin Attackable implementation
	Player & owner() const override {return Building::owner();}
	bool can_attack() override;
	void aggressor(Soldier &) override;
	bool attack   (Soldier &) override;
	// End Attackable implementation

	void receive_ware(Game &, WareIndex ware) override;
	void receive_worker(Game &, Worker & worker) override;

	StockPolicy get_ware_policy(WareIndex ware) const;
	StockPolicy get_worker_policy(WareIndex ware) const;
	StockPolicy get_stock_policy(WareWorker waretype, WareIndex wareindex) const;
	void set_ware_policy(WareIndex ware, StockPolicy policy);
	void set_worker_policy(WareIndex ware, StockPolicy policy);

	// Get the portdock if this is a port.
	PortDock * get_portdock() const {return m_portdock;}

	// Returns the waresqueue of the expedition if this is a port. Will
	// assert(false) otherwise.
	WaresQueue& waresqueue(WareIndex) override;

	void log_general_info(const EditorGameBase &) override;

protected:
	/// Create the warehouse information window.
	virtual void create_options_window
		(InteractiveGameBase &, UI::Window * & registry) override;

private:
	void init_portdock(EditorGameBase & egbase);

	/**
	 * Plan to produce a certain worker type in this warehouse. This means
	 * requesting all the necessary wares, if multiple different wares types are
	 * needed.
	 */
	struct PlannedWorkers {
		/// Index of the worker type we plan to create
		WareIndex index;

		/// How many workers of this type are we supposed to create?
		uint32_t amount;

		/// Requests to obtain the required build costs
		std::vector<Request *> requests;

		void cleanup();
	};

	static void request_cb
		(Game &, Request &, WareIndex, Worker *, PlayerImmovable &);
	void check_remove_stock(Game &);

	bool _load_finish_planned_worker(PlannedWorkers & pw);
	void _update_planned_workers(Game &, PlannedWorkers & pw);
	void _update_all_planned_workers(Game &);

	WarehouseSupply       * m_supply;

	std::vector<StockPolicy> m_ware_policy;
	std::vector<StockPolicy> m_worker_policy;

	// Workers who live here at the moment
	using WorkerList = std::vector<Worker *>;
	using IncorporatedWorkers = std::map<WareIndex, WorkerList>;
	IncorporatedWorkers        m_incorporated_workers;
	uint32_t                 * m_next_worker_without_cost_spawn;
	uint32_t                   m_next_military_act;
	uint32_t m_next_stock_remove_act;

	std::vector<PlannedWorkers> m_planned_workers;

	PortDock * m_portdock;
};

}

#endif  // end of include guard: WL_LOGIC_WAREHOUSE_H
