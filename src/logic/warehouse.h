/*
 * Copyright (C) 2002-2004, 2006-2010 by the Widelands Development Team
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

#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include "attackable.h"
#include "building.h"

struct EncodeData;
struct Interactive_Player;
struct Profile;

namespace Widelands {

struct Editor_Game_Base;
struct Request;
struct Requirements;
class Soldier;
struct Tribe_Descr;
class WareInstance;
struct WareList;


/*
Warehouse
*/
struct WarehouseSupply;

struct Warehouse_Descr : public Building_Descr {
	Warehouse_Descr
		(char const * name, char const * descname,
		 std::string const & directory, Profile &, Section & global_s,
		 Tribe_Descr const &, EncodeData const *);

	virtual Building & create_object() const;

	virtual uint32_t get_conquers() const {return m_conquers;}

private:
	int32_t m_conquers;
};


class Warehouse : public Building, public Attackable {
	friend struct Map_Buildingdata_Data_Packet;

	MO_DESCR(Warehouse_Descr);

public:
	Warehouse(const Warehouse_Descr &);
	virtual ~Warehouse();

	void load_finish(Editor_Game_Base &);

	void prefill
		(Game &, uint32_t const *, uint32_t const *, Soldier_Counts const *);
	void postfill
		(Game &, uint32_t const *, uint32_t const *, Soldier_Counts const *);

	char const * type_name() const throw () {return "warehouse";}

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
	/// * Sets the size of m_target_supplys to the sum of all ware and worker
	///   types.
	/// * Sees the area (since a warehouse is considered to be always occupied).
	/// * Conquers land if the the warehouse type is configured to do that.
	/// * Sends a message to the player about the creation of this warehouse.
	virtual void init(Editor_Game_Base &);

	virtual void cleanup(Editor_Game_Base &);

	virtual void act(Game & game, uint32_t data);

	virtual void set_economy(Economy *);
	virtual int32_t get_priority
		(int32_t type, Ware_Index ware_index, bool adjust = true) const;
	void set_needed(Ware_Index, uint32_t value = 1);

	WareList const & get_wares() const;
	WareList const & get_workers() const;
	void insert_wares  (Ware_Index, uint32_t count);
	void remove_wares  (Ware_Index, uint32_t count);
	void insert_workers(Ware_Index, uint32_t count);
	void remove_workers(Ware_Index, uint32_t count);

	virtual bool fetch_from_flag(Game &);

	uint32_t count_workers(Game const &, Ware_Index, Requirements const &);
	Worker & launch_worker(Game &, Ware_Index, Requirements const &);
	void incorporate_worker(Game &, Worker &);

	WareInstance & launch_item(Game &, Ware_Index);
	void do_launch_item(Game &, WareInstance &);
	void incorporate_item(Game &, WareInstance &);

	bool can_create_worker(Game &, Ware_Index) const;
	void     create_worker(Game &, Ware_Index);

	void enable_spawn(Game &, uint8_t worker_types_without_cost_index);
	void disable_spawn(uint8_t worker_types_without_cost_index);

	// Begin Attackable implementation
	virtual bool canAttack();
	virtual void aggressor(Soldier &);
	virtual bool attack   (Soldier &);
	// End Attackable implementation

protected:

	/// Create the warehouse information window.
	virtual void create_options_window
		(Interactive_GameBase &, UI::Window * & registry);

private:
	static void idle_request_cb
		(Game &, Request &, Ware_Index, Worker *, PlayerImmovable &);
	void sort_worker_in(Editor_Game_Base &, Worker &);

	WarehouseSupply       * m_supply;
	std::vector<Request *>  m_requests; // one idle request per ware type

	// Workers who live here at the moment
	std::vector<OPtr<Worker> > m_incorporated_workers;
	uint32_t                 * m_next_worker_without_cost_spawn;
	uint32_t                   m_next_military_act;

	/// how many do we want to store in this building
	std::vector<size_t> m_target_supply; // absolute value
};

}

#endif
