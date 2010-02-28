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

#include "widelands_map_buildingdata_data_packet.h"

#include "logic/constructionsite.h"
#include "economy/flag.h"
#include "economy/request.h"
#include "economy/wares_queue.h"
#include "logic/editor_game_base.h"
#include "logic/game.h"
#include "logic/map.h"
#include "logic/militarysite.h"
#include "logic/player.h"
#include "logic/production_program.h"
#include "logic/productionsite.h"
#include "logic/soldier.h"
#include "logic/trainingsite.h"
#include "logic/tribe.h"
#include "logic/warehouse.h"
#include "economy/warehousesupply.h"
#include "logic/widelands_fileread.h"
#include "logic/widelands_filewrite.h"
#include "widelands_map_map_object_loader.h"
#include "widelands_map_map_object_saver.h"
#include "logic/worker.h"

#include "upcast.h"

#include <map>

namespace Widelands {

// Versions
#define CURRENT_PACKET_VERSION 2

// Subversions
#define CURRENT_CONSTRUCTIONSITE_PACKET_VERSION 1
#define CURRENT_WAREHOUSE_PACKET_VERSION        2
#define CURRENT_MILITARYSITE_PACKET_VERSION     3
#define CURRENT_PRODUCTIONSITE_PACKET_VERSION   4
#define CURRENT_TRAININGSITE_PACKET_VERSION     3


void Map_Buildingdata_Data_Packet::Read
	(FileSystem            &       fs,
	 Editor_Game_Base      &       egbase,
	 bool                    const skip,
	 Map_Map_Object_Loader &       mol)
throw (_wexception)
{
	if (skip)
		return;

	FileRead fr;
	try {fr.Open(fs, "binary/building_data");} catch (...) {return;}

	try {
		uint16_t const packet_version = fr.Unsigned16();
		if (1 <= packet_version and packet_version <= CURRENT_PACKET_VERSION) {
			for (;;) {
				if (2 <= packet_version and fr.EndOfFile())
					break;
				Serial const serial = fr.Unsigned32();
				if (packet_version < 2 and serial == 0xffffffff) {
					if (not fr.EndOfFile())
						throw game_data_error
							("expected end of file after serial 0xffffffff");
					break;
				}
				try {
					Building & building = mol.get<Building>(serial);

					if (fr.Unsigned8()) {
						char const * const animation_name = fr.CString();
						try {
							building.m_anim =
								building.descr().get_animation(animation_name);
						} catch (Map_Object_Descr::Animation_Nonexistent const &) {
							log
								("WARNING: %s %s does not have animation \"%s\"; "
								 "using animation \"idle\" instead\n",
								 building.descr().tribe().name().c_str(),
								 building.descname().c_str(),
								 animation_name);
							building.m_anim = building.descr().get_animation("idle");
						}
					} else
						building.m_anim = 0;
					building.m_animstart = fr.Unsigned32();

					{
						Building::Leave_Queue & leave_queue = building.m_leave_queue;
						leave_queue.resize(fr.Unsigned16());
						container_iterate(Building::Leave_Queue, leave_queue, i)
							if (uint32_t const leaver_serial = fr.Unsigned32())
								try {
									//  The check that this worker actually has a
									//  leavebuilding task for this building is in
									//  Building::load_finish, which is called after the
									//  worker (with his stack of tasks) has been fully
									//  loaded.
									*i.current = &mol.get<Worker>(leaver_serial);
								} catch (_wexception const & e) {
									throw game_data_error
										("leave queue item #%lu (%u): %s",
										 static_cast<long int>
										 	(i.current - leave_queue.begin()),
										 leaver_serial, e.what());
								}
							else
								*i.current = 0;
					}

					building.m_leave_time = fr.Unsigned32();

					if (uint32_t const leaver_serial = fr.Unsigned32())
						try {
							building.m_leave_allow =
								&mol.get<Map_Object>(leaver_serial);
						} catch (_wexception const & e) {
							throw game_data_error
								("leave allow item (%u): %s", leaver_serial, e.what());
						}
					else
						building.m_leave_allow = 0;

					if (fr.Unsigned8()) {
						if (upcast(ProductionSite, productionsite, &building))
							if (dynamic_cast<MilitarySite const *>(productionsite))
								log
									("WARNING: Found a stopped %s at (%i, %i) in the "
									 "savegame. Militarysites are not stoppable. "
									 "Ignoring.",
									 building.descname().c_str(),
									 building.get_position().x,
									 building.get_position().y);
							else
								productionsite->set_stopped(true);
						else
							log
								("WARNING: Found a stopped %s at (%i, %i) in the "
								 "savegame. Only productionsites are stoppable. "
								 "Ignoring.",
								 building.descname().c_str(),
								 building.get_position().x,
								 building.get_position().y);
					}

					//  Set economy now, some stuff below will count on this.
					building.set_economy(building.m_flag->get_economy());

					if (upcast(ConstructionSite, constructionsite, &building))
						read_constructionsite
							(*constructionsite,
							 fr,
							 ref_cast<Game, Editor_Game_Base>(egbase),
							 mol);
					else if (upcast(Warehouse, warehouse, &building))
						read_warehouse
							(*warehouse,
							 fr,
							 ref_cast<Game, Editor_Game_Base>(egbase),
							 mol);
					else if (upcast(ProductionSite, productionsite, &building)) {
						if (upcast(MilitarySite, militarysite, productionsite))
							read_militarysite
								(*militarysite,
								 fr,
								 ref_cast<Game, Editor_Game_Base>(egbase),
								 mol);
						else if (upcast(TrainingSite, trainingsite, productionsite))
							read_trainingsite
								(*trainingsite,
								 fr,
								 ref_cast<Game, Editor_Game_Base>(egbase),
								 mol);
						else
							read_productionsite
								(*productionsite,
								 fr,
								 ref_cast<Game, Editor_Game_Base>(egbase),
								 mol);
					} else
						//  type of building is not one of (or derived from)
						//  {ConstructionSite, Warehouse, ProductionSite}
						assert(false);


					mol.mark_object_as_loaded(building);
				} catch (_wexception const & e) {
					throw game_data_error(_("building %u: %s"), serial, e.what());
				}
			}
		} else
			throw game_data_error
				(_("unknown/unhandled version %u"), packet_version);
	} catch (_wexception const & e) {
		throw game_data_error(_("buildingdata: %s"), e.what());
	}
}


void Map_Buildingdata_Data_Packet::read_constructionsite
	(ConstructionSite      & constructionsite,
	 FileRead              & fr,
	 Game                  & game,
	 Map_Map_Object_Loader & mol)
{
	try {
		uint16_t const packet_version = fr.Unsigned16();
		if (packet_version == CURRENT_CONSTRUCTIONSITE_PACKET_VERSION) {
			Tribe_Descr const & tribe = constructionsite.tribe();
			constructionsite.m_building =
				tribe.get_building_descr(tribe.safe_building_index(fr.CString()));
			if (fr.Unsigned8()) {
				constructionsite.m_prev_building =
					tribe.get_building_descr
						(tribe.safe_building_index(fr.CString()));
			} else
				constructionsite.m_prev_building = 0;

			delete constructionsite.m_builder_request;
			if (fr.Unsigned8()) {
				constructionsite.m_builder_request =
					new Request
						(constructionsite,
						 Ware_Index::First(),
						 ConstructionSite::request_builder_callback,
						 Request::WORKER);
				constructionsite.m_builder_request->Read(fr, game, mol);
			} else
				constructionsite.m_builder_request = 0;

			if (uint32_t const builder_serial = fr.Unsigned32()) {
				try {
					constructionsite.m_builder = &mol.get<Worker>(builder_serial);
				} catch (_wexception const & e) {
					throw game_data_error
						("builder (%u): %s", builder_serial, e.what());
				}
			} else
				constructionsite.m_builder = 0;

			try {
				uint16_t const size = fr.Unsigned16();
				if (constructionsite.m_wares.size() < size)
					throw game_data_error("constructionsite.m_wares.size() < size");
				for (uint16_t i = size; i < constructionsite.m_wares.size(); ++i) {
					constructionsite.m_wares[i]->cleanup();
					delete constructionsite.m_wares[i];
				}
				constructionsite.m_wares.resize(size);
				for (uint16_t i = 0; i < constructionsite.m_wares.size(); ++i)
					constructionsite.m_wares[i]->Read(fr, game, mol);
			} catch (_wexception const & e) {
				throw game_data_error(_("wares: %s"), e.what());
			}

			constructionsite.m_fetchfromflag  = fr.  Signed32();

			constructionsite.m_working        = fr.Unsigned8 ();
			constructionsite.m_work_steptime  = fr.Unsigned32();
			constructionsite.m_work_completed = fr.Unsigned32();
			constructionsite.m_work_steps     = fr.Unsigned32();
		} else
			throw game_data_error
				(_("unknown/unhandled version %u"), packet_version);
	} catch (_wexception const & e) {
		throw game_data_error(_("constructionsite: %s"), e.what());
	}
}


void Map_Buildingdata_Data_Packet::read_warehouse
	(Warehouse             & warehouse,
	 FileRead              & fr,
	 Game                  & game,
	 Map_Map_Object_Loader & mol)
{
	try {
		uint16_t const packet_version = fr.Unsigned16();
		if
			(1 <= packet_version and
			 packet_version <= CURRENT_WAREHOUSE_PACKET_VERSION)
		{
			log("Reading warehouse stuff for %p\n", &warehouse);
			//  supply
			Tribe_Descr const & tribe = warehouse.tribe();
			while (fr.Unsigned8()) {
				Ware_Index const id = tribe.safe_ware_index(fr.CString());
				warehouse.remove_wares(id, warehouse.m_supply->stock_wares(id));
				warehouse.insert_wares(id, fr.Unsigned16());
			}
			while (fr.Unsigned8()) {
				Ware_Index const id = tribe.safe_worker_index(fr.CString());
				warehouse.remove_workers
					(id, warehouse.m_supply->stock_workers(id));
				warehouse.insert_workers(id, fr.Unsigned16());
			}

			//  FIXME The reason for this code is that Warehouse::init is called
			//  FIXME when a warehouse is allocated during savegame loading. That
			//  FIXME is a bug. That code must only be executed when a warehouse
			//  FIXME is created in the game. Requests are created there.
			//  FIXME Therefore this code here undoes what Warehouse::init just
			//  FIXME did.
			container_iterate_const
				(std::vector<Request *>, warehouse.m_requests, i)
				delete *i.current;

			warehouse.m_requests.resize(fr.Unsigned16());
			for (uint32_t i = 0; i < warehouse.m_requests.size(); ++i) {
				Request & req =
					*new Request
						(warehouse,
						 Ware_Index::First(),
						 Warehouse::idle_request_cb,
						 Request::WORKER);
				req.Read(fr, game, mol);
				warehouse.m_requests[i] = &req;
			}

			assert(warehouse.m_incorporated_workers.empty());
			{
				uint16_t const nr_workers = fr.Unsigned16();
				for (uint16_t i = 0; i < nr_workers; ++i) {
					uint32_t const worker_serial = fr.Unsigned32();
					try {
						Worker & worker = mol.get<Worker>(worker_serial);
						if (1 == packet_version) {
							char const * const name = fr.CString();
							if (name != worker.name())
								throw game_data_error
									(_("expected %s but found \"%s\""),
									 worker.name().c_str(), name);
						}
						warehouse.sort_worker_in(game, worker);
					} catch (_wexception const & e) {
						throw game_data_error
							("incorporated worker #%u (%u): %s",
							 i, worker_serial, e.what());
					}
				}
			}

			std::vector<Ware_Index> const & worker_types_without_cost =
				tribe.worker_types_without_cost();

			//  FIXME Mighty ugly kludge to undo what Warehouse::init did, so that
			//  FIXME we have a fresh object when starting to fill in the values
			//  FIXME read from file. To get rid of this, make sure that init is
			//  FIXME not called on a Warehouse object that has been allocated
			//  FIXME during savegame loading.
			for (uint8_t i = worker_types_without_cost.size(); i;)
				warehouse.m_next_worker_without_cost_spawn[--i] = Never();

			if (1 == packet_version) { //  a single next_spawn time for "carrier"
				uint32_t const next_spawn = fr.Unsigned32();
				Ware_Index const worker_index =
					tribe.safe_worker_index("carrier");
				if (not worker_index) {
					log
						("WARNING: %s %u has a next_spawn time for nonexistent "
						 "worker type \"%s\" set to %u, ignoring\n",
						 warehouse.descname().c_str(), warehouse.serial(),
						 "carrier", next_spawn);
				} else if
					(tribe.get_worker_descr(worker_index)->buildcost().size())
				{
					log
						("WARNING: %s %u has a next_spawn time for worker type "
						 "\"%s\", that costs something to build, set to %u, "
						 "ignoring\n",
						 warehouse.descname().c_str(), warehouse.serial(),
						 "carrier", next_spawn);
				} else
					for (uint8_t i = 0;; ++i) {
						assert(i < worker_types_without_cost.size());
						if (worker_types_without_cost.at(i) == worker_index) {
							if
								(warehouse.m_next_worker_without_cost_spawn[i]
								 !=
								 Never())
							{
								warehouse.molog
									("read_warehouse: "
									 "m_next_worker_without_cost_spawn[%u] = %u\n",
									 i, warehouse.m_next_worker_without_cost_spawn[i]);
							}
							assert
								(warehouse.m_next_worker_without_cost_spawn[i]
								 ==
								 Never());
							warehouse.m_next_worker_without_cost_spawn[i] =
								next_spawn;
							break;
						}
					}
			} else
				for (;;) {
					char const * const worker_typename = fr.CString   ();
					if (not *worker_typename) //  encountered the terminator ("")
						break;
					uint32_t     const next_spawn      = fr.Unsigned32();
					Ware_Index   const worker_index    =
						tribe.safe_worker_index(worker_typename);
					if (not worker_index) {
						log
							("WARNING: %s %u has a next_spawn time for nonexistent "
							 "worker type \"%s\" set to %u, ignoring\n",
							 warehouse.descname().c_str(), warehouse.serial(),
							 worker_typename, next_spawn);
						continue;
					}
					if (tribe.get_worker_descr(worker_index)->buildcost().size()) {
						log
							("WARNING: %s %u has a next_spawn time for worker type "
							 "\"%s\", that costs something to build, set to %u, "
							 "ignoring\n",
							 warehouse.descname().c_str(), warehouse.serial(),
							 worker_typename, next_spawn);
						continue;
					}
					for (uint8_t i = 0;; ++i) {
						assert(i < worker_types_without_cost.size());
						if (worker_types_without_cost.at(i) == worker_index) {
							if
								(warehouse.m_next_worker_without_cost_spawn[i]
								 !=
								 Never())
								throw game_data_error
									(_
									 	("%s %u has a next_spawn time for worker type "
									 	 "\"%s\" set to %u, but it was previously set "
									 	 "to %u\n"),
									 warehouse.descname().c_str(), warehouse.serial(),
									 worker_typename, next_spawn,
									 warehouse.m_next_worker_without_cost_spawn[i]);
							warehouse.m_next_worker_without_cost_spawn[i] =
								next_spawn;
							break;
						}
					}
				}
				//  The checks that the warehouse has a next_spawn time for each
				//  worker type that the player is allowed to spawn, is in
				//  Warehouse::load_finish.

			log("Read warehouse stuff for %p\n", &warehouse);
		} else
			throw game_data_error
				(_("unknown/unhandled version %u"), packet_version);
	} catch (_wexception const & e) {
		throw game_data_error(_("warehouse: %s"), e.what());
	}
}


void Map_Buildingdata_Data_Packet::read_militarysite
	(MilitarySite          & militarysite,
	 FileRead              & fr,
	 Game                  & game,
	 Map_Map_Object_Loader & mol)
{
	try {
		uint16_t const packet_version = fr.Unsigned16();
		if
			(packet_version == CURRENT_MILITARYSITE_PACKET_VERSION ||
			 packet_version == 2)
		{
			read_productionsite(militarysite, fr, game, mol);

			if (packet_version >= 3) {
				delete militarysite.m_soldier_request;
				militarysite.m_soldier_request = 0;

				if (fr.Unsigned8()) {
					militarysite.m_soldier_request =
						new Request
							(militarysite,
							 Ware_Index::First(),
							 MilitarySite::request_soldier_callback,
							 Request::WORKER);
					militarysite.m_soldier_request->Read(fr, game, mol);
				}
			} else if (packet_version == 2) {
				uint16_t const nr_requests = fr.Unsigned16();
				for (uint16_t i = 0; i < nr_requests; ++i) {
					// Oh well...
					Request req
						(militarysite,
						 Ware_Index::First(),
						 MilitarySite::request_soldier_callback,
						 Request::WORKER);
					req.Read(fr, game, mol);
				}
			}

			if (packet_version == 2) {
				// We don't keep soldier lists anymore, but we do have to fix
				// existing soldiers
				uint16_t const nr_soldiers = fr.Unsigned16();
				for (uint16_t i = 0; i < nr_soldiers; ++i)
					fr.Unsigned32();
			}

			if ((militarysite.m_didconquer = fr.Unsigned8())) {
				//  Add to map of military influence.
				Map const & map = game.map();
				Area<FCoords> a
					(map.get_fcoords(militarysite.get_position()),
					 militarysite.get_conquers());
				Field const & first_map_field = map[0];
				Player::Field * const player_fields =
					militarysite.owner().m_fields;
				MapRegion<Area<FCoords> > mr(map, a);
				do
					player_fields[mr.location().field - &first_map_field]
					.military_influence
						+= map.calc_influence(mr.location(), Area<>(a, a.radius));
				while (mr.advance(map));
			}

			//  capacity (modified by user)
			militarysite.m_capacity = fr.Unsigned8();

			if (packet_version >= 3) {
				militarysite.m_nexthealtime = fr.Signed32();
			}
		} else
			throw game_data_error
				(_("unknown/unhandled version %u"), packet_version);

		//  If the site's capacity is outside the allowed range (can happen if
		//  the site's type's definition has changed), set the variable to the
		//  nearest valid value.
		//
		//  This does not drop excessive soldiers, since they are not loaded into
		//  the site yet. To do that we would have to do this change by adding a
		//  Cmd_ChangeSoldierCapacity to the beginning of the game's command
		//  queue. But that would not work because the command queue is not read
		//  yet and will be cleared before it is read.
		if        (militarysite.m_capacity < militarysite.minSoldierCapacity()) {
			log
				("WARNING: militarysite %u of player %u at (%i, %i) has capacity "
				 "set to %u but it must be at least %u. Changing to that value.\n",
				 militarysite.serial(), militarysite.owner().player_number(),
				 militarysite.get_position().x, militarysite.get_position().y,
				 militarysite.m_capacity, militarysite.minSoldierCapacity());
			militarysite.m_capacity = militarysite.minSoldierCapacity();
		} else if (militarysite.maxSoldierCapacity() < militarysite.m_capacity) {
			log
				("WARNING: militarysite %u of player %u at (%i, %i) has capacity "
				 "set to %u but it can be at most %u. Changing to that value.\n",
				 militarysite.serial(), militarysite.owner().player_number(),
				 militarysite.get_position().x, militarysite.get_position().y,
				 militarysite.m_capacity, militarysite.maxSoldierCapacity());
			militarysite.m_capacity = militarysite.maxSoldierCapacity();
		}
	} catch (_wexception const & e) {
		throw game_data_error(_("militarysite: %s"), e.what());
	}
}

void Map_Buildingdata_Data_Packet::read_productionsite
	(ProductionSite        & productionsite,
	 FileRead              & fr,
	 Game                  & game,
	 Map_Map_Object_Loader & mol)
{
	try {
		uint16_t const packet_version = fr.Unsigned16();
		if
			(1 <= packet_version and
			 packet_version <= CURRENT_PRODUCTIONSITE_PACKET_VERSION)
		{
			//  FIXME The reason for this code is that ProductionSite::init is
			//  FIXME called when a productionsite is allocated during savegame
			//  FIXME loading. That is a bug. That code must only be executed when
			//  FIXME a productionsite is created in the game. Requests are
			//  FIXME created there. Therefore this code here undoes what
			//  FIXME ProductionSite::init just did.
			for (uint32_t i = productionsite.descr().nr_working_positions(); i;) {
				delete productionsite.m_working_positions[--i].worker_request;
				productionsite.m_working_positions[i].worker_request = 0;
			}

			ProductionSite::Working_Position & wp_begin =
				*productionsite.m_working_positions;
			ProductionSite_Descr const & descr = productionsite.descr();
			Ware_Types const & working_positions = descr.working_positions();

			uint16_t nr_worker_requests = fr.Unsigned16();
			for (uint16_t i = nr_worker_requests; i; --i) {
				Request & req =
					*new Request
						(productionsite,
						 Ware_Index::First(),
						 ProductionSite::request_worker_callback,
						 Request::WORKER);
				req.Read(fr, game, mol);
				Ware_Index const worker_index = req.get_index();

				//  Find a working position that matches this request.
				ProductionSite::Working_Position * wp = &wp_begin;
				for
                    (wl_const_range<Ware_Types>
                     j(working_positions);;
					 ++j)
				{
					if (j.empty())
						throw game_data_error
							("site has request for %s, for which there is no working "
							 "position",
							 productionsite.tribe()
							 .get_worker_descr(req.get_index())->name().c_str());
					uint32_t count = j->second;
					assert(count);
					if (worker_index == j->first) {
						while (wp->worker_request)
							if (--count)
								++wp;
							else
								throw game_data_error
									("request for %s does not match any free working "
									 "position",
									 productionsite.tribe()
									 .get_worker_descr(req.get_index())->name().c_str
									 	());
						break;
					} else
						wp += count;
				}
				wp->worker_request = &req;
			}

			uint16_t nr_workers = fr.Unsigned16();
			for (uint16_t i = nr_workers; i; --i) {
				Worker & worker = mol.get<Worker>(fr.Unsigned32());
				Worker_Descr const & worker_descr = worker.descr();

				//  Find a working position that matches this worker.
				ProductionSite::Working_Position * wp = &wp_begin;
				for
                    (wl_const_range<Ware_Types> j(working_positions);;
					 ++j)
				{
					if (j.empty())
						throw game_data_error
							("site has %s, for which there is no free working "
							 "position",
							 worker_descr.name().c_str());
					uint32_t count = j->second;
					assert(count);
					if (worker_descr.can_act_as(j->first)) {
						while (wp->worker or wp->worker_request) {
							++wp;
							if (not --count)
								goto end_working_position;
						}
						break;
					} else
						wp += count;
				end_working_position:;
				}
				wp->worker = &worker;
			}

			if (nr_worker_requests + nr_workers < descr.nr_working_positions())
				throw game_data_error
					("number of worker requests and workers are fewer than the "
					 "number of working positions");

			//  items from flags
			productionsite.m_fetchfromflag = fr.Signed32();

			//  skipped programs
			uint32_t const gametime = game.get_gametime();
			for (uint8_t i = 3 <= packet_version ? fr.Unsigned8() : 0; i; --i) {
				char const * const program_name = fr.CString();
				if (descr.programs().count(program_name)) {
					uint32_t const skip_time = fr.Unsigned32();
					if (gametime < skip_time)
						throw game_data_error
							(_
							 	("program %s was skipped at time %u, but time is only "
							 	 "%u"),
							 program_name, skip_time, gametime);
					productionsite.m_skipped_programs[program_name] = skip_time;
				} else
					log
						("WARNING: productionsite has skipped program \"%s\", which "
						 "does not exist\n",
						 program_name);
			}

			//  state
			uint16_t const nr_progs = fr.Unsigned16();
			productionsite.m_stack.resize(nr_progs);
			for (uint16_t i = 0; i < nr_progs; ++i) {
				std::string program_name = fr.CString();
				std::transform
					(program_name.begin(), program_name.end(), program_name.begin(),
					 tolower);
				productionsite.m_stack[i].program =
					productionsite.descr().get_program(program_name);
				productionsite.m_stack[i].ip    = fr.  Signed32();
				productionsite.m_stack[i].phase = fr.  Signed32();
				productionsite.m_stack[i].flags = fr.Unsigned32();
			}
			productionsite.m_program_timer = fr.Unsigned8();
			productionsite.m_program_time = fr.Signed32();

			uint16_t nr_queues = fr.Unsigned16();
			// perhaps the building had more input queues in earlier versions
			for (; nr_queues > productionsite.m_input_queues.size(); --nr_queues)
				productionsite.m_input_queues[0]->Read(fr, game, mol);
			// do not use productionsite.m_input_queues.size() as maximum, perhaps
			// the older version had less inputs - that way we leave the new ones
			// empty
			for (uint16_t i = 0; i < nr_queues; ++i)
				productionsite.m_input_queues[i]->Read(fr, game, mol);

			uint16_t const stats_size = fr.Unsigned16();
			productionsite.m_statistics.resize(stats_size);
			for (uint32_t i = 0; i < productionsite.m_statistics.size(); ++i)
				productionsite.m_statistics[i] = fr.Unsigned8();
			productionsite.m_statistics_changed = fr.Unsigned8();
			if (packet_version == 1) {
				memcpy(productionsite.m_statistics_buffer, fr.Data(40), 40);
				productionsite.m_statistics_buffer[39] = '\0';
			} else {
				{
					char const * const statistics_string        = fr.CString();
					size_t       const statistics_string_length =
						snprintf
							(productionsite.m_statistics_buffer,
							 sizeof(productionsite.m_statistics_buffer),
							 "%s", statistics_string);
					if
						(sizeof(productionsite.m_statistics_buffer)
						 <=
						 statistics_string_length)
						log
							("WARNING: productionsite statistics string can be at "
							 "most %u characters but a loaded building has the "
							 "string \"%s\" of length %u\n",
							 sizeof(productionsite.m_statistics_buffer) - 1,
							 statistics_string, statistics_string_length);
				}
				if (4 <= packet_version) {
					char const * const result_string        = fr.CString();
					size_t       const result_string_length =
						snprintf
							(productionsite.m_result_buffer,
							 sizeof(productionsite.m_result_buffer),
							 "%s", result_string);
					if
						(sizeof(productionsite.m_result_buffer)
						 <=
						 result_string_length)
						log
							("WARNING: productionsite result string can be at "
							 "most %u characters but a loaded building has the "
							 "string \"%s\" of length %u\n",
							 sizeof(productionsite.m_result_buffer) - 1,
							 result_string, result_string_length);
				}
			}
		} else
			throw game_data_error
				(_("unknown/unhandled version %u"), packet_version);
	} catch (_wexception const & e) {
		throw game_data_error
			("productionsite (%s): %s",
			 productionsite.descname().c_str(), e.what());
	}
}


void Map_Buildingdata_Data_Packet::read_trainingsite
	(TrainingSite          & trainingsite,
	 FileRead              & fr,
	 Game                  & game,
	 Map_Map_Object_Loader & mol)
{
	try {
		uint16_t const trainingsite_packet_version = fr.Unsigned16();
		if
			(trainingsite_packet_version == CURRENT_TRAININGSITE_PACKET_VERSION ||
			 trainingsite_packet_version == 2)
		{
			read_productionsite(trainingsite, fr, game, mol);

			delete trainingsite.m_soldier_request;
			trainingsite.m_soldier_request = 0;
			if (fr.Unsigned8()) {
				trainingsite.m_soldier_request =
					new Request
						(trainingsite,
						 Ware_Index::First(),
						 TrainingSite::request_soldier_callback,
						 Request::WORKER);
				trainingsite.m_soldier_request->Read(fr, game, mol);
			}

			trainingsite.m_capacity = fr.Unsigned8();
			trainingsite.m_build_heros = fr.Unsigned8();

			uint8_t const nr_upgrades = fr.Unsigned8();
			for (uint8_t i = 0; i < nr_upgrades; ++i) {
				tAttribute attribute = static_cast<tAttribute>(fr.Unsigned8());
				if
					(TrainingSite::Upgrade * const upgrade =
					 	trainingsite.get_upgrade(attribute))
				{
					upgrade->prio = fr.Unsigned8();
					upgrade->credit = fr.Unsigned8();
					upgrade->lastattempt = fr.Signed32();
					if (trainingsite_packet_version > 2)
						upgrade->lastsuccess = fr.Unsigned8();
					else
						upgrade->lastsuccess = fr.Signed32() == upgrade->lastattempt;
				} else {
					fr.Unsigned8();
					fr.Unsigned8();
					fr.Signed32();
					fr.Signed32();
				}
			}
		} else if (trainingsite_packet_version == 1) {
			read_productionsite(trainingsite, fr, game, mol);

			// Compatibility: trainingsite used to require a list of soldiers
			// This is now dealt with automatically via add_workers
			{
				uint16_t const nr_requests = fr.Unsigned16();
				for (uint16_t i = 0; i < nr_requests; ++i) {
					Request req
						(trainingsite,
						 Ware_Index::First(),
						 TrainingSite::request_soldier_callback,
						 Request::WORKER);
					req.Read(fr, game, mol);
				}
			}
			{
				uint16_t const nr_soldiers = fr.Unsigned16();
				for (uint16_t i = 0; i < nr_soldiers; ++i) {
					uint32_t const soldier_serial = fr.Unsigned32();
					try {
						trainingsite.m_soldiers.push_back
							(&mol.get<Soldier>(soldier_serial));
					} catch (_wexception const & e) {
						throw game_data_error
							("soldier #%u (%u): %s", i, soldier_serial, e.what());
					}
				}
			}

			// Do not save m_list_upgrades (remake at load).

			//  Building heros?
			trainingsite.m_build_heros = fr.Unsigned8();

			//  priority upgrades
			trainingsite.set_pri(atrHP, fr.Unsigned16());
			trainingsite.set_pri(atrAttack, fr.Unsigned16());
			trainingsite.set_pri(atrDefense, fr.Unsigned16());
			trainingsite.set_pri(atrEvade, fr.Unsigned16());

			//  priority modificators (not compatible with new version)
			fr.Unsigned16();
			fr.Unsigned16();
			fr.Unsigned16();
			fr.Unsigned16();

			//  capacity (modified by user)
			trainingsite.m_capacity = fr.Unsigned8();

			fr.CString(); //  m_prog_name -- this is obsolete

			trainingsite.update_soldier_request();
		} else
			throw game_data_error
				(_("unknown/unhandled version %u"), trainingsite_packet_version);

		//  If the site's capacity is outside the allowed range (can happen if
		//  the site's type's definition has changed), set the variable to the
		//  nearest valid value.
		//
		//  This does not drop excessive soldiers, since they are not loaded into
		//  the site yet. To do that we would have to do this change by adding a
		//  Cmd_ChangeSoldierCapacity to the beginning of the game's command
		//  queue. But that would not work because the command queue is not read
		//  yet and will be cleared before it is read.
		if        (trainingsite.m_capacity < trainingsite.minSoldierCapacity()) {
			log
				("WARNING: trainingsite %u of player %u at (%i, %i) has capacity "
				 "set to %u but it must be at least %u. Changing to that value.\n",
				 trainingsite.serial(), trainingsite.owner().player_number(),
				 trainingsite.get_position().x, trainingsite.get_position().y,
				 trainingsite.m_capacity, trainingsite.minSoldierCapacity());
			trainingsite.m_capacity = trainingsite.minSoldierCapacity();
		} else if (trainingsite.maxSoldierCapacity() < trainingsite.m_capacity) {
			log
				("WARNING: trainingsite %u of player %u at (%i, %i) has capacity "
				 "set to %u but it can be at most %u. Changing to that value.\n",
				 trainingsite.serial(), trainingsite.owner().player_number(),
				 trainingsite.get_position().x, trainingsite.get_position().y,
				 trainingsite.m_capacity, trainingsite.maxSoldierCapacity());
			trainingsite.m_capacity = trainingsite.maxSoldierCapacity();
		}
	} catch (_wexception const & e) {
		throw game_data_error(_("trainingsite: %s"), e.what());
	}
}


void Map_Buildingdata_Data_Packet::Write
	(FileSystem & fs, Editor_Game_Base & egbase, Map_Map_Object_Saver & mos)
throw (_wexception)
{
	FileWrite fw;

	// now packet version
	fw.Unsigned16(CURRENT_PACKET_VERSION);

	// Walk the map again
	Map & map = egbase.map();
	const uint32_t mapwidth = map.get_width();
	Map_Index const max_index = map.max_index();
	for (Map_Index i = 0; i < max_index; ++i)
		if (upcast(Building const, building, map[i].get_immovable())) {
			assert(mos.is_object_known(*building));

			if (Map::get_index(building->get_position(), mapwidth) != i)
				continue; // This is not this buildings main position.

			fw.Unsigned32(mos.get_object_file_index(*building));

			//  player immovable owner is already in existence packet

			//  write the general stuff
			if (building->m_anim) {
				fw.Unsigned8(1);
				fw.String(building->descr().get_animation_name(building->m_anim));
			} else
				fw.Unsigned8(0);

			fw.Unsigned32(building->m_animstart);

			{
				const Building::Leave_Queue & leave_queue = building->m_leave_queue;
				fw.Unsigned16(leave_queue.size());
				container_iterate_const(Building::Leave_Queue, leave_queue, j) {
					assert(mos.is_object_known(*j.current->get(egbase)));
					fw.Unsigned32
						(mos.get_object_file_index(*j.current->get(egbase)));
				}
			}
			fw.Unsigned32(building->m_leave_time);
			if (Map_Object const * const o = building->m_leave_allow.get(egbase))
			{
				assert(mos.is_object_known(*o));
				fw.Unsigned32(mos.get_object_file_index(*o));
			} else
				fw.Unsigned32(0);
			{
				bool is_stopped = false;
				if (upcast(ProductionSite const, productionsite, building))
					is_stopped = productionsite->is_stopped();
				fw.Unsigned8(is_stopped);
			}

			if (upcast(ConstructionSite const, constructionsite, building))
				write_constructionsite
					(*constructionsite,
					 fw,
					 ref_cast<Game, Editor_Game_Base>(egbase),
					 mos);
			else if (upcast(Warehouse const, warehouse, building))
				write_warehouse
					(*warehouse,
					 fw,
					 ref_cast<Game, Editor_Game_Base>(egbase),
					 mos);
			else if (upcast(ProductionSite const, productionsite, building)) {
				if (upcast(MilitarySite const, militarysite, productionsite))
					write_militarysite
						(*militarysite,
						 fw,
						 ref_cast<Game, Editor_Game_Base>(egbase),
						 mos);
				else if (upcast(TrainingSite const, trainingsite, productionsite))
					write_trainingsite
						(*trainingsite,
						 fw,
						 ref_cast<Game, Editor_Game_Base>(egbase),
						 mos);
				else
					write_productionsite
						(*productionsite,
						 fw,
						 ref_cast<Game, Editor_Game_Base>(egbase),
						 mos);
			} else {
				assert(false);
				//  type of building is not one of (or derived from)
				//  {ConstructionSite, Warehouse, ProductionSite}
			}

			mos.mark_object_as_saved(*building);
		}

	fw.Write(fs, "binary/building_data");
}


void Map_Buildingdata_Data_Packet::write_constructionsite
	(ConstructionSite const & constructionsite,
	 FileWrite              & fw,
	 Game                   & game,
	 Map_Map_Object_Saver   & mos)
{

	fw.Unsigned16(CURRENT_CONSTRUCTIONSITE_PACKET_VERSION);

	//  descriptions
	fw.String(constructionsite.m_building->name());
	//  FIXME Just write the string without the 1 first:
	//  FIXME   fw.CString(constructionsite.m_prev_building ?
	//  constructionsite.m_prev_building->name().c_str() : "");
	//  FIXME When reading, the empty string should mean no prev_building.
	if (constructionsite.m_prev_building) {
		fw.Unsigned8(1);
		fw.String(constructionsite.m_prev_building->name());
	} else
		fw.Unsigned8(0);

	// builder request
	if (constructionsite.m_builder_request) {
		fw.Unsigned8(1);
		constructionsite.m_builder_request->Write(fw, game, mos);
	} else
		fw.Unsigned8(0);

	// builder
	if (Worker const * builder = constructionsite.m_builder.get(game)) {
		assert(mos.is_object_known(*builder));
		fw.Unsigned32(mos.get_object_file_index(*builder));
	} else
		fw.Unsigned32(0);

	const uint16_t wares_size = constructionsite.m_wares.size();
	fw.Unsigned16(wares_size);
	for (uint16_t i = 0; i < wares_size; ++i)
		constructionsite.m_wares[i]->Write(fw, game, mos);

	fw.  Signed32(constructionsite.m_fetchfromflag);

	fw.Unsigned8 (constructionsite.m_working);
	fw.Unsigned32(constructionsite.m_work_steptime);
	fw.Unsigned32(constructionsite.m_work_completed);
	fw.Unsigned32(constructionsite.m_work_steps);
}


void Map_Buildingdata_Data_Packet::write_warehouse
	(Warehouse      const & warehouse,
	 FileWrite            & fw,
	 Game                 & game,
	 Map_Map_Object_Saver & mos)
{
	fw.Unsigned16(CURRENT_WAREHOUSE_PACKET_VERSION);

	//  supply
	Tribe_Descr const & tribe = warehouse.tribe();
	WareList const & wares = warehouse.m_supply->get_wares();
	for (Ware_Index i = Ware_Index::First(); i < wares.get_nrwareids  (); ++i) {
		fw.Unsigned8(1);
		fw.String(tribe.get_ware_descr(i)->name());
		fw.Unsigned16(wares.stock(i));
	}
	fw.Unsigned8(0);
	WareList const & workers = warehouse.m_supply->get_workers();
	for (Ware_Index i = Ware_Index::First(); i < workers.get_nrwareids(); ++i) {
		fw.Unsigned8(1);
		fw.String(tribe.get_worker_descr(i)->name());
		fw.Unsigned16(workers.stock(i));
	}
	fw.Unsigned8(0);

	const uint16_t requests_size = warehouse.m_requests.size();
	fw.Unsigned16(requests_size);
	for (uint16_t i = 0; i < warehouse.m_requests.size(); ++i)
		warehouse.m_requests[i]->Write(fw, game, mos);

	//  Incorporated workers, write sorted after file-serial.
	fw.Unsigned16(warehouse.m_incorporated_workers.size());
    typedef std::map<uint32_t, const Worker *> TWorkerMap;
	TWorkerMap workermap;
	container_iterate_const
		(std::vector<OPtr<Worker> >, warehouse.m_incorporated_workers, i)
	{
		Worker const & w = *i.current->get(game);
		assert(mos.is_object_known(w));
		workermap.insert
			(std::pair<uint32_t, const Worker *>
			 	(mos.get_object_file_index(w), &w));
	}
    container_iterate_const(TWorkerMap, workermap, i)
	{
		Worker const & obj = *i.current->second;
		assert(mos.is_object_known(obj));
		fw.Unsigned32(mos.get_object_file_index(obj));
	}

	{
		std::vector<Ware_Index> const & worker_types_without_cost =
			tribe.worker_types_without_cost();
		for (uint8_t i = worker_types_without_cost.size(); i;) {
			uint32_t const next_spawn =
				warehouse.m_next_worker_without_cost_spawn[--i];
			if (next_spawn != Never()) {
				fw.String
					(tribe.get_worker_descr(tribe.worker_types_without_cost().at(i))
					 ->name());
				fw.Unsigned32(next_spawn);
			}
		}
	}
	fw.Unsigned8(0); //  terminator for spawn times
}


void Map_Buildingdata_Data_Packet::write_militarysite
	(MilitarySite   const & militarysite,
	 FileWrite            & fw,
	 Game                 & game,
	 Map_Map_Object_Saver & mos)
{
	fw.Unsigned16(CURRENT_MILITARYSITE_PACKET_VERSION);
	write_productionsite(militarysite, fw, game, mos);

	if (militarysite.m_soldier_request) {
		fw.Unsigned8(1);
		militarysite.m_soldier_request->Write(fw, game, mos);
	} else {
		fw.Unsigned8(0);
	}

	fw.Unsigned8(militarysite.m_didconquer);
	fw.Unsigned8(militarysite.m_capacity);
	fw.Signed32(militarysite.m_nexthealtime);
}


void Map_Buildingdata_Data_Packet::write_productionsite
	(ProductionSite const & productionsite,
	 FileWrite            & fw,
	 Game                 & game,
	 Map_Map_Object_Saver & mos)
{
	fw.Unsigned16(CURRENT_PRODUCTIONSITE_PACKET_VERSION);

	uint32_t const nr_working_positions =
		productionsite.descr().nr_working_positions();
	ProductionSite::Working_Position const & begin =
		productionsite.m_working_positions[0];
	ProductionSite::Working_Position const & end =
		(&begin)[nr_working_positions];
	uint32_t nr_workers = 0;
	for (ProductionSite::Working_Position const * i = &begin; i < &end; ++i)
		nr_workers += i->worker ? 1 : 0;

	//  worker requests
	fw.Unsigned16(nr_working_positions - nr_workers);
	for (ProductionSite::Working_Position const * i = &begin; i < &end; ++i)
		if (Request const * const r = i->worker_request)
			r->Write(fw, game, mos);

	//  workers
	fw.Unsigned16(nr_workers);
	for (ProductionSite::Working_Position const * i = &begin; i < &end; ++i)
		if (Worker const * const w = i->worker) {
			assert(not i->worker_request);
			assert(mos.is_object_known(*w));
			fw.Unsigned32(mos.get_object_file_index(*w));
		}

	fw.Signed32(productionsite.m_fetchfromflag);

	//  skipped programs
	assert
		(productionsite.m_skipped_programs.size()
		 <=
		 std::numeric_limits<uint8_t>::max());
	fw.Unsigned8(productionsite.m_skipped_programs.size());
	container_iterate_const
		(ProductionSite::Skipped_Programs, productionsite.m_skipped_programs, i)
	{
		fw.String    (i.current->first);
		fw.Unsigned32(i.current->second);
	}

	//  state
	uint16_t const program_size = productionsite.m_stack.size();
	fw.Unsigned16(program_size);
	for (uint16_t i = 0; i < program_size; ++i) {
		fw.String    (productionsite.m_stack[i].program->name());
		fw.  Signed32(productionsite.m_stack[i].ip);
		fw.  Signed32(productionsite.m_stack[i].phase);
		fw.Unsigned32(productionsite.m_stack[i].flags);
	}
	fw.Unsigned8(productionsite.m_program_timer);
	fw. Signed32(productionsite.m_program_time);

	const uint16_t input_queues_size = productionsite.m_input_queues.size();
	fw.Unsigned16(input_queues_size);
	for (uint16_t i = 0; i < input_queues_size; ++i)
		productionsite.m_input_queues[i]->Write(fw, game, mos);

	const uint16_t statistics_size = productionsite.m_statistics.size();
	fw.Unsigned16(statistics_size);
	for (uint32_t i = 0; i < statistics_size; ++i)
		fw.Unsigned8(productionsite.m_statistics[i]);
	fw.Unsigned8(productionsite.m_statistics_changed);
	fw.String(productionsite.m_statistics_buffer);
	fw.String(productionsite.m_result_buffer);
}

/*
 * write for trainingsite
 */
void Map_Buildingdata_Data_Packet::write_trainingsite
	(TrainingSite   const & trainingsite,
	 FileWrite            & fw,
	 Game                 & game,
	 Map_Map_Object_Saver & mos)
{
	fw.Unsigned16(CURRENT_TRAININGSITE_PACKET_VERSION);

	write_productionsite(trainingsite, fw, game, mos);

	//  requests

	if (trainingsite.m_soldier_request) {
		fw.Unsigned8(1);
		trainingsite.m_soldier_request->Write(fw, game, mos);
	} else {
		fw.Unsigned8(0);
	}

	fw.Unsigned8(trainingsite.m_capacity);
	fw.Unsigned8(trainingsite.m_build_heros);

	// upgrades
	fw.Unsigned8(trainingsite.m_upgrades.size());
	for (uint8_t i = 0; i < trainingsite.m_upgrades.size(); ++i) {
		TrainingSite::Upgrade const & upgrade = trainingsite.m_upgrades[i];
		fw.Unsigned8(upgrade.attribute);
		fw.Unsigned8(upgrade.prio);
		fw.Unsigned8(upgrade.credit);
		fw.Signed32(upgrade.lastattempt);
		fw.Signed8(upgrade.lastsuccess);
	}

	// DONE
}

}
