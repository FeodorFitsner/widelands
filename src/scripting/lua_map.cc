/*
 * Copyright (C) 2006-2010, 2013 by the Widelands Development Team
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

#include "scripting/lua_map.h"

#include <memory>

#include <boost/format.hpp>

#include "base/log.h"
#include "base/macros.h"
#include "base/wexception.h"
#include "economy/wares_queue.h"
#include "graphic/graphic.h"
#include "logic/findimmovable.h"
#include "logic/map_objects/checkstep.h"
#include "logic/map_objects/immovable.h"
#include "logic/map_objects/tribes/carrier.h"
#include "logic/map_objects/tribes/ship.h"
#include "logic/map_objects/tribes/soldier.h"
#include "logic/map_objects/tribes/tribes.h"
#include "logic/map_objects/tribes/warelist.h"
#include "logic/map_objects/world/resource_description.h"
#include "logic/map_objects/world/terrain_description.h"
#include "logic/map_objects/world/world.h"
#include "logic/maphollowregion.h"
#include "logic/mapregion.h"
#include "logic/player.h"
#include "logic/widelands_geometry.h"
#include "scripting/factory.h"
#include "scripting/globals.h"
#include "scripting/lua_errors.h"
#include "scripting/lua_game.h"
#include "wui/mapviewpixelfunctions.h"

using namespace Widelands;

namespace LuaMaps {


/* RST
:mod:`wl.map`
=============

.. module:: wl.map
   :synopsis: Provides access to Fields and Objects on the map

.. moduleauthor:: The Widelands development team

.. currentmodule:: wl.map

*/

namespace {

// Pushes a lua table with (name, count) pairs for the given 'ware_amount_container' on the
// stack. The 'type' needs to be WARE or WORKER. Returns 1.
int wares_or_workers_map_to_lua(lua_State* L, const Buildcost& ware_amount_map, MapObjectType type) {
	lua_newtable(L);
	for (const auto& ware_amount : ware_amount_map) {
		switch (type) {
		case MapObjectType::WORKER:
			lua_pushstring(L, get_egbase(L).tribes().get_worker_descr(ware_amount.first)->name());
			break;
		case MapObjectType::WARE:
			lua_pushstring(L, get_egbase(L).tribes().get_ware_descr(ware_amount.first)->name());
			break;
		default:
			throw wexception("wares_or_workers_map_to_lua needs a ware or worker");
		}
		lua_pushuint32(L, ware_amount.second);
		lua_settable(L, -3);
	}
	return 1;
}

// Pushes a lua table of tables with food ware names on the stack. Returns 1.
// Resulting table will look e.g. like {{"barbarians_bread"}, {"fish", "meat"}}
int food_list_to_lua(lua_State* L, const std::vector<std::vector<std::string>>& table) {
	lua_newtable(L);
	int counter = 0;
	for (const std::vector<std::string>& foodlist : table) {
		lua_pushnumber(L, ++counter);
		lua_newtable(L);
		int counter2 = 0;
		for (const std::string& foodname : foodlist) {
			lua_pushnumber(L, ++counter2);
			lua_pushstring(L, foodname);
			lua_settable(L, -3);
		}
		lua_settable(L, -3);
	}
	return 1;
}

struct SoldierMapDescr {
	SoldierMapDescr(uint8_t ghp, uint8_t gat, uint8_t gde, uint8_t gev)
	   : hp(ghp), at(gat), de(gde), ev(gev) {
	}
	SoldierMapDescr() : hp(0), at(0), de(0), ev(0) {
	}

	uint8_t hp;
	uint8_t at;
	uint8_t de;
	uint8_t ev;

	bool operator<(const SoldierMapDescr& ot) const {
		bool hp_eq = hp == ot.hp;
		bool at_eq = at == ot.at;
		bool de_eq = de == ot.de;
		if (hp_eq && at_eq && de_eq)
			return ev < ot.ev;
		if (hp_eq && at_eq)
			return de < ot.de;
		if (hp_eq)
			return at < ot.at;
		return hp < ot.hp;
	}
	bool operator == (const SoldierMapDescr& ot) const {
		if (hp == ot.hp && at == ot.at && de == ot.de && ev == ot.ev)
			return true;
		return false;
	}
};

using SoldiersMap = std::map<SoldierMapDescr, uint32_t>;
using WaresMap = std::map<Widelands::DescriptionIndex, uint32_t>;
using WorkersMap = std::map<Widelands::DescriptionIndex, uint32_t>;
using SoldierAmount = std::pair<SoldierMapDescr, uint32_t>;
using WorkerAmount = std::pair<Widelands::DescriptionIndex, uint32_t>;
using PlrInfluence = std::pair<uint8_t, uint32_t>;
using WaresSet = std::set<Widelands::DescriptionIndex>;
using WorkersSet = std::set<Widelands::DescriptionIndex>;
using SoldiersList = std::vector<Widelands::Soldier *>;

// parses the get argument for all classes that can be asked for their
// current wares. Returns a set with all DescriptionIndices that must be considered.
#define GET_INDEX(type) \
	DescriptionIndex m_get_ ## type ## _index \
		(lua_State * L, const TribeDescr& tribe,  const std::string & what) \
	{ \
		DescriptionIndex idx = tribe. type ## _index(what); \
		if (!tribe.has_ ## type (idx)) \
			report_error(L, "Invalid " #type ": <%s>", what.c_str()); \
		return idx; \
	}
GET_INDEX(ware)
GET_INDEX(worker)
#undef GET_INDEX

#define PARSERS(type, btype) \
btype ##sSet m_parse_get_##type##s_arguments \
		(lua_State * L, const TribeDescr& tribe, bool * return_number) \
{ \
	 /* takes either "all", a name or an array of names */ \
	int32_t nargs = lua_gettop(L); \
	if (nargs != 2) \
		report_error(L, "Wrong number of arguments to get_" #type "!"); \
	*return_number = false; \
	btype ## sSet rv; \
	if (lua_isstring(L, 2)) { \
		std::string what = luaL_checkstring(L, -1); \
		if (what == "all") { \
			for (const DescriptionIndex& i : tribe.type##s()) { \
				rv.insert(i); \
			} \
		} else { \
			/* Only one item requested */ \
			rv.insert(m_get_##type##_index(L, tribe, what)); \
			*return_number = true; \
		} \
	} else { \
		/* array of names */ \
		luaL_checktype(L, 2, LUA_TTABLE); \
		lua_pushnil(L); \
		while (lua_next(L, 2) != 0) { \
			rv.insert(m_get_##type##_index(L, tribe, luaL_checkstring(L, -1))); \
			lua_pop(L, 1); \
		} \
	} \
	return rv; \
} \
\
btype##sMap m_parse_set_##type##s_arguments \
	(lua_State * L, const TribeDescr& tribe) \
{ \
	int32_t nargs = lua_gettop(L); \
	if (nargs != 2 && nargs != 3) \
		report_error(L, "Wrong number of arguments to set_" #type "!"); \
   btype##sMap rv; \
	if (nargs == 3) { \
		/* name amount */ \
		rv.insert(btype##Amount( \
			m_get_##type##_index(L, tribe, luaL_checkstring(L, 2)), \
			luaL_checkuint32(L, 3) \
		)); \
	} else { \
		/* array of (name, count) */ \
		luaL_checktype(L, 2, LUA_TTABLE); \
		lua_pushnil(L); \
		while (lua_next(L, 2) != 0) { \
			rv.insert(btype##Amount( \
				m_get_##type##_index(L, tribe, luaL_checkstring(L, -2)), \
				luaL_checkuint32(L, -1) \
			)); \
			lua_pop(L, 1); \
		} \
	} \
	return rv; \
}
PARSERS(ware, Ware)
PARSERS(worker, Worker)
#undef PARSERS

WaresMap count_wares_on_flag_(Flag& f, const Tribes& tribes) {
	WaresMap rv;

	for (const WareInstance * ware : f.get_wares()) {
		DescriptionIndex i = tribes.ware_index(ware->descr().name());
		if (!rv.count(i))
			rv.insert(Widelands::WareAmount(i, 1));
		else
			rv[i] += 1;
	}
	return rv;
}

// Sort functor to sort the owners claiming a field by their influence.
static int sort_claimers(const PlrInfluence& first, const PlrInfluence& second) {
	return first.second > second.second;
}

// Return the valid workers for a Road.
WorkersMap get_valid_workers_for(const Road& r) {
	WorkersMap valid_workers;
	valid_workers.insert(WorkerAmount(r.owner().tribe().carrier(), 1));

	if (r.get_roadtype() == RoadType::kBusy)
		valid_workers.insert(WorkerAmount(r.owner().tribe().carrier2(), 1));

	return valid_workers;
}

// Returns the valid workers allowed in 'pi'.
WorkersMap get_valid_workers_for(const ProductionSite& ps)
{
	WorkersMap rv;
	for (const Widelands::WareAmount& item : ps.descr().working_positions()) {
		rv.insert(WorkerAmount(item.first, item.second));
	}
	return rv;
}

// Translate the given Workers map into a (string, count) Lua table.
int workers_map_to_lua(lua_State * L, const WorkersMap& valid_workers) {
	lua_newtable(L);
	for (const WorkersMap::value_type& item : valid_workers) {
		lua_pushstring(L, get_egbase(L).tribes().get_worker_descr(item.first)->name());
		lua_pushuint32(L, item.second);
		lua_rawset(L, -3);
	}
	return 1;
}

// Does most of the work of get_workers for player immovables (buildings and roads mainly).
int do_get_workers(lua_State* L, const PlayerImmovable& pi, const WorkersMap& valid_workers) {
	const TribeDescr& tribe = pi.owner().tribe();

	bool return_number = false;
	WorkersSet set = m_parse_get_workers_arguments(L, tribe, &return_number);

	WorkersMap c_workers;
	for (const Worker* w : pi.get_workers()) {
		DescriptionIndex i = tribe.worker_index(w->descr().name());
		if (!c_workers.count(i)) {
			c_workers.insert(WorkerAmount(i, 1));
		} else {
			c_workers[i] += 1;
		}
	}

	if (set.size() == tribe.get_nrworkers()) {  // Wants all returned
		set.clear();
		for (const WorkersMap::value_type& v : valid_workers) {
			set.insert(v.first);
		}
	}

	if (!return_number)
		lua_newtable(L);

	for (const DescriptionIndex& i : set) {
		uint32_t cnt = 0;
		if (c_workers.count(i))
			cnt = c_workers[i];

		if (return_number) {
			lua_pushuint32(L, cnt);
			break;
		} else {
			lua_pushstring(L, tribe.get_worker_descr(i)->name());
			lua_pushuint32(L, cnt);
			lua_rawset(L, -3);
		}
	}
	return 1;
}

// Does most of the work of set_workers for player immovables (buildings and roads mainly).
template <typename T>
int do_set_workers(lua_State* L, PlayerImmovable* pi, const WorkersMap& valid_workers) {
	EditorGameBase& egbase = get_egbase(L);
	const TribeDescr& tribe = pi->owner().tribe();

	WorkersMap setpoints = m_parse_set_workers_arguments(L, tribe);

	WorkersMap c_workers;
	for (const Worker* w : pi->get_workers()) {
		DescriptionIndex i = tribe.worker_index(w->descr().name());
		if (!c_workers.count(i))
			c_workers.insert(WorkerAmount(i, 1));
		else
			c_workers[i] += 1;
		if (!setpoints.count(i))
			setpoints.insert(WorkerAmount(i, 0));
	}

	// The idea is to change as little as possible
	for (const WorkersMap::value_type sp : setpoints) {
		const WorkerDescr* wdes = tribe.get_worker_descr(sp.first);
		if (!valid_workers.count(sp.first))
			report_error(L, "<%s> can't be employed here!", wdes->name().c_str());

		uint32_t cur = 0;
		WorkersMap::iterator i = c_workers.find(sp.first);
		if (i != c_workers.end())
			cur = i->second;

		int d = sp.second - cur;
		if (d < 0) {
			while (d) {
				for (const Worker* w : pi->get_workers()) {
					if (tribe.worker_index(w->descr().name()) == sp.first) {
						const_cast<Worker*>(w)->remove(egbase);
						++d;
						break;
					}
				}
			}
		} else if (d > 0) {
			for (; d; --d)
				if (T::create_new_worker(*pi, egbase, wdes))
					report_error(L, "No space left for this worker");
		}
	}
	return 0;
}

// Unpacks the Lua table of the form {hp, at, de, ev} at the stack index
// 'table_index' into a SoldierMapDescr struct.
SoldierMapDescr unbox_lua_soldier_description(lua_State* L, int table_index, const SoldierDescr& sd) {
	SoldierMapDescr soldier_descr;

	lua_pushuint32(L, 1);
	lua_rawget(L, table_index);
	soldier_descr.hp = luaL_checkuint32(L, -1);
	lua_pop(L, 1);
	if (soldier_descr.hp > sd.get_max_hp_level())
		report_error
			(L, "hp level (%i) > max hp level (%i)", soldier_descr.hp, sd.get_max_hp_level());

	lua_pushuint32(L, 2);
	lua_rawget(L, table_index);
	soldier_descr.at = luaL_checkuint32(L, -1);
	lua_pop(L, 1);
	if (soldier_descr.at > sd.get_max_attack_level())
		report_error
			(L, "attack level (%i) > max attack level (%i)", soldier_descr.at, sd.get_max_attack_level());

	lua_pushuint32(L, 3);
	lua_rawget(L, table_index);
	soldier_descr.de = luaL_checkuint32(L, -1);
	lua_pop(L, 1);
	if (soldier_descr.de > sd.get_max_defense_level())
		report_error
			(L, "defense level (%i) > max defense level (%i)", soldier_descr.de,
			 sd.get_max_defense_level());

	lua_pushuint32(L, 4);
	lua_rawget(L, table_index);
	soldier_descr.ev = luaL_checkuint32(L, -1);
	lua_pop(L, 1);
	if (soldier_descr.ev > sd.get_max_evade_level())
		report_error
			(L, "evade level (%i) > max evade level (%i)", soldier_descr.ev, sd.get_max_evade_level());

	return soldier_descr;
}

// Parser the arguments of set_soldiers() into a setpoint. See the
// documentation in HasSoldiers to understand the valid arguments.
SoldiersMap m_parse_set_soldiers_arguments(lua_State* L, const SoldierDescr& soldier_descr) {
	SoldiersMap rv;
	if (lua_gettop(L) > 2) {
		// STACK: cls, descr, count
		const uint32_t count = luaL_checkuint32(L, 3);
		const SoldierMapDescr d = unbox_lua_soldier_description(L, 2, soldier_descr);
		rv.insert(SoldierAmount(d, count));
	} else {
		lua_pushnil(L);
		while (lua_next(L, 2) != 0) {
			const SoldierMapDescr d = unbox_lua_soldier_description(L, 3, soldier_descr);
			const uint32_t count = luaL_checkuint32(L, -1);
			rv.insert(SoldierAmount(d, count));
			lua_pop(L, 1);
		}
	}
	return rv;
}

// Does most of the work of get_soldiers for buildings.
int do_get_soldiers(lua_State* L, const Widelands::SoldierControl& sc, const TribeDescr& tribe) {
	if (lua_gettop(L) != 2)
		report_error(L, "Invalid arguments!");

	const SoldiersList soldiers = sc.stationed_soldiers();
	if (lua_isstring(L, -1)) {
		if (std::string(luaL_checkstring(L, -1)) != "all")
			report_error(L, "Invalid arguments!");

		// Return All Soldiers
		SoldiersMap hist;
		for (const Soldier* s : soldiers) {
			SoldierMapDescr sd
				(s->get_hp_level(), s->get_attack_level(),
				 s->get_defense_level(), s->get_evade_level());

			SoldiersMap::iterator i = hist.find(sd);
			if (i == hist.end())
				hist[sd] = 1;
			else
				i->second += 1;
		}

		// Get this to Lua.
		lua_newtable(L);
		for (const SoldiersMap::value_type& i : hist) {
			lua_createtable(L, 4, 0);
#define PUSHLEVEL(idx, name)                                                                       \
	lua_pushuint32(L, idx);                                                                         \
	lua_pushuint32(L, i.first.name);                                                                \
	lua_rawset(L, -3);
			PUSHLEVEL(1, hp);
			PUSHLEVEL(2, at);
			PUSHLEVEL(3, de);
			PUSHLEVEL(4, ev);
#undef PUSHLEVEL

			lua_pushuint32(L, i.second);
			lua_rawset(L, -3);
		}
	} else {
		const SoldierDescr& soldier_descr = dynamic_cast<const SoldierDescr&>
			(*tribe.get_worker_descr(tribe.soldier()));

		// Only return the number of those requested
		const SoldierMapDescr wanted = unbox_lua_soldier_description(L, 2, soldier_descr);
		uint32_t rv = 0;
		for (const Soldier* s : soldiers) {
			SoldierMapDescr sd
				(s->get_hp_level(), s->get_attack_level(), s->get_defense_level(), s->get_evade_level());
			if (sd == wanted)
				++rv;
		}
		lua_pushuint32(L, rv);
	}
	return 1;
}

// Does most of the work of set_soldiers for buildings.
int do_set_soldiers
	(lua_State* L, const Coords& building_position, SoldierControl* sc, Player* owner)
{
	assert(sc != nullptr);
	assert(owner != nullptr);

	const TribeDescr& tribe = owner->tribe();
	const SoldierDescr& soldier_descr =  //  soldiers
		dynamic_cast<const SoldierDescr&>
			(*tribe.get_worker_descr(tribe.soldier()));
	SoldiersMap setpoints = m_parse_set_soldiers_arguments(L, soldier_descr);

	// Get information about current soldiers
	const std::vector<Soldier*> curs = sc->stationed_soldiers();
	SoldiersMap hist;
	for (const Soldier* s : curs) {
		SoldierMapDescr sd
			(s->get_hp_level(), s->get_attack_level(),
			 s->get_defense_level(), s->get_evade_level());

		SoldiersMap::iterator i = hist.find(sd);
		if (i == hist.end())
			hist[sd] = 1;
		else
			i->second += 1;
		if (!setpoints.count(sd))
			setpoints[sd] = 0;
	}

	// Now adjust them
	EditorGameBase& egbase = get_egbase(L);
	for (const SoldiersMap::value_type& sp : setpoints) {
		uint32_t cur = 0;
		SoldiersMap::iterator i = hist.find(sp.first);
		if (i != hist.end())
			cur = i->second;

		int d = sp.second - cur;
		if (d < 0) {
			while (d) {
				for (Soldier* s : sc->stationed_soldiers()) {
					SoldierMapDescr is
						(s->get_hp_level(), s->get_attack_level(),
						 s->get_defense_level(), s->get_evade_level());

					if (is == sp.first) {
						sc->outcorporate_soldier(egbase, *s);
						s->remove(egbase);
						++d;
						break;
					}
				}
			}
		} else if (d > 0) {
			for (; d; --d) {
				Soldier& soldier = dynamic_cast<Soldier&>
					(soldier_descr.create(egbase, *owner, nullptr, building_position));
				soldier.set_level
					(sp.first.hp, sp.first.at, sp.first.de, sp.first.ev);
				if (sc->incorporate_soldier(egbase, soldier)) {
					soldier.remove(egbase);
					report_error(L, "No space left for soldier!");
				}
			}
		}
	}
	return 0;
}
}  // namespace


/*
 * Upcast the given map object description to a higher type and hand this
 * to Lua. We use this so that scripters always work with the highest class
 * object available.
 */
#define CAST_TO_LUA(klass, lua_klass) to_lua<lua_klass> \
	(L, new lua_klass(static_cast<const klass *>(descr)))
int upcasted_map_object_descr_to_lua(lua_State* L, const MapObjectDescr* const descr) {
	assert(descr != nullptr);

	if (descr->type() >= MapObjectType::BUILDING)
	{
		switch (descr->type()) {
			case MapObjectType::CONSTRUCTIONSITE:
				return CAST_TO_LUA(ConstructionSiteDescr, LuaConstructionSiteDescription);
			case MapObjectType::DISMANTLESITE:
				return CAST_TO_LUA(DismantleSiteDescr, LuaDismantleSiteDescription);
			case MapObjectType::PRODUCTIONSITE:
				return CAST_TO_LUA(ProductionSiteDescr, LuaProductionSiteDescription);
			case MapObjectType::MILITARYSITE:
				return CAST_TO_LUA(MilitarySiteDescr, LuaMilitarySiteDescription);
			case MapObjectType::WAREHOUSE:
				return CAST_TO_LUA(WarehouseDescr, LuaWarehouseDescription);
			case MapObjectType::TRAININGSITE:
				return CAST_TO_LUA(TrainingSiteDescr, LuaTrainingSiteDescription);
			default:
				return CAST_TO_LUA(BuildingDescr, LuaBuildingDescription);
		}
	}
	else {
		switch (descr->type()) {
			case MapObjectType::WARE:
				return CAST_TO_LUA(WareDescr, LuaWareDescription);
			case MapObjectType::WORKER:
				return CAST_TO_LUA(WorkerDescr, LuaWorkerDescription);
			case MapObjectType::CARRIER:
				return CAST_TO_LUA(WorkerDescr, LuaWorkerDescription);
			case MapObjectType::SOLDIER:
				return CAST_TO_LUA(WorkerDescr, LuaWorkerDescription);
			default:
				return CAST_TO_LUA(MapObjectDescr, LuaMapObjectDescription);
		}
	}
}
#undef CAST_TO_LUA

/*
 * Upcast the given map object to a higher type and hand this to
 * Lua. We use this so that scripters always work with the highest class
 * object available.
 */
#define CAST_TO_LUA(k) to_lua<Lua ##k> \
	(L, new Lua ##k(*static_cast<k *>(mo)))
int upcasted_map_object_to_lua(lua_State * L, MapObject * mo) {
	if (!mo)
		return 0;

	switch (mo->descr().type()) {
	case MapObjectType::CRITTER:
		return CAST_TO_LUA(Bob);
	case MapObjectType::SHIP:
		return CAST_TO_LUA(Ship);
	case MapObjectType::WORKER:
		return CAST_TO_LUA(Worker);
	case MapObjectType::CARRIER:
		// TODO(sirver): not yet implemented
		return CAST_TO_LUA(Worker);
	case MapObjectType::SOLDIER:
		return CAST_TO_LUA(Soldier);

	case MapObjectType::IMMOVABLE:
		return CAST_TO_LUA(BaseImmovable);

	case MapObjectType::FLAG:
		return CAST_TO_LUA(Flag);
	case MapObjectType::ROAD:
		return CAST_TO_LUA(Road);
	case MapObjectType::PORTDOCK:
		return CAST_TO_LUA(PortDock);

	case MapObjectType::BUILDING:
		return CAST_TO_LUA(Building);
	case MapObjectType::CONSTRUCTIONSITE:
		return CAST_TO_LUA(ConstructionSite);
	case MapObjectType::DISMANTLESITE:
		// TODO(sirver): not yet implemented.
		return CAST_TO_LUA(Building);
	case MapObjectType::WAREHOUSE:
		return CAST_TO_LUA(Warehouse);
	case MapObjectType::PRODUCTIONSITE:
		return CAST_TO_LUA(ProductionSite);
	case MapObjectType::MILITARYSITE:
		return CAST_TO_LUA(MilitarySite);
	case MapObjectType::TRAININGSITE:
		return CAST_TO_LUA(TrainingSite);

	default:
		throw LuaError((boost::format("upcasted_map_object_to_lua: Unknown %i") %
		                static_cast<int>(mo->descr().type())).str());
	}
}
#undef CAST_TO_LUA


/*
 * ========================================================================
 *                         MODULE CLASSES
 * ========================================================================
 */

/* RST
Module Interfaces
^^^^^^^^^^^^^^^^^

*/

/* RST
HasWares
--------

.. class:: HasWares

	HasWares is an interface that all :class:`PlayerImmovable` objects
	that can contain wares implement. This is at the time of this writing
	:class:`~wl.map.Flag`, :class:`~wl.map.Warehouse` and
	:class:`~wl.map.ProductionSite`.
*/

/* RST
	.. method:: get_wares(which)

		Gets the number of wares that currently reside here.

		:arg which:  can be either of

		* the string :const:`all`.
			  In this case the function will return a
			  :class:`table` of (ware name,amount) pairs that gives information
			  about all ware information available for this object.
		* a ware name.
			In this case a single integer is returned. No check is made
			if this ware makes sense for this location, you can for example ask a
			:const:`lumberjacks_hut` for the number of :const:`raw_stone` he has
			and he will return 0.
		* an :class:`array` of ware names.
			In this case a :class:`table` of
			(ware name,amount) pairs is returned where only the requested wares
			are listed. All other entries are :const:`nil`.

		:returns: :class:`integer` or :class:`table`
*/

/* RST
	.. method:: set_wares(which[, amount])

		Sets the wares available in this location. Either takes two arguments,
		a ware name and an amount to set it too. Or it takes a table of
		(ware name, amount) pairs. Wares are created and added to an economy out
		of thin air.

		:arg which: name of ware or (ware_name, amount) table
		:type which: :class:`string` or :class:`table`
		:arg amount: this many units will be available after the call
		:type amount: :class:`integer`
*/

/* RST
	.. attribute:: valid_wares

		(RO) A :class:`table` of (ware_name, count) if storage is somehow
		constrained in this location. For example for a
		:class:`~wl.map.ProductionSite` this is the information what wares
		and how much can be stored as inputs. For unconstrained storage (like
		:class:`~wl.map.Warehouse`) this is :const:`nil`.

		You can use this to quickly fill a building:

		.. code-block:: lua

			if b.valid_wares then b:set_wares(b.valid_wares) end
*/

/* RST
HasWorkers
----------

.. class:: HasWorkers

	Analogon to :class:`HasWares`, but for Workers. Supported at the time of
	this writing by :class:`~wl.map.Road`, :class:`~wl.map.Warehouse` and
	:class:`~wl.map.ProductionSite`.
*/

/* RST
	.. method:: get_workers(which)

		Similar to :meth:`HasWares.get_wares`.
*/

/* RST
	.. method:: set_workers(which[, amount])

		Similar to :meth:`HasWares.set_wares`.
*/

/* RST
	.. attribute:: valid_workers

		(RO) Similar to :attr:`HasWares.valid_wares` but for workers in this
		location.
*/

/* RST
HasSoldiers
------------

.. class:: HasSoldiers

	Analogon to :class:`HasWorkers`, but for Soldiers. Due to differences in
	Soldiers and implementation details in Lua this class has a slightly
	different interface than :class:`HasWorkers`. Supported at the time of this
	writing by :class:`~wl.map.Warehouse`, :class:`~wl.map.MilitarySite` and
	:class:`~wl.map.TrainingSite`.
*/

/* RST
	.. method:: get_soldiers(descr)

		Gets information about the soldiers in a location.

		:arg descr: can be either of

		* a soldier description.
			Returns an :class:`integer` which is the number of soldiers of this
			kind in this building.

			A soldier description is a :class:`array` that contains the level for
			hitpoints, attack, defense and evade (in this order). An usage example:

			.. code-block:: lua

				w:get_soldiers({0,0,0,0})

			would return the number of soldiers of level 0 in this location.

		* the string :const:`all`.
			In this case a :class:`table` of (soldier descriptions, count) is
			returned. Note that the following will not work, because Lua indexes
			tables by identity:

			.. code-block:: lua

				w:set_soldiers({0,0,0,0}, 100)
				w:get_soldiers({0,0,0,0}) -- works, returns 100
				w:get_soldiers("all")[{0,0,0,0}] -- works not, this is nil

				-- Following is a working way to check for a {0,0,0,0} soldier
				for descr,count in pairs(w:get_soldiers("all")) do
					if descr[1] == 0 and descr[2] == 0 and
						descr[3] == 0 and descr[4] == 0 then
							print(count)
					end
				end

		:returns: Number of soldiers that match descr or the :class:`table`
			containing all soldiers
		:rtype: :class:`integer` or :class:`table`.
*/

/* RST
	.. method:: set_soldiers(which[, amount])

		Analogous to :meth:`HasWorkers.set_workers`, but for soldiers. Instead of
		a name a :class:`array` is used to define the soldier. See
		:meth:`get_soldiers` for an example.

		Usage example:

		.. code-block:: lua

			l:set_soldiers({0,0,0,0}, 100)

		would add 100 level 0 soldiers. While

		.. code-block:: lua

			l:set_soldiers({
			  [{0,0,0,0}] = 10,
			  [{1,2,3,4}] = 5,
			})

		would add 10 level 0 soldier and 5 soldiers with hit point level 1,
		attack level 2, defense level 3 and evade level 4 (as long as this is
		legal for the players tribe).

		:arg which: either a table of (description, count) pairs or one
			description. In that case amount has to be specified as well.
		:type which: :class:`table` or :class:`array`.
*/

/* RST
	.. attribute:: max_soldiers

		(RO) The maximum number of soldiers that can be inside this building at
		one time. If it is not constrained, like for :class:`~wl.map.Warehouse`
		this will be :const:`nil`.
*/


/* RST
Module Classes
^^^^^^^^^^^^^^

*/

/* RST
Map
---

.. class:: Map

	Access to the map and it's objects. You cannot instantiate this directly,
	instead access it via :attr:`wl.Game.map`.
*/
const char LuaMap::className[] = "Map";
const MethodType<LuaMap> LuaMap::Methods[] = {
	METHOD(LuaMap, place_immovable),
	METHOD(LuaMap, get_field),
	METHOD(LuaMap, recalculate),
	{nullptr, nullptr},
};
const PropertyType<LuaMap> LuaMap::Properties[] = {
	PROP_RO(LuaMap, width),
	PROP_RO(LuaMap, height),
	PROP_RO(LuaMap, player_slots),
	{nullptr, nullptr, nullptr},
};

void LuaMap::__persist(lua_State * /* L */) {
}

void LuaMap::__unpersist(lua_State * /* L */) {
}

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
/* RST
	.. attribute:: width

		(RO) The width of the map in fields.
*/
int LuaMap::get_width(lua_State * L) {
	lua_pushuint32(L, get_egbase(L).map().get_width());
	return 1;
}
/* RST
	.. attribute:: height

		(RO) The height of the map in fields.
*/
int LuaMap::get_height(lua_State * L) {
	lua_pushuint32(L, get_egbase(L).map().get_height());
	return 1;
}

/* RST
	.. attribute:: player_slots

		(RO) This is an :class:`array` that contains :class:`~wl.map.PlayerSlots`
		for each player defined in the map.
*/
int LuaMap::get_player_slots(lua_State * L) {
	Map & m = get_egbase(L).map();

	lua_createtable(L, m.get_nrplayers(), 0);
	for (uint32_t i = 0; i < m.get_nrplayers(); i++) {
		lua_pushuint32(L, i + 1);
		to_lua<LuaMaps::LuaPlayerSlot>(L, new LuaMaps::LuaPlayerSlot(i + 1));
		lua_settable(L, -3);
	}

	return 1;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */
/* RST
	.. method:: place_immovable(name, field, from_where)

		Creates an immovable that is defined by the world (e.g. trees, rocks...)
		or a tribe (field) on a given field. If there is already an immovable on
		the field, an error is reported.

		:arg name: The name of the immovable to create
		:type name: :class:`string`
		:arg field: The immovable is created on this field.
		:type field: :class:`wl.map.Field`
		:arg from_where: "world" if the immovable is defined in the world,
			"tribes" if it is defined in the tribes.
		:type from_where: :class:`string`

		:returns: The created immovable.
*/
int LuaMap::place_immovable(lua_State * const L) {
	std::string from_where = "";

	const std::string objname = luaL_checkstring(L, 2);
	LuaMaps::LuaField * c = *get_user_class<LuaMaps::LuaField>(L, 3);
	if (lua_gettop(L) > 3 && !lua_isnil(L, 4))
		from_where = luaL_checkstring(L, 4);

	// Check if the map is still free here
	if
	 (BaseImmovable const * const imm = c->fcoords(L).field->get_immovable())
		if (imm->get_size() >= BaseImmovable::SMALL)
			report_error(L, "Node is no longer free!");

	EditorGameBase & egbase = get_egbase(L);

	BaseImmovable * m = nullptr;
	if (from_where == "world") {
		DescriptionIndex const imm_idx = egbase.world().get_immovable_index(objname);
		if (imm_idx == Widelands::INVALID_INDEX)
			report_error(L, "Unknown world immovable <%s>", objname.c_str());

		m = &egbase.create_immovable(c->coords(), imm_idx, MapObjectDescr::OwnerType::kWorld);
	} else if (from_where == "tribes") {
		DescriptionIndex const imm_idx = egbase.tribes().immovable_index(objname);
		if (imm_idx == Widelands::INVALID_INDEX)
			report_error(L, "Unknown tribes immovable <%s>", objname.c_str());

		m = &egbase.create_immovable(c->coords(), imm_idx, MapObjectDescr::OwnerType::kTribe);
	} else {
		report_error(L, "There are no immovables for <%s>. Use \"world\" or \"tribes\"", from_where.c_str());
	}

	return LuaMaps::upcasted_map_object_to_lua(L, m);
}

/* RST
	.. method:: get_field(x, y)

		Returns a :class:`wl.map.Field` object of the given index.
*/
int LuaMap::get_field(lua_State * L) {
	uint32_t x = luaL_checkuint32(L, 2);
	uint32_t y = luaL_checkuint32(L, 3);

	Map & m = get_egbase(L).map();

	if (x >= static_cast<uint32_t>(m.get_width()))
		report_error(L, "x coordinate out of range!");
	if (y >= static_cast<uint32_t>(m.get_height()))
		report_error(L, "y coordinate out of range!");

	return to_lua<LuaMaps::LuaField>(L, new LuaMaps::LuaField(x, y));
}

/* RST
	.. method:: recalculate()

		This map recalculates the whole map state: height of fields, buildcaps
		and so on. You only need to call this function if you changed
		Field.raw_height in any way.
*/
// TODO(unknown): do we really want this function?
int LuaMap::recalculate(lua_State * L) {
	EditorGameBase & egbase = get_egbase(L);
	egbase.map().recalc_whole_map(egbase.world());
	return 0;
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */


/* RST
TribeDescription
--------------------
.. class:: TribeDescription

	A static description of a tribe.
	This class contains information about which buildings, wares, workers etc. a tribe uses.
*/
const char LuaTribeDescription::className[] = "TribeDescription";
const MethodType<LuaTribeDescription> LuaTribeDescription::Methods[] = {
	METHOD(LuaTribeDescription, has_building),
	METHOD(LuaTribeDescription, has_ware),
	METHOD(LuaTribeDescription, has_worker),
	{nullptr, nullptr},
};
const PropertyType<LuaTribeDescription> LuaTribeDescription::Properties[] = {
	PROP_RO(LuaTribeDescription, buildings),
	PROP_RO(LuaTribeDescription, carrier),
	PROP_RO(LuaTribeDescription, carrier2),
	PROP_RO(LuaTribeDescription, descname),
	PROP_RO(LuaTribeDescription, geologist),
	PROP_RO(LuaTribeDescription, headquarters),
	PROP_RO(LuaTribeDescription, name),
	PROP_RO(LuaTribeDescription, port),
	PROP_RO(LuaTribeDescription, ship),
	PROP_RO(LuaTribeDescription, soldier),
	PROP_RO(LuaTribeDescription, wares),
	PROP_RO(LuaTribeDescription, workers),
	{nullptr, nullptr, nullptr},
};

void LuaTribeDescription::__persist(lua_State* L) {
	const TribeDescr* descr = get();
	PERS_STRING("name", descr->name());
}

void LuaTribeDescription::__unpersist(lua_State* L) {
	std::string name;
	UNPERS_STRING("name", name);
	const Tribes& tribes = get_egbase(L).tribes();
	DescriptionIndex idx = tribes.safe_tribe_index(name);
	set_description_pointer(tribes.get_tribe_descr(idx));
}

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */

/* RST
	.. attribute:: buildings

			(RO) an array of :class:`string` with the names of all the buildings that the tribe can use
*/

int LuaTribeDescription::get_buildings(lua_State * L) {
	lua_newtable(L);
	int counter = 0;
	for (DescriptionIndex building : get()->buildings()) {
		lua_pushinteger(L, ++counter);
		lua_pushstring(L, get_egbase(L).tribes().get_building_descr(building)->name());
		lua_settable(L, -3);
	}
	return 1;
}

/* RST
	.. attribute:: carrier

			(RO) the :class:`string` internal name of the carrier type that this tribe uses
*/

int LuaTribeDescription::get_carrier(lua_State * L) {
	lua_pushstring(L, get_egbase(L).tribes().get_worker_descr(get()->carrier())->name());
	return 1;
}

/* RST
	.. attribute:: carrier2

			(RO) the :class:`string` internal name of the carrier2 type that this tribe uses.
				  e.g. 'atlanteans_horse'
*/

int LuaTribeDescription::get_carrier2(lua_State * L) {
	lua_pushstring(L, get_egbase(L).tribes().get_worker_descr(get()->carrier2())->name());
	return 1;
}


/* RST
	.. attribute:: descname

			(RO) a :class:`string` with the tribe's localized name
*/

int LuaTribeDescription::get_descname(lua_State * L) {
	lua_pushstring(L, get()->descname());
	return 1;
}

/* RST
	.. attribute:: geologist

			(RO) the :class:`string` internal name of the geologist type that this tribe uses
*/

int LuaTribeDescription::get_geologist(lua_State * L) {
	lua_pushstring(L, get_egbase(L).tribes().get_worker_descr(get()->geologist())->name());
	return 1;
}

/* RST
	.. attribute:: headquarters

			(RO) the :class:`string` internal name of the default headquarters type that this tribe uses
*/

int LuaTribeDescription::get_headquarters(lua_State * L) {
	lua_pushstring(L, get_egbase(L).tribes().get_building_descr(get()->headquarters())->name());
	return 1;
}

/* RST
	.. attribute:: name

			(RO) a :class:`string` with the tribe's internal name
*/

int LuaTribeDescription::get_name(lua_State * L) {
	lua_pushstring(L, get()->name());
	return 1;
}

/* RST
	.. attribute:: port

			(RO) the :class:`string` internal name of the port type that this tribe uses
*/

int LuaTribeDescription::get_port(lua_State * L) {
	lua_pushstring(L, get_egbase(L).tribes().get_building_descr(get()->port())->name());
	return 1;
}


/* RST
	.. attribute:: ship

			(RO) the :class:`string` internal name of the ship type that this tribe uses
*/

int LuaTribeDescription::get_ship(lua_State * L) {
	lua_pushstring(L, get_egbase(L).tribes().get_ship_descr(get()->ship())->name());
	return 1;
}



/* RST
	.. attribute:: soldier

			(RO) the :class:`string` internal name of the soldier type that this tribe uses
*/

int LuaTribeDescription::get_soldier(lua_State * L) {
	lua_pushstring(L, get_egbase(L).tribes().get_worker_descr(get()->soldier())->name());
	return 1;
}


/* RST
	.. attribute:: wares

			(RO) an array of :class:`string` with the names of all the wares that the tribe uses
*/

int LuaTribeDescription::get_wares(lua_State * L) {
	lua_newtable(L);
	int counter = 0;
	for (DescriptionIndex ware : get()->wares()) {
		lua_pushinteger(L, ++counter);
		lua_pushstring(L, get_egbase(L).tribes().get_ware_descr(ware)->name());
		lua_settable(L, -3);
	}
	return 1;
}

/* RST
	.. attribute:: workers

			(RO) an array of :class:`string` with the names of all the workers that the tribe can use
*/

int LuaTribeDescription::get_workers(lua_State * L) {
	lua_newtable(L);
	int counter = 0;
	for (DescriptionIndex worker : get()->workers()) {
		lua_pushinteger(L, ++counter);
		lua_pushstring(L, get_egbase(L).tribes().get_worker_descr(worker)->name());
		lua_settable(L, -3);
	}
	return 1;
}


/* RST
	.. method:: has_building(buildingname)

		Returns true if buildingname is a building and the tribe can use it.

		:returns: :const:`true` or :const:`false`
		:rtype: :class:`bool`
*/
int LuaTribeDescription::has_building(lua_State * L) {
	const std::string buildingname = luaL_checkstring(L, 2);
	const DescriptionIndex index = get_egbase(L).tribes().building_index(buildingname);
	lua_pushboolean(L, get()->has_building(index));
	return 1;
}

/* RST
	.. method:: has_ware(warename)

		Returns true if warename is a ware and the tribe uses it.

		:returns: :const:`true` or :const:`false`
		:rtype: :class:`bool`
*/
int LuaTribeDescription::has_ware(lua_State * L) {
	const std::string warename = luaL_checkstring(L, 2);
	const DescriptionIndex index = get_egbase(L).tribes().ware_index(warename);
	lua_pushboolean(L, get()->has_ware(index));
	return 1;
}


/* RST
	.. method:: has_worker(workername)

		Returns true if workername is a worker and the tribe can use it.

		:returns: :const:`true` or :const:`false`
		:rtype: :class:`bool`
*/
int LuaTribeDescription::has_worker(lua_State * L) {
	const std::string workername = luaL_checkstring(L, 2);
	const DescriptionIndex index = get_egbase(L).tribes().worker_index(workername);
	lua_pushboolean(L, get()->has_worker(index));
	return 1;
}




/* RST
MapObjectDescription
--------------------

.. class:: MapObjectDescription

	A static description of a tribe's map object, so it can be used in help files
	without having to access an actual object on the map.
	This class contains the properties that are common to all map objects such as buildings or wares.

	The dynamic MapObject class corresponding to this class is the base class for all Objects in widelands,
	including immovables and Bobs. This class can't be instantiated directly, but provides the base
	for all others.
*/
const char LuaMapObjectDescription::className[] = "MapObjectDescription";
const MethodType<LuaMapObjectDescription> LuaMapObjectDescription::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaMapObjectDescription> LuaMapObjectDescription::Properties[] = {
	PROP_RO(LuaMapObjectDescription, descname),
	PROP_RO(LuaMapObjectDescription, icon_name),
	PROP_RO(LuaMapObjectDescription, name),
	PROP_RO(LuaMapObjectDescription, type_name),
	PROP_RO(LuaMapObjectDescription, representative_image),
	{nullptr, nullptr, nullptr},
};

// Only base classes can be persisted.
void LuaMapObjectDescription::__persist(lua_State*) {
	NEVER_HERE();
}

void LuaMapObjectDescription::__unpersist(lua_State*) {
	NEVER_HERE();
}

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */


/* RST
	.. attribute:: descname

			(RO) a :class:`string` with the map object's localized name
*/

int LuaMapObjectDescription::get_descname(lua_State * L) {
	lua_pushstring(L, get()->descname());
	return 1;
}

/* RST
	.. attribute:: icon_name

			(RO) the filename for the menu icon.
*/
int LuaMapObjectDescription::get_icon_name(lua_State * L) {
	lua_pushstring(L, get()->icon_filename());
	return 1;
}


/* RST
	.. attribute:: name

			(RO) a :class:`string` with the map object's internal name
*/

int LuaMapObjectDescription::get_name(lua_State * L) {
	lua_pushstring(L, get()->name());
	return 1;
}


/* RST
	.. attribute:: representative_image

			(RO) a :class:`string` with the file path to the representative image
			of the map object's idle animation
*/
int LuaMapObjectDescription::get_representative_image(lua_State * L) {
	lua_pushstring(L, get()->representative_image_filename());
	return 1;
}


/* RST
	.. attribute:: type

			(RO) the name of the building, e.g. building.
*/
int LuaMapObjectDescription::get_type_name(lua_State * L) {
	lua_pushstring(L, to_string(get()->type()));
	return 1;
}

/* RST
BuildingDescription
-------------------

.. class:: BuildingDescription

	A static description of a tribe's building, so it can be used in help files
	without having to access an actual building on the map.
	This class contains the properties that are common to all buildings.
	Further properties are implemented in the subclasses.
	See also class MapObjectDescription for more properties.
*/
const char LuaBuildingDescription::className[] = "BuildingDescription";
const MethodType<LuaBuildingDescription> LuaBuildingDescription::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaBuildingDescription> LuaBuildingDescription::Properties[] = {
	PROP_RO(LuaBuildingDescription, build_cost),
	PROP_RO(LuaBuildingDescription, buildable),
	PROP_RO(LuaBuildingDescription, conquers),
	PROP_RO(LuaBuildingDescription, destructible),
	PROP_RO(LuaBuildingDescription, helptext_script),
	PROP_RO(LuaBuildingDescription, enhanced),
	PROP_RO(LuaBuildingDescription, enhanced_from),
	PROP_RO(LuaBuildingDescription, enhancement_cost),
	PROP_RO(LuaBuildingDescription, enhancement),
	PROP_RO(LuaBuildingDescription, is_mine),
	PROP_RO(LuaBuildingDescription, is_port),
	PROP_RO(LuaBuildingDescription, returned_wares),
	PROP_RO(LuaBuildingDescription, returned_wares_enhanced),
	// TODO(SirVer): size should be similar to
	// https://wl.widelands.org/docs/wl/autogen_wl_map/#wl.map.BaseImmovable.size.
	// In fact, as soon as all descriptions are wrapped (also for other
	// immovables besides buildings) we should get rid of BaseImmovable.size.
	PROP_RO(LuaBuildingDescription, size),
	PROP_RO(LuaBuildingDescription, vision_range),
	PROP_RO(LuaBuildingDescription, workarea_radius),
	{nullptr, nullptr, nullptr},
};


void LuaBuildingDescription::__persist(lua_State* L) {
	const BuildingDescr* descr = get();
	PERS_STRING("name", descr->name());
}

void LuaBuildingDescription::__unpersist(lua_State* L) {
	std::string name;
	UNPERS_STRING("name", name);
	const Tribes& tribes = get_egbase(L).tribes();
	DescriptionIndex idx = tribes.safe_building_index(name.c_str());
	set_description_pointer(tribes.get_building_descr(idx));
}


/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */


/* RST
	.. attribute:: build_cost

			(RO) a list of ware build cost for the building.
*/
int LuaBuildingDescription::get_build_cost(lua_State * L) {
	return wares_or_workers_map_to_lua(L, get()->buildcost(), MapObjectType::WARE);
}


/* RST
	.. attribute:: buildable

			(RO) true if the building can be built.
*/
int LuaBuildingDescription::get_buildable(lua_State * L) {
	lua_pushboolean(L, get()->is_buildable());
	return 1;
}


/* RST
	.. attribute:: conquers

			(RO) the conquer range of the building as an int.
*/
int LuaBuildingDescription::get_conquers(lua_State * L) {
	lua_pushinteger(L, get()->get_conquers());
	return 1;
}



/* RST
	.. attribute:: destructible

			(RO) true if the building is destructible.
*/
int LuaBuildingDescription::get_destructible(lua_State * L) {
	lua_pushboolean(L, get()->is_destructible());
	return 1;
}

/* RST
	.. attribute:: helptext_script

			(RO) The path and filename to the building's helptext script
*/
int LuaBuildingDescription::get_helptext_script(lua_State * L) {
	lua_pushstring(L, get()->helptext_script());
	return 1;
}


/* RST
	.. attribute:: enhanced

			(RO) true if the building is enhanced from another building.
*/
int LuaBuildingDescription::get_enhanced(lua_State * L) {
	lua_pushboolean(L, get()->is_enhanced());
	return 1;
}

/* RST
	.. attribute:: enhanced_from

			(RO) returns the building that this was enhanced from, or nil if this isn't an enhanced building.
*/
int LuaBuildingDescription::get_enhanced_from(lua_State * L) {
	if (get()->is_enhanced()) {
		const DescriptionIndex& enhanced_from = get()->enhanced_from();
		assert(get_egbase(L).tribes().building_exists(enhanced_from));
		return upcasted_map_object_descr_to_lua(L, get_egbase(L).tribes().get_building_descr(enhanced_from));
	}
	lua_pushnil(L);
	return 0;
}


/* RST
	.. attribute:: enhancement_cost

			(RO) a list of ware cost for enhancing to this building type.
*/
int LuaBuildingDescription::get_enhancement_cost(lua_State * L) {
	return wares_or_workers_map_to_lua(L, get()->enhancement_cost(), MapObjectType::WARE);
}

/* RST
	.. attribute:: enhancement

		(RO) a building description that this building can enhance to.
*/
int LuaBuildingDescription::get_enhancement(lua_State * L) {
	const DescriptionIndex enhancement = get()->enhancement();
	if (enhancement == INVALID_INDEX) {
		return 0;
	}
	return upcasted_map_object_descr_to_lua(L, get_egbase(L).tribes().get_building_descr(enhancement));
}


/* RST
	.. attribute:: is_mine

			(RO) true if the building is a mine.
*/
int LuaBuildingDescription::get_is_mine(lua_State * L) {
	lua_pushboolean(L, get()->get_ismine());
	return 1;
}

/* RST
	.. attribute:: is_port

			(RO) true if the building is a port.
*/
int LuaBuildingDescription::get_is_port(lua_State * L) {
	lua_pushboolean(L, get()->get_isport());
	return 1;
}

/* RST
	.. attribute:: returned_wares

			(RO) a list of wares returned upon dismantling.
*/
int LuaBuildingDescription::get_returned_wares(lua_State * L) {
	return wares_or_workers_map_to_lua(L, get()->returned_wares(), MapObjectType::WARE);
}


/* RST
	.. attribute:: returned_wares_enhanced

			(RO) a list of wares returned upon dismantling an enhanced building.
*/
int LuaBuildingDescription::get_returned_wares_enhanced(lua_State * L) {
	return wares_or_workers_map_to_lua(L, get()->returned_wares_enhanced(), MapObjectType::WARE);
}


/* RST
	.. attribute:: size

			(RO) the size of the building: 1 = small, 2 = medium, 3 = big.
*/
int LuaBuildingDescription::get_size(lua_State * L) {
	lua_pushinteger(L, get()->get_size());
	return 1;
}


/* RST
	.. attribute:: vision range

			(RO) the vision_range of the building as an int.
*/
int LuaBuildingDescription::get_vision_range(lua_State * L) {
	lua_pushinteger(L, get()->vision_range());
	return 1;
}

/* RST
	.. attribute:: workarea_radius

			(RO) the workarea_radius of the building as an int.
*/
int LuaBuildingDescription::get_workarea_radius(lua_State * L) {
	lua_pushinteger(L, get()->m_workarea_info.begin()->first);
	return 1;
}

/* RST
ConstructionSiteDescription
---------------------------

.. class:: ConstructionSiteDescription

    A static description of a tribe's constructionsite, so it can be used in help files
    without having to access an actual building on the map.
    See also class BuildingDescription and class MapObjectDescription for more properties.
*/
const char LuaConstructionSiteDescription::className[] = "ConstructionSiteDescription";
const MethodType<LuaConstructionSiteDescription> LuaConstructionSiteDescription::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaConstructionSiteDescription> LuaConstructionSiteDescription::Properties[] = {
	{nullptr, nullptr, nullptr},
};


/* RST
DismantleSiteDescription
---------------------------

.. class:: DismantleSiteDescription

	 A static description of a tribe's dismantlesite, so it can be used in help files
	 without having to access an actual building on the map.
	 See also class BuildingDescription and class MapObjectDescription for more properties.
*/
const char LuaDismantleSiteDescription::className[] = "DismantleSiteDescription";
const MethodType<LuaDismantleSiteDescription> LuaDismantleSiteDescription::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaDismantleSiteDescription> LuaDismantleSiteDescription::Properties[] = {
	{nullptr, nullptr, nullptr},
};



/* RST
ProductionSiteDescription
-------------------------

.. class:: ProductionSiteDescription

	A static description of a tribe's productionsite, so it can be used in help files
	without having to access an actual building on the map.
	This class contains the properties for productionsites that have workers.
	For militarysites and trainingsites, please use the subclasses.
	See also class BuildingDescription and class MapObjectDescription for more properties.
*/
const char LuaProductionSiteDescription::className[] = "ProductionSiteDescription";
const MethodType<LuaProductionSiteDescription> LuaProductionSiteDescription::Methods[] = {
	METHOD(LuaProductionSiteDescription, consumed_wares),
	METHOD(LuaProductionSiteDescription, produced_wares),
	METHOD(LuaProductionSiteDescription, recruited_workers),
	{nullptr, nullptr},
};
const PropertyType<LuaProductionSiteDescription> LuaProductionSiteDescription::Properties[] = {
	PROP_RO(LuaProductionSiteDescription, inputs),
	PROP_RO(LuaProductionSiteDescription, output_ware_types),
	PROP_RO(LuaProductionSiteDescription, output_worker_types),
	PROP_RO(LuaProductionSiteDescription, production_programs),
	PROP_RO(LuaProductionSiteDescription, working_positions),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */


/* RST
	.. attribute:: inputs
		(RO) An array with :class:`LuaWareDescription` containing the wares that
		the productionsite needs for its production.
*/
int LuaProductionSiteDescription::get_inputs(lua_State * L) {
	lua_newtable(L);
	int index = 1;
	for (const WareAmount& input_ware : get()->inputs()) {
		lua_pushint32(L, index++);
		const WareDescr* descr = get_egbase(L).tribes().get_ware_descr(input_ware.first);
		to_lua<LuaWareDescription>(L, new LuaWareDescription(descr));
		lua_settable(L, -3);
	}
	return 1;
}

/* RST
	.. attribute:: output_ware_types
		(RO) An array with :class:`LuaWareDescription` containing the wares that
		the productionsite can produce.
*/
int LuaProductionSiteDescription::get_output_ware_types(lua_State * L) {
	lua_newtable(L);
	int index = 1;
	for (const auto& ware_index : get()->output_ware_types()) {
		lua_pushint32(L, index++);
		const WareDescr* descr = get_egbase(L).tribes().get_ware_descr(ware_index);
		to_lua<LuaWareDescription>(L, new LuaWareDescription(descr));
		lua_rawset(L, -3);
	}

	return 1;
}

/* RST
	.. attribute:: output_worker_types
		(RO) An array with :class:`LuaWorkerDescription` containing the workers that
		the productionsite can produce.
*/
int LuaProductionSiteDescription::get_output_worker_types(lua_State * L) {
	lua_newtable(L);
	int index = 1;
	for (const auto& worker_index : get()->output_worker_types()) {
		lua_pushint32(L, index++);
		const WorkerDescr* descr = get_egbase(L).tribes().get_worker_descr(worker_index);
		to_lua<LuaWorkerDescription>(L, new LuaWorkerDescription(descr));
		lua_rawset(L, -3);
	}

	return 1;
}

/* RST
	.. attribute:: production_programs

		(RO) An array with the production program names as string.
*/
int LuaProductionSiteDescription::get_production_programs(lua_State * L) {
	lua_newtable(L);
	int index = 1;
	for (const auto& program : get()->programs()) {
		lua_pushint32(L, index++);
		lua_pushstring(L, program.first);
		lua_settable(L, -3);
	}
	return 1;
}

/* RST
	.. attribute:: working_positions
		(RO) An array with :class:`WorkerDescription` containing the workers that
		can work here with their multiplicity, i.e. for a atlantean mine this
		would be { miner, miner, miner }.
*/
int LuaProductionSiteDescription::get_working_positions(lua_State * L) {
	lua_newtable(L);
	int index = 1;
	for (const auto& positions_pair : get()->working_positions()) {
		int amount = positions_pair.second;
		while (amount > 0)
		{
			lua_pushint32(L, index++);
			const WorkerDescr* descr = get_egbase(L).tribes().get_worker_descr(positions_pair.first);
			to_lua<LuaWorkerDescription>(L, new LuaWorkerDescription(descr));
			lua_settable(L, -3);
			--amount;
		}
	}
	return 1;
}


/* RST
	.. attribute:: consumed_wares

		:arg program_name: the name of the production program that we want to get the consumed wares for
		:type tribename: :class:`string`

		(RO) Returns a table of {{ware name}, ware amount} for the wares consumed by this production program.
			  Multiple entries in {ware name} are alternatives (OR logic)).
*/
int LuaProductionSiteDescription::consumed_wares(lua_State * L) {
	std::string program_name = luaL_checkstring(L, -1);
	const Widelands::ProductionSiteDescr::Programs & programs = get()->programs();
	if (programs.count(program_name) == 1) {
		const ProductionProgram& program = *programs.at(program_name);
		lua_newtable(L);
		int counter = 0;
		for (const Widelands::ProductionProgram::WareTypeGroup& group: program.consumed_wares()) {
			lua_pushnumber(L, ++counter);
			lua_newtable(L);
			for (const DescriptionIndex& ware_index : group.first) {
				lua_pushstring(L, get_egbase(L).tribes().get_ware_descr(ware_index)->name());
				lua_pushnumber(L, group.second);
				lua_settable(L, -3);
			}
			lua_settable(L, -3);
		}
	}
	return 1;
}


/* RST
	.. attribute:: produced_wares

		:arg program_name: the name of the production program that we want to get the produced wares for
		:type tribename: :class:`string`

		(RO) Returns a table of {ware name, ware amount} for the wares produced by this production program
*/
int LuaProductionSiteDescription::produced_wares(lua_State * L) {
	std::string program_name = luaL_checkstring(L, -1);
	const Widelands::ProductionSiteDescr::Programs & programs = get()->programs();
	if (programs.count(program_name) == 1) {
		const ProductionProgram& program = *programs.at(program_name);
		return wares_or_workers_map_to_lua(L, program.produced_wares(), MapObjectType::WARE);
	}
	return 1;
}

/* RST
	.. attribute:: recruited_workers

		:arg program_name: the name of the production program that we want to get the recruited workers for
		:type tribename: :class:`string`

		(RO) Returns a table of {worker name, worker amount} for the workers recruited
			  by this production program
*/
int LuaProductionSiteDescription::recruited_workers(lua_State * L) {
	std::string program_name = luaL_checkstring(L, -1);
	const Widelands::ProductionSiteDescr::Programs & programs = get()->programs();
	if (programs.count(program_name) == 1) {
		const ProductionProgram& program = *programs.at(program_name);
		return wares_or_workers_map_to_lua(L, program.recruited_workers(), MapObjectType::WORKER);
	}
	return 1;
}


/* RST
MilitarySiteDescription
-----------------------

.. class:: MilitarySiteDescription

	A static description of a tribe's militarysite, so it can be used in help files
	without having to access an actual building on the map.
	A militarysite can garrison and heal soldiers, and it will expand your territory.
	See also class BuildingDescription and class MapObjectDescription for more properties.
*/
const char LuaMilitarySiteDescription::className[] = "MilitarySiteDescription";
const MethodType<LuaMilitarySiteDescription> LuaMilitarySiteDescription::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaMilitarySiteDescription> LuaMilitarySiteDescription::Properties[] = {
	PROP_RO(LuaMilitarySiteDescription, heal_per_second),
	PROP_RO(LuaMilitarySiteDescription, max_number_of_soldiers),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */


/* RST
	.. attribute:: heal_per_second

		(RO) The number of health healed per second by the militarysite
*/
int LuaMilitarySiteDescription::get_heal_per_second(lua_State * L) {
	const MilitarySiteDescr * descr = get();
	lua_pushinteger(L, descr->get_heal_per_second());
	return 1;
}

/* RST
	.. attribute:: max_number_of_soldiers

		(RO) The number of soldiers that can be garrisoned at the militarysite
*/
int LuaMilitarySiteDescription::get_max_number_of_soldiers(lua_State * L) {
	const MilitarySiteDescr * descr = get();
	lua_pushinteger(L, descr->get_max_number_of_soldiers());
	return 1;
}

/* RST
TrainingSiteDescription
-----------------------

.. class:: TrainingSiteDescription

	A static description of a tribe's trainingsite, so it can be used in help files
	without having to access an actual building on the map.
	A training site can train some or all of a soldier's properties (Attack, Defense, Evade and Health).
	See also class BuildingDescription and class MapObjectDescription for more properties.
*/
const char LuaTrainingSiteDescription::className[] = "TrainingSiteDescription";
const MethodType<LuaTrainingSiteDescription> LuaTrainingSiteDescription::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaTrainingSiteDescription> LuaTrainingSiteDescription::Properties[] = {
	PROP_RO(LuaTrainingSiteDescription, food_attack),
	PROP_RO(LuaTrainingSiteDescription, food_defense),
	PROP_RO(LuaTrainingSiteDescription, food_evade),
	PROP_RO(LuaTrainingSiteDescription, food_hp),
	PROP_RO(LuaTrainingSiteDescription, max_attack),
	PROP_RO(LuaTrainingSiteDescription, max_defense),
	PROP_RO(LuaTrainingSiteDescription, max_evade),
	PROP_RO(LuaTrainingSiteDescription, max_hp),
	PROP_RO(LuaTrainingSiteDescription, max_number_of_soldiers),
	PROP_RO(LuaTrainingSiteDescription, min_attack),
	PROP_RO(LuaTrainingSiteDescription, min_defense),
	PROP_RO(LuaTrainingSiteDescription, min_evade),
	PROP_RO(LuaTrainingSiteDescription, min_hp),
	PROP_RO(LuaTrainingSiteDescription, weapons_attack),
	PROP_RO(LuaTrainingSiteDescription, weapons_defense),
	PROP_RO(LuaTrainingSiteDescription, weapons_evade),
	PROP_RO(LuaTrainingSiteDescription, weapons_hp),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */

/* RST
	.. attribute:: food_attack

		(RO) A table of tables with food ware names used for Attack training,
			  e.g. {{"barbarians_bread"}, {"fish", "meat"}}
*/
int LuaTrainingSiteDescription::get_food_attack(lua_State * L) {
	return food_list_to_lua(L, get()->get_food_attack());
}

/* RST
	.. attribute:: food_defense

		(RO) A table of tables with food ware names used for Defense training,
			  e.g. {{"barbarians_bread"}, {"fish", "meat"}}
*/
int LuaTrainingSiteDescription::get_food_defense(lua_State * L) {
	return food_list_to_lua(L, get()->get_food_defense());
}

/* RST
	.. attribute:: food_evade

		(RO) A table of tables with food ware names used for Evade training,
			  e.g. {{"barbarians_bread"}, {"fish", "meat"}}
*/
int LuaTrainingSiteDescription::get_food_evade(lua_State * L) {
	return food_list_to_lua(L, get()->get_food_evade());
}


/* RST
	.. attribute:: food_hp

		(RO) A table of tables with food ware names used for Health training,
			  e.g. {{"barbarians_bread"}, {"fish", "meat"}}
*/
int LuaTrainingSiteDescription::get_food_hp(lua_State * L) {
	return food_list_to_lua(L, get()->get_food_hp());
}


/* RST
	.. attribute:: max_attack

		(RO) The number of attack points that a soldier can train
*/
int LuaTrainingSiteDescription::get_max_attack(lua_State * L) {
	const TrainingSiteDescr* descr = get();
	if (descr->get_train_attack())
		lua_pushinteger(L, descr->get_max_level(atrAttack));
	else
		lua_pushnil(L);
	return 1;
}

/* RST
	.. attribute:: max_defense

		(RO) The number of defense points that a soldier can train
*/
int LuaTrainingSiteDescription::get_max_defense(lua_State * L) {
	const TrainingSiteDescr* descr = get();
	if (descr->get_train_defense())
		lua_pushinteger(L, descr->get_max_level(atrDefense));
	else
		lua_pushnil(L);
	return 1;
}


/* RST
	.. attribute:: max_evade

		(RO) The number of evade points that a soldier can train
*/
int LuaTrainingSiteDescription::get_max_evade(lua_State * L) {
	const TrainingSiteDescr * descr = get();
	if (descr->get_train_evade())
		lua_pushinteger(L, descr->get_max_level(atrEvade));
	else lua_pushnil(L);
	return 1;
}


/* RST
	.. attribute:: max_hp

		(RO) The number of health points that a soldier can train
*/
int LuaTrainingSiteDescription::get_max_hp(lua_State * L) {
	const TrainingSiteDescr * descr = get();
	if (descr->get_train_hp())
		lua_pushinteger(L, descr->get_max_level(atrHP));
	else lua_pushnil(L);
	return 1;
}


/* RST
	.. attribute:: max_number_of_soldiers

		(RO) The number of soldiers that can be garrisoned at the trainingsite
*/
int LuaTrainingSiteDescription::get_max_number_of_soldiers(lua_State * L) {
	const TrainingSiteDescr * descr = get();
	lua_pushinteger(L, descr->get_max_number_of_soldiers());
	return 1;
}


/* RST
	.. attribute:: min_attack

		(RO) The number of attack points that a soldier starts training with
*/
int LuaTrainingSiteDescription::get_min_attack(lua_State * L) {
	const TrainingSiteDescr * descr = get();
	if (descr->get_train_attack())
		lua_pushinteger(L, descr->get_min_level(atrAttack));
	else lua_pushnil(L);
	return 1;
}

/* RST
	.. attribute:: min_defense

		(RO) The number of defense points that a soldier starts training with
*/
int LuaTrainingSiteDescription::get_min_defense(lua_State * L) {
	const TrainingSiteDescr * descr = get();
	if (descr->get_train_defense())
		lua_pushinteger(L, descr->get_min_level(atrDefense));
	else lua_pushnil(L);
	return 1;
}


/* RST
	.. attribute:: min_evade

		(RO) The number of evade points that a soldier starts training with
*/
int LuaTrainingSiteDescription::get_min_evade(lua_State * L) {
	const TrainingSiteDescr * descr = get();
	if (descr->get_train_evade())
		lua_pushinteger(L, descr->get_min_level(atrEvade));
	else lua_pushnil(L);
	return 1;
}


/* RST
	.. attribute:: min_hp

		(RO) The number of health points that a soldier starts training with
*/
int LuaTrainingSiteDescription::get_min_hp(lua_State * L) {
	const TrainingSiteDescr * descr = get();
	if (descr->get_train_hp())
		lua_pushinteger(L, descr->get_min_level(atrHP));
	else lua_pushnil(L);
	return 1;
}

/* RST
	.. attribute:: weapons_attack

		(RO) A table with weapon ware names used for Attack training
*/
int LuaTrainingSiteDescription::get_weapons_attack(lua_State * L) {
	lua_newtable(L);
	int counter = 0;
	for (const std::string& weaponname : get()->get_weapons_attack()) {
		lua_pushnumber(L, ++counter);
		lua_pushstring(L, weaponname);
		lua_settable(L, -3);
	}
	return 1;
}

/* RST
	.. attribute:: weapons_defense

		(RO) A table with weapon ware names used for Defense training
*/
int LuaTrainingSiteDescription::get_weapons_defense(lua_State * L) {
	lua_newtable(L);
	int counter = 0;
	for (const std::string& weaponname : get()->get_weapons_defense()) {
		lua_pushnumber(L, ++counter);
		lua_pushstring(L, weaponname);
		lua_settable(L, -3);
	}
	return 1;
}

/* RST
	.. attribute:: weapons_evade

		(RO) A table with weapon ware names used for Evade training
*/
int LuaTrainingSiteDescription::get_weapons_evade(lua_State * L) {
	lua_newtable(L);
	int counter = 0;
	for (const std::string& weaponname : get()->get_weapons_evade()) {
		lua_pushnumber(L, ++counter);
		lua_pushstring(L, weaponname);
		lua_settable(L, -3);
	}
	return 1;
}

/* RST
	.. attribute:: weapons_hp

		(RO) A table with weapon ware names used for Health training
*/
int LuaTrainingSiteDescription::get_weapons_hp(lua_State * L) {
	lua_newtable(L);
	int counter = 0;
	for (const std::string& weaponname : get()->get_weapons_hp()) {
		lua_pushnumber(L, ++counter);
		lua_pushstring(L, weaponname);
		lua_settable(L, -3);
	}
	return 1;
}


/* RST
WarehouseDescription
--------------------

.. class:: WarehouseDescription

	A static description of a tribe's warehouse, so it can be used in help files
	without having to access an actual building on the map.
	Note that headquarters are also warehouses.
	A warehouse keeps people, animals and wares.
	See also class BuildingDescription and class MapObjectDescription for more properties.
*/
const char LuaWarehouseDescription::className[] = "WarehouseDescription";
const MethodType<LuaWarehouseDescription> LuaWarehouseDescription::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaWarehouseDescription> LuaWarehouseDescription::Properties[] = {
	PROP_RO(LuaWarehouseDescription, heal_per_second),
	{nullptr, nullptr, nullptr},
};


/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */


/* RST
	.. attribute:: heal_per_second

		(RO) The number of health healed per second by the warehouse
*/
int LuaWarehouseDescription::get_heal_per_second(lua_State * L) {
	const WarehouseDescr * descr = get();
	lua_pushinteger(L, descr->get_heal_per_second());
	return 1;
}

/* RST
WareDescription
---------------

.. class:: WareDescription

	A static description of a tribe's ware, so it can be used in help files
	without having to access an actual instance of the ware on the map.
	See also class MapObjectDescription for more properties.
*/
const char LuaWareDescription::className[] = "WareDescription";
const MethodType<LuaWareDescription> LuaWareDescription::Methods[] = {
	METHOD(LuaWareDescription, is_construction_material),
	{nullptr, nullptr},
};
const PropertyType<LuaWareDescription> LuaWareDescription::Properties[] = {
	PROP_RO(LuaWareDescription, consumers),
	PROP_RO(LuaWareDescription, helptext_script),
	PROP_RO(LuaWareDescription, producers),
	{nullptr, nullptr, nullptr},
};


void LuaWareDescription::__persist(lua_State* L) {
	const WareDescr* descr = get();
	PERS_STRING("name", descr->name());
}

void LuaWareDescription::__unpersist(lua_State* L) {
	std::string name;
	UNPERS_STRING("name", name);
	DescriptionIndex idx = get_egbase(L).tribes().safe_ware_index(name.c_str());
	set_description_pointer(get_egbase(L).tribes().get_ware_descr(idx));
}


/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */


/* RST
	.. attribute:: consumers
		(RO) An array with :class:`LuaBuildingDescription` with buildings that
		need this ware for their production.
*/
int LuaWareDescription::get_consumers(lua_State * L) {
	lua_newtable(L);
	int index = 1;
	for (const DescriptionIndex& building_index : get()->consumers()) {
		lua_pushint32(L, index++);
		upcasted_map_object_descr_to_lua(L, get_egbase(L).tribes().get_building_descr(building_index));
		lua_rawset(L, -3);
	}
	return 1;
}

/* RST
	.. attribute:: helptext_script

			(RO) The path and filename to the ware's helptext script
*/
int LuaWareDescription::get_helptext_script(lua_State * L) {
	lua_pushstring(L, get()->helptext_script());
	return 1;
}


/* RST
	.. attribute:: is_construction_material

		:arg tribename: the name of the tribe that this ware gets checked for
		:type tribename: :class:`string`

		(RO) A bool that is true if this ware is used by the tribe's construction sites.
*/
int LuaWareDescription::is_construction_material(lua_State * L) {
	std::string tribename = luaL_checkstring(L, -1);
	const Tribes& tribes = get_egbase(L).tribes();
	if (tribes.tribe_exists(tribename)) {
		const DescriptionIndex& ware_index = tribes.safe_ware_index(get()->name());
		int tribeindex = tribes.tribe_index(tribename);
		lua_pushboolean(L, tribes.get_tribe_descr(tribeindex)->is_construction_material(ware_index));
	} else {
		lua_pushboolean(L, false);
	}
	return 1;
}



/* RST
	.. attribute:: producers
		(RO) An array with :class:`LuaBuildingDescription` with buildings that
		can procude this ware.
*/
int LuaWareDescription::get_producers(lua_State * L) {
	lua_newtable(L);
	int index = 1;
	for (const DescriptionIndex& building_index : get()->producers()) {
		lua_pushint32(L, index++);
		upcasted_map_object_descr_to_lua(L, get_egbase(L).tribes().get_building_descr(building_index));
		lua_rawset(L, -3);
	}
	return 1;
}



/* RST
WorkerDescription
-----------------

.. class:: WorkerDescription

	A static description of a tribe's worker, so it can be used in help files
	without having to access an actual instance of the worker on the map.
	See also class MapObjectDescription for more properties.
*/
const char LuaWorkerDescription::className[] = "WorkerDescription";
const MethodType<LuaWorkerDescription> LuaWorkerDescription::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaWorkerDescription> LuaWorkerDescription::Properties[] = {
	PROP_RO(LuaWorkerDescription, becomes),
	PROP_RO(LuaWorkerDescription, buildcost),
	PROP_RO(LuaWorkerDescription, helptext_script),
	PROP_RO(LuaWorkerDescription, is_buildable),
	PROP_RO(LuaWorkerDescription, needed_experience),
	{nullptr, nullptr, nullptr},
};


void LuaWorkerDescription::__persist(lua_State* L) {
	const WorkerDescr * descr = get();
	PERS_STRING("name", descr->name());
}

void LuaWorkerDescription::__unpersist(lua_State* L) {
	std::string name;
	UNPERS_STRING("name", name);
	const Tribes& tribes = get_egbase(L).tribes();
	DescriptionIndex idx = tribes.safe_worker_index(name.c_str());
	set_description_pointer(tribes.get_worker_descr(idx));
}

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */


/* RST
	.. attribute:: becomes

		(RO) The :class:`WorkerDescription` of the worker this one will level up
		to or :const:`nil` if it never levels up.
*/
int LuaWorkerDescription::get_becomes(lua_State * L) {
	const DescriptionIndex becomes_index = get()->becomes();
	if (becomes_index == INVALID_INDEX) {
		lua_pushnil(L);
		return 1;
	}
	return to_lua<LuaWorkerDescription>(
		L, new LuaWorkerDescription(get_egbase(L).tribes().get_worker_descr(becomes_index)));
}


/* RST
	.. attribute:: buildcost

		(RO) a list of building requirements, e.g. {"atlanteans_carrier", "ax"}
*/
int LuaWorkerDescription::get_buildcost(lua_State * L) {
	lua_newtable(L);
	int index = 1;
	if (get()->is_buildable()) {
		for (const std::pair<std::string, uint8_t>& buildcost_pair : get()->buildcost()) {
			lua_pushint32(L, index++);
			lua_pushstring(L, buildcost_pair.first);
			lua_settable(L, -3);
		}
	}
	return 1;
}

/* RST
	.. attribute:: helptext_script

			(RO) The path and filename to the worker's helptext script
*/
int LuaWorkerDescription::get_helptext_script(lua_State * L) {
	lua_pushstring(L, get()->helptext_script());
	return 1;
}

/* RST
	.. attribute:: is_buildable

		(RO) returns true if this worker is buildable
*/
int LuaWorkerDescription::get_is_buildable(lua_State * L) {
	lua_pushboolean(L, get()->is_buildable());
	return 1;
}


/* RST
	.. attribute:: needed_experience

			(RO) the experience the worker needs to reach this level.
*/
int LuaWorkerDescription::get_needed_experience(lua_State * L) {
	lua_pushinteger(L, get()->get_needed_experience());
	return 1;
}



/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */



/*
 ==========================================================
 C METHODS
 ==========================================================
 */



/* RST
MapObject
---------

.. class:: MapObject

	This is the base class for all Objects in widelands, including immovables
	and Bobs. This class can't be instantiated directly, but provides the base
	for all others.
*/
const char LuaMapObject::className[] = "MapObject";
const MethodType<LuaMapObject> LuaMapObject::Methods[] = {
	METHOD(LuaMapObject, remove),
	METHOD(LuaMapObject, destroy),
	METHOD(LuaMapObject, __eq),
	METHOD(LuaMapObject, has_attribute),
	{nullptr, nullptr},
};
const PropertyType<LuaMapObject> LuaMapObject::Properties[] = {
	PROP_RO(LuaMapObject, __hash),
	PROP_RO(LuaMapObject, descr),
	PROP_RO(LuaMapObject, serial),
	{nullptr, nullptr, nullptr},
};

void LuaMapObject::__persist(lua_State * L) {
	MapObjectSaver & mos = *get_mos(L);
	Game & game = get_game(L);

	uint32_t idx = 0;
	if (MapObject* obj = m_ptr.get(game))
		idx = mos.get_object_file_index(*obj);

	PERS_UINT32("file_index", idx);
}
void LuaMapObject::__unpersist(lua_State* L) {
	uint32_t idx;
	UNPERS_UINT32("file_index", idx);

	if (!idx)
		m_ptr = nullptr;
	else {
		MapObjectLoader& mol = *get_mol(L);
		m_ptr = &mol.get<MapObject>(idx);
	}
}

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
// Hash is used to identify a class in a Set
int LuaMapObject::get___hash(lua_State * L) {
	lua_pushuint32(L, get(L, get_egbase(L))->serial());
	return 1;
}

/* RST
	.. attribute:: serial

		(RO)
		The serial number of this object. Note that this value does not stay
		constant after saving/loading.
*/
int LuaMapObject::get_serial(lua_State * L) {
	lua_pushuint32(L, get(L, get_egbase(L))->serial());
	return 1;
}


// use the dynamic type of BuildingDescription
#define CAST_TO_LUA(klass, lua_klass) to_lua<lua_klass> \
	(L, new lua_klass(static_cast<const klass *>(desc)))


/* RST
    .. attribute:: descr

		  (RO) The description object for this immovable, e.g. BuildingDescription.
*/
int LuaMapObject::get_descr(lua_State * L) {
	const MapObjectDescr* desc = &get(L, get_egbase(L))->descr();
	assert(desc != nullptr);

	switch (desc->type()) {
		case (MapObjectType::BUILDING):
			return CAST_TO_LUA(BuildingDescr, LuaBuildingDescription);
		case (MapObjectType::CONSTRUCTIONSITE):
			return CAST_TO_LUA(ConstructionSiteDescr, LuaConstructionSiteDescription);
		case (MapObjectType::DISMANTLESITE):
			return CAST_TO_LUA(DismantleSiteDescr, LuaDismantleSiteDescription);
		case (MapObjectType::PRODUCTIONSITE):
			return CAST_TO_LUA(ProductionSiteDescr, LuaProductionSiteDescription);
		case (MapObjectType::MILITARYSITE):
			return CAST_TO_LUA(MilitarySiteDescr, LuaMilitarySiteDescription);
		case (MapObjectType::TRAININGSITE):
			return CAST_TO_LUA(TrainingSiteDescr, LuaTrainingSiteDescription);
		case (MapObjectType::WAREHOUSE):
			return CAST_TO_LUA(WarehouseDescr, LuaWarehouseDescription);
		default:
			return CAST_TO_LUA(MapObjectDescr, LuaMapObjectDescription);
	}
}

#undef CAST_TO_LUA


/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */
int LuaMapObject::__eq(lua_State * L) {
	EditorGameBase & egbase = get_egbase(L);
	LuaMapObject * other = *get_base_user_class<LuaMapObject>(L, -1);

	MapObject * me = m_get_or_zero(egbase);
	MapObject * you = other->m_get_or_zero(egbase);

	// Both objects are destroyed (nullptr) or equal: they are equal
	if (me == you) {
		lua_pushboolean(L, true);
	} else if (me == nullptr || you == nullptr) { // One of the objects is destroyed: they are distinct
		lua_pushboolean(L, false);
	} else { // Compare their serial number.
		lua_pushboolean
			(L, other->get(L, egbase)->serial() == get(L, egbase)->serial());
	}

	return 1;
}

/* RST
	.. method:: remove()

		Removes this object immediately. If you want to destroy an
		object as if the player had see :func:`destroy`.
*/
int LuaMapObject::remove(lua_State * L) {
	EditorGameBase & egbase = get_egbase(L);
	MapObject* o = get(L, egbase);
	if (!o)
		return 0;

	o->remove(egbase);
	return 0;
}

/* RST
	.. method:: destroy()

		Removes this object immediately. Might do special actions (like leaving a
		burning fire). If you want to remove an object without side effects, see
		:func:`remove`.
*/
int LuaMapObject::destroy(lua_State * L) {
	EditorGameBase& egbase = get_egbase(L);
	MapObject* o = get(L, egbase);
	if (!o)
		return 0;

	o->destroy(egbase);
	return 0;
}

/* RST
	.. method:: has_attribute(string)

		returns true, if the map object has this attribute, else false
*/
int LuaMapObject::has_attribute(lua_State * L) {
	EditorGameBase & egbase = get_egbase(L);
	MapObject * obj = m_get_or_zero(egbase);
	if (!obj) {
		lua_pushboolean(L, false);
		return 1;
	}

	// Check if object has the attribute
	std::string attrib = luaL_checkstring(L, 2);
	if (obj->has_attribute(MapObjectDescr::get_attribute_id(attrib)))
		lua_pushboolean(L, true);
	else
		lua_pushboolean(L, false);
	return 1;
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */
MapObject* LuaMapObject::get(lua_State* L, EditorGameBase& egbase, std::string name) {
	MapObject* o = m_get_or_zero(egbase);
	if (!o)
		report_error(L, "%s no longer exists!", name.c_str());
	return o;
}
MapObject* LuaMapObject::m_get_or_zero(EditorGameBase& egbase) {
	return m_ptr.get(egbase);
}

/* RST
BaseImmovable
-------------

.. class:: BaseImmovable

	Child of: :class:`MapObject`

	This is the base class for all Immovables in widelands.
*/
const char LuaBaseImmovable::className[] = "BaseImmovable";
const MethodType<LuaBaseImmovable> LuaBaseImmovable::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaBaseImmovable> LuaBaseImmovable::Properties[] = {
	PROP_RO(LuaBaseImmovable, size),
	PROP_RO(LuaBaseImmovable, fields),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
/* RST
	.. attribute:: size

		(RO) The size of this immovable. Can be either of

		* :const:`none` -- Example: mushrooms. Immovables will be destroyed when
			something else is build on this field.
		* :const:`small` -- Example: trees or flags
		* :const:`medium` -- Example: Medium sized buildings
		* :const:`big` -- Example: Big sized buildings or rocks
*/
int LuaBaseImmovable::get_size(lua_State * L) {
	BaseImmovable * o = get(L, get_egbase(L));

	switch (o->get_size()) {
		case BaseImmovable::NONE: lua_pushstring(L, "none"); break;
		case BaseImmovable::SMALL: lua_pushstring(L, "small"); break;
		case BaseImmovable::MEDIUM: lua_pushstring(L, "medium"); break;
		case BaseImmovable::BIG: lua_pushstring(L, "big"); break;
		default:
			report_error(L, "Unknown size in LuaBaseImmovable::get_size: %i", o->get_size());
	}
	return 1;
}

/* RST
	.. attribute:: fields

		(RO) An :class:`array` of :class:`~wl.map.Field` that is occupied by this
		Immovable. If the immovable occupies more than one field (roads or big
		buildings for example) the first entry in this list will be the main field
*/
int LuaBaseImmovable::get_fields(lua_State * L) {
	EditorGameBase & egbase = get_egbase(L);

	BaseImmovable::PositionList pl = get(L, egbase)->get_positions(egbase);

	lua_createtable(L, pl.size(), 0);
	uint32_t idx = 1;
	for (const Coords& coords : pl) {
		lua_pushuint32(L, idx++);
		to_lua<LuaField>(L, new LuaField(coords.x, coords.y));
		lua_rawset(L, -3);
	}
	return 1;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

/*
 ==========================================================
 C METHODS
 ==========================================================
 */



/* RST
PlayerImmovable
---------------

.. class:: PlayerImmovable

	Child of: :class:`BaseImmovable`

	All Immovables that belong to a Player (Buildings, Flags, ...) are based on
	this Class.
*/
const char LuaPlayerImmovable::className[] = "PlayerImmovable";
const MethodType<LuaPlayerImmovable> LuaPlayerImmovable::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaPlayerImmovable> LuaPlayerImmovable::Properties[] = {
	PROP_RO(LuaPlayerImmovable, owner),
	PROP_RO(LuaPlayerImmovable, debug_economy),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
/* RST
	.. attribute:: owner

		(RO) The :class:`wl.game.Player` who owns this object.
*/
int LuaPlayerImmovable::get_owner(lua_State * L) {
	get_factory(L).push_player
		(L, get(L, get_egbase(L))->get_owner()->player_number());
	return 1;
}

// UNTESTED, for debug only
int LuaPlayerImmovable::get_debug_economy(lua_State* L) {
	lua_pushlightuserdata(L, get(L, get_egbase(L))->get_economy());
	return 1;
}


/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

/*
 ==========================================================
 C METHODS
 ==========================================================
 */

/* RST
Flag
--------

.. class:: Flag

	Child of: :class:`PlayerImmovable`, :class:`HasWares`

	One flag in the economy of this Player.
*/
const char LuaFlag::className[] = "Flag";
const MethodType<LuaFlag> LuaFlag::Methods[] = {
	METHOD(LuaFlag, set_wares),
	METHOD(LuaFlag, get_wares),
	{nullptr, nullptr},
};
const PropertyType<LuaFlag> LuaFlag::Properties[] = {
	PROP_RO(LuaFlag, roads),
	PROP_RO(LuaFlag, building),
	{nullptr, nullptr, nullptr},
};


/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
/* RST
	.. attribute:: roads

		(RO) Array of roads leading to the flag. Directions
		can be tr,r,br,bl,l and tl

		:returns: The array of 'direction:road', if any
*/
int LuaFlag::get_roads(lua_State * L) {

		const std::vector<std::string> directions = {"tr", "r", "br", "bl", "l", "tl"};

		lua_newtable(L);

		EditorGameBase & egbase = get_egbase(L);
		Flag * f = get(L, egbase);

		for (uint32_t i = 1; i <= 6; i++){
 	       if (f->get_road(i) != nullptr)  {
				lua_pushstring(L, directions.at(i - 1));
				upcasted_map_object_to_lua(L, f->get_road(i));
				lua_rawset(L, -3);
			}
		}
		return 1;
}

/* RST
	.. attribute:: building

		(RO) building belonging to the flag
*/
int LuaFlag::get_building(lua_State * L) {

	EditorGameBase & egbase = get_egbase(L);
	Flag * f = get(L, egbase);

	PlayerImmovable * building = f->get_building();
	if (!building)  {
		return 0;
	} else {
		upcasted_map_object_to_lua(L, building);
	}
	return 1;

}
/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */
// Documented in ParentClass
int LuaFlag::set_wares(lua_State * L)
{
	EditorGameBase & egbase = get_egbase(L);
	Flag * f = get(L, egbase);
	const Tribes& tribes = egbase.tribes();

	WaresMap setpoints = m_parse_set_wares_arguments(L, f->owner().tribe());
	WaresMap c_wares = count_wares_on_flag_(*f, tribes);

	uint32_t nwares = 0;

	for (const std::pair<Widelands::DescriptionIndex, uint32_t>& ware : c_wares) {
		// all wares currently on the flag without a setpoint should be removed
		if (!setpoints.count(ware.first))
			setpoints.insert(Widelands::WareAmount(ware.first, 0));
		nwares += ware.second;
	}

	// The idea is to change as little as possible on this flag
	for (const std::pair<Widelands::DescriptionIndex, uint32_t>& sp : setpoints) {
		uint32_t cur = 0;
		WaresMap::iterator i = c_wares.find(sp.first);
		if (i != c_wares.end())
			cur = i->second;

		int d = sp.second - cur;
		nwares += d;

		if (f->total_capacity() < nwares)
			report_error(L, "Flag has no capacity left!");

		if (d < 0) {
			while (d) {
				for (const WareInstance * ware : f->get_wares()) {
					if (tribes.ware_index(ware->descr().name()) == sp.first) {
						const_cast<WareInstance *>(ware)->remove(egbase);
						++d;
						break;
					}
				}
			}
		} else if (d > 0) {
			// add wares
			const WareDescr & wd = *tribes.get_ware_descr(sp.first);
			for (int32_t j = 0; j < d; j++) {
				WareInstance & ware = *new WareInstance(sp.first, &wd);
				ware.init(egbase);
				f->add_ware(egbase, ware);
			}
		}

	}
	return 0;
}

// Documented in parent Class
int LuaFlag::get_wares(lua_State * L) {
	EditorGameBase& egbase = get_egbase(L);
	const Tribes& tribes = egbase.tribes();
	Flag * flag = get(L, egbase);

	bool return_number = false;
	WaresSet wares_set = m_parse_get_wares_arguments(L, flag->owner().tribe(), &return_number);

	WaresMap wares = count_wares_on_flag_(*flag, tribes);

	if (wares_set.size() == flag->owner().tribe().get_nrwares()) { // Want all returned
		wares_set.clear();

		for (const std::pair<Widelands::DescriptionIndex, uint32_t>& ware : wares) {
			wares_set.insert(ware.first);
		}
	}

	if (!return_number)
		lua_newtable(L);

	for (const Widelands::DescriptionIndex& ware : wares_set) {
		uint32_t count = 0;
		if (wares.count(ware))
			count = wares[ware];

		if (return_number) {
			lua_pushuint32(L, count);
			break;
		} else {
			lua_pushstring(L, tribes.get_ware_descr(ware)->name());
			lua_pushuint32(L, count);
			lua_rawset(L, -3);
		}
	}
	return 1;
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */


/* RST
Road
----

.. class:: Road

	Child of: :class:`PlayerImmovable`, :class:`HasWorkers`

	One flag in the economy of this Player.
*/
const char LuaRoad::className[] = "Road";
const MethodType<LuaRoad> LuaRoad::Methods[] = {
	METHOD(LuaRoad, get_workers),
	METHOD(LuaRoad, set_workers),
	{nullptr, nullptr},
};
const PropertyType<LuaRoad> LuaRoad::Properties[] = {
	PROP_RO(LuaRoad, length),
	PROP_RO(LuaRoad, start_flag),
	PROP_RO(LuaRoad, end_flag),
	PROP_RO(LuaRoad, valid_workers),
	PROP_RO(LuaRoad, road_type),
	{nullptr, nullptr, nullptr},
};


/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
/* RST
	.. attribute:: length

		(RO) The length of the roads in number of edges.
*/
int LuaRoad::get_length(lua_State * L) {
	lua_pushuint32(L, get(L, get_egbase(L))->get_path().get_nsteps());
	return 1;
}

/* RST
	.. attribute:: start_flag

		(RO) The flag were this road starts
*/
int LuaRoad::get_start_flag(lua_State * L) {
	return
		to_lua<LuaFlag>
			(L, new LuaFlag(get(L, get_egbase(L))->get_flag(Road::FlagStart)));
}

/* RST
	.. attribute:: end_flag

		(RO) The flag were this road ends
*/
int LuaRoad::get_end_flag(lua_State * L) {
	return
		to_lua<LuaFlag>
			(L, new LuaFlag(get(L, get_egbase(L))->get_flag(Road::FlagEnd)));
}

/* RST
	.. attribute:: road_type

		(RO) Type of road. Can be any either of:

		* normal
		* busy
*/
int LuaRoad::get_road_type(lua_State * L) {
	switch (get(L, get_egbase(L))->get_roadtype()) {
		case RoadType::kNormal:
			lua_pushstring(L, "normal"); break;
		case RoadType::kBusy:
			lua_pushstring(L, "busy"); break;
		default:
		   report_error(L, "Unknown Roadtype! This is a bug in widelands!");
	}
	return 1;
}

// documented in parent class
int LuaRoad::get_valid_workers(lua_State* L) {
	Road* road = get(L, get_egbase(L));
	return workers_map_to_lua(L, get_valid_workers_for(*road));
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

// documented in parent class
int LuaRoad::get_workers(lua_State* L) {
	Road* road = get(L, get_egbase(L));
	return do_get_workers(L, *road, get_valid_workers_for(*road));
}

int LuaRoad::set_workers(lua_State* L) {
	Road* road = get(L, get_egbase(L));
	return do_set_workers<LuaRoad>(L, road, get_valid_workers_for(*road));
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */

int LuaRoad::create_new_worker
	(PlayerImmovable & pi, EditorGameBase & egbase, const WorkerDescr * wdes)
{
	Road & r = static_cast<Road &>(pi);

	if (r.get_workers().size())
		return -1; // No space

	// Determine Idle position.
	Flag & start = r.get_flag(Road::FlagStart);
	Coords idle_position = start.get_position();
	const Path & path = r.get_path();
	Path::StepVector::size_type idle_index = r.get_idle_index();
	for (Path::StepVector::size_type i = 0; i < idle_index; ++i)
		egbase.map().get_neighbour(idle_position, path[i], &idle_position);

	Carrier& carrier = dynamic_cast<Carrier&>
		(wdes->create(egbase, r.owner(), &r, idle_position));

	if (upcast(Game, game, &egbase)) {
		carrier.start_task_road(*game);
	}

	r.assign_carrier(carrier, 0);
	return 0;
}


/* RST
PortDock
--------

.. class:: PortDock

	Child of: :class:`PlayerImmovable`

	Each :class:`Warehouse` that is a port has a dock attached to
	it. The PortDock is an immovable that also occupies a field on
	the water near the port.
*/

const char LuaPortDock::className[] = "PortDock";
const MethodType<LuaPortDock> LuaPortDock::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaPortDock> LuaPortDock::Properties[] = {
	{nullptr, nullptr, nullptr},
};


/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */


/*
 ==========================================================
 C METHODS
 ==========================================================
 */

/* RST
Building
--------

.. class:: Building

	Child of: :class:`PlayerImmovable`

	This represents a building owned by a player.
*/
const char LuaBuilding::className[] = "Building";
const MethodType<LuaBuilding> LuaBuilding::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaBuilding> LuaBuilding::Properties[] = {
	PROP_RO(LuaBuilding, flag),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */

/* RST
	.. attribute:: flag

		(RO) The flag that belongs to this building (that is to the bottom right
		of it's main location).
*/
// UNTESTED
int LuaBuilding::get_flag(lua_State * L) {
	return upcasted_map_object_to_lua(L, &get(L, get_egbase(L))->base_flag());
}


/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

/*
 ==========================================================
 C METHODS
 ==========================================================
 */

/* RST
ConstructionSite
-----------------

.. class:: ConstructionSite

	Child of: :class:`Building`

	A ConstructionSite as it appears in Game. This is only a minimal wrapping at
	the moment
*/
const char LuaConstructionSite::className[] = "ConstructionSite";
const MethodType<LuaConstructionSite> LuaConstructionSite::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaConstructionSite> LuaConstructionSite::Properties[] = {
	PROP_RO(LuaConstructionSite, building),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
/* RST
	.. attribute:: building

		(RO) The name of the building that is constructed here
*/
int LuaConstructionSite::get_building(lua_State * L) {
	lua_pushstring(L, get(L, get_egbase(L))->building().name());
	return 1;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

/*
 ==========================================================
 C METHODS
 ==========================================================
 */




/* RST
Warehouse
---------

.. class:: Warehouse

	Child of: :class:`Building`, :class:`HasWares`, :class:`HasWorkers`,
	:class:`HasSoldiers`

	Every Headquarter or Warehouse on the Map is of this type.
*/
const char LuaWarehouse::className[] = "Warehouse";
const MethodType<LuaWarehouse> LuaWarehouse::Methods[] = {
	METHOD(LuaWarehouse, set_wares),
	METHOD(LuaWarehouse, get_wares),
	METHOD(LuaWarehouse, set_workers),
	METHOD(LuaWarehouse, get_workers),
	METHOD(LuaWarehouse, set_soldiers),
	METHOD(LuaWarehouse, get_soldiers),
	METHOD(LuaWarehouse, start_expedition),
	METHOD(LuaWarehouse, cancel_expedition),
	{nullptr, nullptr},
};
const PropertyType<LuaWarehouse> LuaWarehouse::Properties[] = {
	PROP_RO(LuaWarehouse, portdock),
	PROP_RO(LuaWarehouse, expedition_in_progress),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
// UNTESTED
/* RST
	.. attribute:: portdock

		(RO) If this Warehouse is a port, returns the
		:class:`PortDock` attached to it, otherwise nil.
*/
int LuaWarehouse::get_portdock(lua_State * L) {
	return upcasted_map_object_to_lua(L, get(L, get_egbase(L))->get_portdock());
}

/* RST
	.. attribute:: expedition_in_progress

		(RO) If this Warehouse is a port, and an expedition is in
		progress, returns true, otherwise nil
*/
int LuaWarehouse::get_expedition_in_progress(lua_State * L) {

	Warehouse* wh = get(L, get_egbase(L));

	if (is_a(Game, &get_egbase(L))) {
		PortDock* pd = wh->get_portdock();
		if (pd) {
			if (pd->expedition_started()){
				return 1;
			}
		}
	}
	return 0;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */
#define WH_SET(type, btype) \
int LuaWarehouse::set_##type##s(lua_State * L) { \
	Warehouse * wh = get(L, get_egbase(L)); \
	btype##sMap setpoints = m_parse_set_##type##s_arguments(L, wh->owner().tribe()); \
 \
	for (btype##sMap::iterator i = setpoints.begin(); i != setpoints.end(); ++i) { \
		int32_t d = i->second - \
			wh->get_##type##s().stock(i->first); \
		if (d < 0) \
			wh->remove_##type##s(i->first, -d); \
		else if (d > 0) \
			wh->insert_##type##s(i->first, d); \
	} \
	return 0; \
}
// documented in parent class
WH_SET(ware, Ware)
// documented in parent class
WH_SET(worker, Worker)
#undef WH_SET

#define WH_GET(type, btype) \
int LuaWarehouse::get_##type##s(lua_State * L) { \
	Warehouse * wh = get(L, get_egbase(L)); \
	const Tribes& tribes = get_egbase(L).tribes(); \
	bool return_number = false; \
	btype##sSet set = m_parse_get_##type##s_arguments \
		(L, wh->owner().tribe(), &return_number); \
	lua_newtable(L); \
	if (return_number) \
		lua_pushuint32(L, wh->get_##type##s().stock(*set.begin())); \
	else { \
		lua_newtable(L); \
		for (btype##sSet::iterator i = set.begin(); i != set.end(); ++i) { \
			lua_pushstring(L, tribes.get_##type##_descr(*i)->name()); \
			lua_pushuint32(L, wh->get_##type##s().stock(*i)); \
			lua_rawset(L, -3); \
		} \
	} \
	return 1; \
}
// documented in parent class
WH_GET(ware, Ware)
// documented in parent class
WH_GET(worker, Worker)
#undef GET

// documented in parent class
int LuaWarehouse::get_soldiers(lua_State* L) {
	Warehouse* wh = get(L, get_egbase(L));
	return do_get_soldiers(L, *wh, wh->owner().tribe());
}

// documented in parent class
int LuaWarehouse::set_soldiers(lua_State* L) {
	Warehouse* wh = get(L, get_egbase(L));
	return do_set_soldiers(L, wh->get_position(), wh, wh->get_owner());
}

/* RST
	.. method:: start_expedition(port)

		:arg port

		Starts preparation for expedition

*/
int LuaWarehouse::start_expedition(lua_State* L) {

	Warehouse* Wh = get(L, get_egbase(L));

	if (!Wh) {
		return 0;
	}

	if (upcast(Game, game, &get_egbase(L))) {
		PortDock* pd = Wh->get_portdock();
		if (!pd) {
			return 0;
		}
		if (!pd->expedition_started()){
			game->send_player_start_or_cancel_expedition(*Wh);
			return 1;
		}
	}

	return 0;
}

/* RST
	.. method:: cancel_expedition(port)

		:arg port

		Cancels an expedition if in progress

*/
int LuaWarehouse::cancel_expedition(lua_State* L) {

	Warehouse* Wh = get(L, get_egbase(L));

	if (!Wh) {
		return 0;
	}

	if (upcast(Game, game, &get_egbase(L))) {
		PortDock* pd = Wh->get_portdock();
			if (!pd) {
				return 0;
			}
		if (pd->expedition_started()){
			game->send_player_start_or_cancel_expedition(*Wh);
			return 1;
		}
	}

	return 0;
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */

/* RST
ProductionSite
--------------

.. class:: ProductionSite

	Child of: :class:`Building`, :class:`HasWares`, :class:`HasWorkers`

	Every building that produces anything.
*/
const char LuaProductionSite::className[] = "ProductionSite";
const MethodType<LuaProductionSite> LuaProductionSite::Methods[] = {
	METHOD(LuaProductionSite, set_wares),
	METHOD(LuaProductionSite, get_wares),
	METHOD(LuaProductionSite, get_workers),
	METHOD(LuaProductionSite, set_workers),
	{nullptr, nullptr},
};
const PropertyType<LuaProductionSite> LuaProductionSite::Properties[] = {
	PROP_RO(LuaProductionSite, valid_workers),
	PROP_RO(LuaProductionSite, valid_wares),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
// documented in parent class
int LuaProductionSite::get_valid_wares(lua_State * L) {
	EditorGameBase& egbase = get_egbase(L);
	ProductionSite * ps = get(L, egbase);

	lua_newtable(L);
	for (const WareAmount& input_ware : ps->descr().inputs()) {
		const WareDescr* descr = egbase.tribes().get_ware_descr(input_ware.first);
		lua_pushstring(L, descr->name());
		lua_pushuint32(L, input_ware.second);
		lua_rawset(L, -3);
	}
	return 1;
}

// documented in parent class
int LuaProductionSite::get_valid_workers(lua_State * L) {
	ProductionSite* ps = get(L, get_egbase(L));
	return workers_map_to_lua(L, get_valid_workers_for(*ps));
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

// documented in parent class
int LuaProductionSite::set_wares(lua_State * L) {
	ProductionSite * ps = get(L, get_egbase(L));
	const TribeDescr& tribe = ps->owner().tribe();
	WaresMap setpoints = m_parse_set_wares_arguments(L, tribe);

	WaresSet valid_wares;
	for (const WareAmount& input_ware : ps->descr().inputs()) {
		valid_wares.insert(input_ware.first);
	}
	for (const std::pair<Widelands::DescriptionIndex, uint32_t>& sp : setpoints) {
		if (!valid_wares.count(sp.first)) {
			report_error(
				L, "<%s> can't be stored in this building: %s!",
						tribe.get_ware_descr(sp.first)->name().c_str(),
						ps->descr().name().c_str());
		}
		WaresQueue & wq = ps->waresqueue(sp.first);
		if (sp.second > wq.get_max_size()) {
			report_error(
				L, "Not enough space for %u items, only for %i", sp.second, wq.get_max_size());
		}
		wq.set_filled(sp.second);
	}

	return 0;
}

// documented in parent class
int LuaProductionSite::get_wares(lua_State * L) {
	ProductionSite * ps = get(L, get_egbase(L));
	const TribeDescr& tribe = ps->owner().tribe();

	bool return_number = false;
	WaresSet wares_set = m_parse_get_wares_arguments(L, tribe, &return_number);

	WaresSet valid_wares;
	for (const WareAmount& input_ware : ps->descr().inputs()) {
		valid_wares.insert(input_ware.first);
	}


	if (wares_set.size() == tribe.get_nrwares()) // Want all returned
		wares_set = valid_wares;

	if (!return_number)
		lua_newtable(L);

	for (const Widelands::DescriptionIndex& ware : wares_set) {
		uint32_t cnt = 0;
		if (valid_wares.count(ware))
			cnt = ps->waresqueue(ware).get_filled();

		if (return_number) { // this is the only thing the customer wants to know
			lua_pushuint32(L, cnt);
			break;
		} else {
			lua_pushstring(L, tribe.get_ware_descr(ware)->name());
			lua_pushuint32(L, cnt);
			lua_rawset(L, -3);
		}
	}
	return 1;
}

// documented in parent class
int LuaProductionSite::get_workers(lua_State* L) {
	ProductionSite* ps = get(L, get_egbase(L));
	return do_get_workers(L, *ps, get_valid_workers_for(*ps));
}

// documented in parent class
int LuaProductionSite::set_workers(lua_State* L) {
	ProductionSite* ps = get(L, get_egbase(L));
	return do_set_workers<LuaProductionSite>(L, ps, get_valid_workers_for(*ps));
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */

int LuaProductionSite::create_new_worker
	(PlayerImmovable & pi, EditorGameBase & egbase, const WorkerDescr * wdes)
{
	ProductionSite & ps = static_cast<ProductionSite &>(pi);
	return ps.warp_worker(egbase, *wdes);
}



/* RST
MilitarySite
--------------

.. class:: MilitarySite

	Child of: :class:`Building`, :class:`HasSoldiers`

	Miltary Buildings
*/
const char LuaMilitarySite::className[] = "MilitarySite";
const MethodType<LuaMilitarySite> LuaMilitarySite::Methods[] = {
	METHOD(LuaMilitarySite, get_soldiers),
	METHOD(LuaMilitarySite, set_soldiers),
	{nullptr, nullptr},
};
const PropertyType<LuaMilitarySite> LuaMilitarySite::Properties[] = {
	PROP_RO(LuaMilitarySite, max_soldiers),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */

// documented in parent class
int LuaMilitarySite::get_max_soldiers(lua_State* L) {
	lua_pushuint32(L, get(L, get_egbase(L))->soldier_capacity());
	return 1;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

// documented in parent class
int LuaMilitarySite::get_soldiers(lua_State* L) {
	MilitarySite* ms = get(L, get_egbase(L));
	return do_get_soldiers(L, *ms, ms->owner().tribe());
}

// documented in parent class
int LuaMilitarySite::set_soldiers(lua_State* L) {
	MilitarySite* ms = get(L, get_egbase(L));
	return do_set_soldiers(L, ms->get_position(), ms, ms->get_owner());
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */

/* RST
TrainingSite
--------------

.. class:: TrainingSite

	Child of: :class:`Building`, :class:`HasSoldiers`

	Miltary Buildings
*/
const char LuaTrainingSite::className[] = "TrainingSite";
const MethodType<LuaTrainingSite> LuaTrainingSite::Methods[] = {
	METHOD(LuaTrainingSite, get_soldiers),
	METHOD(LuaTrainingSite, set_soldiers),
	{nullptr, nullptr},
};
const PropertyType<LuaTrainingSite> LuaTrainingSite::Properties[] = {
	PROP_RO(LuaTrainingSite, max_soldiers),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */

// documented in parent class
int LuaTrainingSite::get_max_soldiers(lua_State* L) {
	lua_pushuint32(L, get(L, get_egbase(L))->soldier_capacity());
	return 1;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

// documented in parent class
int LuaTrainingSite::get_soldiers(lua_State* L) {
	TrainingSite* ts = get(L, get_egbase(L));
	return do_get_soldiers(L, *ts, ts->owner().tribe());
}

// documented in parent class
int LuaTrainingSite::set_soldiers(lua_State* L) {
	TrainingSite* ts = get(L, get_egbase(L));
	return do_set_soldiers(L, ts->get_position(), ts, ts->get_owner());
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */


/* RST
Bob
---

.. class:: Bob

	Child of: :class:`MapObject`

	This is the base class for all Bobs in widelands.
*/
const char LuaBob::className[] = "Bob";
const MethodType<LuaBob> LuaBob::Methods[] = {
	METHOD(LuaBob, has_caps),
	{nullptr, nullptr},
};
const PropertyType<LuaBob> LuaBob::Properties[] = {
	PROP_RO(LuaBob, field),
	{nullptr, nullptr, nullptr},
};

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */

/* RST
	.. attribute:: field //working here

		(RO) The field the bob is located on
*/
// UNTESTED
int LuaBob::get_field(lua_State * L) {

	EditorGameBase & egbase = get_egbase(L);

	Coords coords = get(L, egbase)->get_position();

	return to_lua<LuaMaps::LuaField>(L, new LuaMaps::LuaField(coords.x, coords.y));
}


/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */
/* RST
	.. method:: has_caps(capname)

		Similar to :meth:`Field::has_caps`.

		:arg capname: can be either of

		* :const:`swims`: This bob can swim.
		* :const:`walks`: This bob can walk.
*/
// UNTESTED
int LuaBob::has_caps(lua_State * L) {
	std::string query = luaL_checkstring(L, 2);

	uint32_t movecaps = get(L, get_egbase(L))->descr().movecaps();

	if (query == "swims")
		lua_pushboolean(L, movecaps & MOVECAPS_SWIM);
	else if (query == "walks")
		lua_pushboolean(L,  movecaps & MOVECAPS_WALK);
	else
		report_error(L, "Unknown caps queried: %s!", query.c_str());

	return 1;
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */

/* RST
Ship
----

.. class:: Ship

	This represents a ship in game.
*/

const char LuaShip::className[] = "Ship";
const MethodType<LuaShip> LuaShip::Methods[] = {
	METHOD(LuaShip, get_wares),
	METHOD(LuaShip, get_workers),
	METHOD(LuaShip, build_colonization_port),
	{nullptr, nullptr},
};
const PropertyType<LuaShip> LuaShip::Properties[] = {
	PROP_RO(LuaShip, debug_economy),
	PROP_RO(LuaShip, last_portdock),
	PROP_RO(LuaShip, destination),
	PROP_RO(LuaShip, state),
	PROP_RW(LuaShip, scouting_direction),
	PROP_RW(LuaShip, island_explore_direction),
	PROP_RO(LuaShip, shipname),
	{nullptr, nullptr, nullptr},
};


/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
// UNTESTED, for debug only
int LuaShip::get_debug_economy(lua_State* L) {
	lua_pushlightuserdata(L, get(L, get_egbase(L))->get_economy());
	return 1;
}

/* RST
	.. attribute:: destination

		(RO) Either :const:`nil` if there is no current destination, otherwise
		the :class:`PortDock`.
*/
// UNTESTED
int LuaShip::get_destination(lua_State* L) {
	EditorGameBase & egbase = get_egbase(L);
	return upcasted_map_object_to_lua(L, get(L, egbase)->get_destination(egbase));
}

/* RST
	.. attribute:: last_portdock

		(RO) Either :const:`nil` if no port was ever visited or the last portdock
		was destroyed, otherwise the :class:`PortDock` of the last visited port.
*/
// UNTESTED
int LuaShip::get_last_portdock(lua_State* L) {
	EditorGameBase & egbase = get_egbase(L);
	return upcasted_map_object_to_lua(L, get(L, egbase)->get_lastdock(egbase));
}

/* RST
	.. attribute:: state

	Query which state the ship is in:

	- transport,
	- exp_waiting, exp_scouting, exp_found_port_space, exp_colonizing,
	- sink_request, sink_animation

		(RW) returns the :class:`string` ship's state, or :const:`nil` if there is no valid state.


*/
// UNTESTED sink states
int LuaShip::get_state(lua_State* L) {
	if (is_a(Game, &get_egbase(L))) {
		switch (get(L, get_egbase(L))->get_ship_state()) {
			case Ship::TRANSPORT:
				lua_pushstring(L, "transport");
				break;
			case Ship::EXP_WAITING:
				lua_pushstring(L, "exp_waiting");
				break;
			case Ship::EXP_SCOUTING:
				lua_pushstring(L, "exp_scouting");
				break;
			case Ship::EXP_FOUNDPORTSPACE:
				lua_pushstring(L, "exp_found_port_space");
				break;
			case Ship::EXP_COLONIZING:
				lua_pushstring(L, "exp_colonizing");
				break;
			case Ship::SINK_REQUEST:
				lua_pushstring(L, "sink_request");
				break;
			case Ship::SINK_ANIMATION:
				lua_pushstring(L, "sink_animation");
				break;
			default:
				lua_pushnil(L);
				return 0;
			}
		return 1;
	}
	return 0;
}

int LuaShip::get_scouting_direction(lua_State* L) {
	if (is_a(Game, &get_egbase(L))) {
		switch (get(L, get_egbase(L))->get_scouting_direction()) {
			case WalkingDir::WALK_NE:
				lua_pushstring(L, "ne");
				break;
			case WalkingDir::WALK_E:
				lua_pushstring(L, "e");
				break;
			case WalkingDir::WALK_SE:
				lua_pushstring(L, "se");
				break;
			case WalkingDir::WALK_SW:
				lua_pushstring(L, "sw");
				break;
			case WalkingDir::WALK_W:
				lua_pushstring(L, "w");
				break;
			case WalkingDir::WALK_NW:
				lua_pushstring(L, "nw");
				break;
			case WalkingDir::IDLE:
				return 0;
			}
		return 1;
	}
	return 0;
}

int LuaShip::set_scouting_direction(lua_State* L) {
	if (upcast(Game, game, &get_egbase(L))) {
		std::string dirname = luaL_checkstring(L, 3);
		WalkingDir dir = WalkingDir::IDLE;

		if (dirname == "ne") {
			dir = WalkingDir::WALK_NE;
		} else if (dirname == "e") {
			dir = WalkingDir::WALK_E;
		} else if (dirname == "se") {
			dir = WalkingDir::WALK_SE;
		} else if (dirname == "sw") {
			dir = WalkingDir::WALK_SW;
		} else if (dirname == "w") {
			dir = WalkingDir::WALK_W;
		} else if (dirname == "nw") {
			dir = WalkingDir::WALK_NW;
		} else {
			return 0;
		}
		game->send_player_ship_scouting_direction(*get(L, get_egbase(L)), dir);
		return 1;
	}
	return 0;

}

/* RST
	.. attribute:: island_explore_direction

		(RW) actual direction if the ship sails around an island.
		Sets/returns cw, ccw or nil

*/
int LuaShip::get_island_explore_direction(lua_State* L) {
	if (is_a(Game, &get_egbase(L))) {
		switch (get(L, get_egbase(L))->get_island_explore_direction()) {
			case IslandExploreDirection::kCounterClockwise:
				lua_pushstring(L, "ccw");
				break;
			case IslandExploreDirection::kClockwise:
				lua_pushstring(L, "cw");
				break;
			case IslandExploreDirection::kNotSet:
				return 0;
		}
		return 1;
	}
	return 0;
}

int LuaShip::set_island_explore_direction(lua_State* L) {
	if (upcast(Game, game, &get_egbase(L))) {
		Ship* ship = get(L, get_egbase(L));
		std::string dir = luaL_checkstring(L, 3);
		if (dir == "ccw"){
			 game->send_player_ship_explore_island(*ship,  IslandExploreDirection::kCounterClockwise);
		} else if (dir == "cw") {
			 game->send_player_ship_explore_island(*ship, IslandExploreDirection::kClockwise);
		} else {
			return 0;
		}
		return 1;
	}
	return 0;
}

/* RST
	.. attribute:: NAME

	Get name of ship:

		(RO) returns the :class:`string` ship's name.


*/
int LuaShip::get_shipname(lua_State* L) {
	EditorGameBase& egbase = get_egbase(L);
	Ship* ship = get(L, egbase);
	lua_pushstring(L, ship->get_shipname().c_str());
	return 1;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

/* RST
	.. method:: get_wares()

		Returns the number of wares on this ship. This does not implement
		everything that :class:`HasWares` offers.

		:returns: the number of wares
*/
// UNTESTED
int LuaShip::get_wares(lua_State* L) {
	EditorGameBase& egbase = get_egbase(L);
	int nwares = 0;
	WareInstance* ware;
	Ship* ship = get(L, egbase);
	for (uint32_t i = 0; i < ship->get_nritems(); ++i) {
		const ShippingItem& item = ship->get_item(i);
		item.get(egbase, &ware, nullptr);
		if (ware != nullptr) {
			++nwares;
		}
	}
	lua_pushint32(L, nwares);
	return 1;
}

/* RST
	.. method:: get_workers()

		Returns the number of workers on this ship. This does not implement
		everything that :class:`HasWorkers` offers.

		:returns: the number of workers
*/
// UNTESTED
int LuaShip::get_workers(lua_State* L) {
	EditorGameBase& egbase = get_egbase(L);
	int nworkers = 0;
	Worker* worker;
	Ship* ship = get(L, egbase);
	for (uint32_t i = 0; i < ship->get_nritems(); ++i) {
		const ShippingItem& item = ship->get_item(i);
		item.get(egbase, nullptr, &worker);
		if (worker != nullptr) {
			++nworkers;
		}
	}
	lua_pushint32(L, nworkers);
	return 1;
}

/* RST
	.. method:: build_colonization_port()

		Returns true if port space construction was started (ship was in adequate
		status and a found portspace was nearby)

		:returns: true/false
*/
int LuaShip::build_colonization_port(lua_State* L) {
	Ship* ship =  get(L, get_egbase(L));
	if (ship->get_ship_state() == Widelands::Ship::EXP_FOUNDPORTSPACE) {
		if (upcast(Game, game, &get_egbase(L))) {
			game->send_player_ship_construct_port(*ship, ship->exp_port_spaces()->front());
			return 1;
		}
	}
	return 0;
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */


/* RST
Worker
------

.. class:: Worker

	Child of: :class:`Bob`

	All workers that are visible on the map are of this kind.
*/

const char LuaWorker::className[] = "Worker";
const MethodType<LuaWorker> LuaWorker::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaWorker> LuaWorker::Properties[] = {
	PROP_RO(LuaWorker, owner),
	PROP_RO(LuaWorker, location),
	{nullptr, nullptr, nullptr},
};


/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
/* RST
	.. attribute:: owner

		(RO) The :class:`wl.game.Player` who owns this worker.
*/
// UNTESTED
int LuaWorker::get_owner(lua_State * L) {
	get_factory(L).push_player
		(L, get(L, get_egbase(L))->get_owner()->player_number());
	return 1;
}

/* RST
	.. attribute:: location

		(RO) The location where this worker is situated. This will be either a
		:class:`Building`, :class:`Road`, :class:`Flag` or :const:`nil`. Note
		that a worker that is stored in a warehouse has a location :const:`nil`.
		A worker that is out working (e.g. hunter) has as a location his
		building. A stationed soldier has his military building as location.
		Workers on transit usually have the Road they are currently on as
		location.
*/
// UNTESTED
int LuaWorker::get_location(lua_State * L) {
	EditorGameBase & egbase = get_egbase(L);
	return
		upcasted_map_object_to_lua
			(L, static_cast<BaseImmovable *>
			 	(get(L, egbase)->get_location(egbase)));
}




/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

/*
 ==========================================================
 C METHODS
 ==========================================================
 */


/* RST
Soldier
-------

.. class:: Soldier

	Child of: :class:`Worker`

	All soldiers that are on the map are represented by this class.
*/

const char LuaSoldier::className[] = "Soldier";
const MethodType<LuaSoldier> LuaSoldier::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaSoldier> LuaSoldier::Properties[] = {
	PROP_RO(LuaSoldier, attack_level),
	PROP_RO(LuaSoldier, defense_level),
	PROP_RO(LuaSoldier, hp_level),
	PROP_RO(LuaSoldier, evade_level),
	{nullptr, nullptr, nullptr},
};


/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
/* RST
	.. attribute:: attack_level

		(RO) The current attack level of this soldier
*/
// UNTESTED
int LuaSoldier::get_attack_level(lua_State * L) {
	lua_pushuint32(L, get(L, get_egbase(L))->get_attack_level());
	return 1;
}

/* RST
	.. attribute:: defense_level

		(RO) The current defense level of this soldier
*/
// UNTESTED
int LuaSoldier::get_defense_level(lua_State * L) {
	lua_pushuint32(L, get(L, get_egbase(L))->get_defense_level());
	return 1;
}

/* RST
	.. attribute:: hp_level

		(RO) The current hp level of this soldier
*/
// UNTESTED
int LuaSoldier::get_hp_level(lua_State * L) {
	lua_pushuint32(L, get(L, get_egbase(L))->get_hp_level());
	return 1;
}

/* RST
	.. attribute:: evade_level

		(RO) The current evade level of this soldier
*/
// UNTESTED
int LuaSoldier::get_evade_level(lua_State * L) {
	lua_pushuint32(L, get(L, get_egbase(L))->get_evade_level());
	return 1;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

/*
 ==========================================================
 C METHODS
 ==========================================================
 */


/* RST
Field
-----

.. class:: Field

	This class represents one Field in Widelands. The field may contain
	immovables like Flags or Buildings and can be connected via Roads. Every
	Field has two Triangles associated with itself: the right and the down one.

	You cannot instantiate this class directly, instead use
	:meth:`wl.map.Map.get_field`.
*/

const char LuaField::className[] = "Field";
const MethodType<LuaField> LuaField::Methods[] = {
	METHOD(LuaField, __eq),
	METHOD(LuaField, __tostring),
	METHOD(LuaField, region),
	METHOD(LuaField, has_caps),
	{nullptr, nullptr},
};
const PropertyType<LuaField> LuaField::Properties[] = {
	PROP_RO(LuaField, __hash),
	PROP_RO(LuaField, x),
	PROP_RO(LuaField, y),
	PROP_RO(LuaField, rn),
	PROP_RO(LuaField, ln),
	PROP_RO(LuaField, trn),
	PROP_RO(LuaField, tln),
	PROP_RO(LuaField, bln),
	PROP_RO(LuaField, brn),
	PROP_RO(LuaField, immovable),
	PROP_RO(LuaField, bobs),
	PROP_RW(LuaField, terr),
	PROP_RW(LuaField, terd),
	PROP_RW(LuaField, height),
	PROP_RW(LuaField, raw_height),
	PROP_RO(LuaField, viewpoint_x),
	PROP_RO(LuaField, viewpoint_y),
	PROP_RW(LuaField, resource),
	PROP_RW(LuaField, resource_amount),
	PROP_RO(LuaField, initial_resource_amount),
	PROP_RO(LuaField, claimers),
	PROP_RO(LuaField, owner),
	{nullptr, nullptr, nullptr},
};


void LuaField::__persist(lua_State * L) {
	PERS_INT32("x", m_c.x); PERS_INT32("y", m_c.y);
}

void LuaField::__unpersist(lua_State * L) {
	UNPERS_INT32("x", m_c.x); UNPERS_INT32("y", m_c.y);
}

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
// Hash is used to identify a class in a Set
int LuaField::get___hash(lua_State * L) {
	const std::string pushme = (boost::format("%i_%i") % m_c.x % m_c.y).str();
	lua_pushstring(L, pushme.c_str());
	return 1;
}

/* RST
	.. attribute:: x, y

		(RO) The x/y coordinate of this field
*/
int LuaField::get_x(lua_State * L) {lua_pushuint32(L, m_c.x); return 1;}
int LuaField::get_y(lua_State * L) {lua_pushuint32(L, m_c.y); return 1;}

/* RST
	.. attribute:: height

		(RW) The height of this field. The default height is 10, you can increase
		or decrease this value to build mountains. Note though that if you change
		this value too much, all surrounding fields will also change their
		heights because the slope is constrained.
*/
int LuaField::get_height(lua_State * L) {
	lua_pushuint32(L, fcoords(L).field->get_height());
	return 1;
}
int LuaField::set_height(lua_State * L) {
	uint32_t height = luaL_checkuint32(L, -1);
	FCoords f = fcoords(L);

	if (f.field->get_height() == height)
		return 0;

	if (height > MAX_FIELD_HEIGHT)
		report_error(L, "height must be <= %i", MAX_FIELD_HEIGHT);

	EditorGameBase & egbase = get_egbase(L);
	egbase.map().set_height(egbase.world(), f, height);

	return 0;
}

/* RST
	.. attribute:: raw_height

		(RW The same as :attr:`height`, but setting this will not trigger a
		recalculation of the surrounding fields. You can use this field to
		change the height of many fields on a map quickly, then use
		:func:`wl.map.recalculate()` to make sure that everything is in order.
*/
// UNTESTED
int LuaField::get_raw_height(lua_State * L) {
	lua_pushuint32(L, fcoords(L).field->get_height());
	return 1;
}
int LuaField::set_raw_height(lua_State * L) {
	uint32_t height = luaL_checkuint32(L, -1);
	FCoords f = fcoords(L);

	if (f.field->get_height() == height)
		return 0;

	if (height > MAX_FIELD_HEIGHT)
		report_error(L, "height must be <= %i", MAX_FIELD_HEIGHT);

	f.field->set_height(height);

	return 0;
}


/* RST
	.. attribute:: viewpoint_x, viewpoint_y

		(RO) Returns the position in pixels to move the view to to center
		this field for the current interactive player
*/
int LuaField::get_viewpoint_x(lua_State * L) {
	int32_t px, py;
	MapviewPixelFunctions::get_save_pix(get_egbase(L).map(), m_c, px, py);
	lua_pushint32(L, px);
	return 1;
}
int LuaField::get_viewpoint_y(lua_State * L) {
	int32_t px, py;
	MapviewPixelFunctions::get_save_pix(get_egbase(L).map(), m_c, px, py);
	lua_pushint32(L, py);
	return 1;
}

/* RST
	.. attribute:: resource

		(RW) The name of the resource that is available in this field or
		:const:`nil`

		:see also: :attr:`resource_amount`
*/
int LuaField::get_resource(lua_State * L) {

	const ResourceDescription* rDesc = get_egbase(L).world().get_resource
			(fcoords(L).field->get_resources());

	lua_pushstring
		(L, rDesc ? rDesc->name().c_str() : "none");

	return 1;
}
int LuaField::set_resource(lua_State * L) {
	auto& egbase = get_egbase(L);
	int32_t res = get_egbase(L).world().get_resource
		(luaL_checkstring(L, -1));

	if (res == Widelands::INVALID_INDEX)
		report_error(L, "Illegal resource: '%s'", luaL_checkstring(L, -1));

	auto c = fcoords(L);
	const auto current_amount = c.field->get_resources_amount();
	auto& map = egbase.map();
	map.initialize_resources(c, res, c.field->get_initial_res_amount());
	map.set_resources(c, current_amount);
	return 0;
}

/* RST
	.. attribute:: resource_amount

		(RW) How many items of the resource is available in this field.

		:see also: :attr:`resource`
*/
int LuaField::get_resource_amount(lua_State * L) {
	lua_pushuint32(L, fcoords(L).field->get_resources_amount());
	return 1;
}
int LuaField::set_resource_amount(lua_State * L) {
	auto c  = fcoords(L);
	int32_t res = c.field->get_resources();
	int32_t amount = luaL_checkint32(L, -1);
	const ResourceDescription * resDesc = get_egbase(L).world().get_resource(res);
	int32_t max_amount = resDesc ? resDesc->max_amount() : 0;

	if (amount < 0 || amount > max_amount)
		report_error(L, "Illegal amount: %i, must be >= 0 and <= %i", amount, max_amount);

	EditorGameBase & egbase = get_egbase(L);
	auto& map = egbase.map();
	if (is_a(Game, &egbase)) {
		map.set_resources(c, amount);
	} else {
		// in editor, reset also initial amount
		map.initialize_resources(c, res, amount);
	}
	return 0;
}
/* RST
	.. attribute:: initial_resource_amount

		(RO) Starting value of resource. It is set be resource_amount

		:see also: :attr:`resource`
*/
int LuaField::get_initial_resource_amount(lua_State * L) {
	lua_pushuint32(L, fcoords(L).field->get_initial_res_amount());
	return 1;
}
/* RST
	.. attribute:: immovable

		(RO) The immovable that stands on this field or :const:`nil`. If you want
		to remove an immovable, you can use :func:`wl.map.MapObject.remove`.
*/
int LuaField::get_immovable(lua_State * L) {
	BaseImmovable * bi = get_egbase(L).map().get_immovable(m_c);

	if (!bi)
		return 0;
	else
		upcasted_map_object_to_lua(L, bi);
	return 1;
}

/* RST
	.. attribute:: bobs

		(RO) An :class:`array` of :class:`~wl.map.Bob` that are associated
		with this field
*/
// UNTESTED
int LuaField::get_bobs(lua_State * L) {
	Bob * b = fcoords(L).field->get_first_bob();

	lua_newtable(L);
	uint32_t cidx = 1;
	while (b) {
		lua_pushuint32(L, cidx++);
		upcasted_map_object_to_lua(L, b);
		lua_rawset(L, -3);
		b = b->get_next_bob();
	}
	return 1;
}

/* RST
	.. attribute:: terr, terd

		(RW) The terrain of the right/down triangle. This is a string value
		containing the name of the terrain as it is defined in the world
		configuration. You can change the terrain by simply assigning another
		valid name to these variables.
*/
int LuaField::get_terr(lua_State * L) {
	TerrainDescription & td =
		get_egbase(L).world().terrain_descr
			(fcoords(L).field->terrain_r());
	lua_pushstring(L, td.name().c_str());
	return 1;
}
int LuaField::set_terr(lua_State* L) {
	const char* name = luaL_checkstring(L, -1);
	EditorGameBase& egbase = get_egbase(L);
	const World& world = egbase.world();
	const DescriptionIndex td = world.terrains().get_index(name);
	if (td == static_cast<DescriptionIndex>(-1))
		report_error(L, "Unknown terrain '%s'", name);

	egbase.map().change_terrain(world, TCoords<FCoords>(fcoords(L), TCoords<FCoords>::R), td);

	lua_pushstring(L, name);
	return 1;
}

int LuaField::get_terd(lua_State * L) {
	TerrainDescription & td =
		get_egbase(L).world().terrain_descr
			(fcoords(L).field->terrain_d());
	lua_pushstring(L, td.name().c_str());
	return 1;
}
int LuaField::set_terd(lua_State * L) {
	const char * name = luaL_checkstring(L, -1);
	EditorGameBase& egbase = get_egbase(L);
	const World& world = egbase.world();
	const DescriptionIndex td =
		world.terrains().get_index(name);
	if (td == static_cast<DescriptionIndex>(INVALID_INDEX))
		report_error(L, "Unknown terrain '%s'", name);

	egbase.map().change_terrain
		(world, TCoords<FCoords> (fcoords(L), TCoords<FCoords>::D), td);

	lua_pushstring(L, name);
	return 1;
}

/* RST
	.. attribute:: rn, ln, brn, bln, trn, tln

		(RO) The neighbour fields of this field. The abbreviations stand for:

		* rn -- Right neighbour
		* ln -- Left neighbour
		* brn -- Bottom right neighbour
		* bln -- Bottom left neighbour
		* trn -- Top right neighbour
		* tln -- Top left neighbour

		Note that the widelands map wraps at its borders, that is the following
		holds:

		.. code-block:: lua

			wl.map.Field(wl.map.get_width()-1, 10).rn == wl.map.Field(0, 10)
*/
#define GET_X_NEIGHBOUR(X) int LuaField::get_ ##X(lua_State* L) { \
   Coords n; \
   get_egbase(L).map().get_ ##X(m_c, &n); \
	to_lua<LuaField>(L, new LuaField(n.x, n.y)); \
	return 1; \
}
GET_X_NEIGHBOUR(rn)
GET_X_NEIGHBOUR(ln)
GET_X_NEIGHBOUR(trn)
GET_X_NEIGHBOUR(tln)
GET_X_NEIGHBOUR(bln)
GET_X_NEIGHBOUR(brn)

/* RST
	.. attribute:: owner

		(RO) The current owner of the field or :const:`nil` if noone owns it. See
		also :attr:`claimers`.
*/
int LuaField::get_owner(lua_State * L) {
	PlayerNumber current_owner = fcoords(L).field->get_owned_by();
	if (current_owner) {
		get_factory(L).push_player(L, current_owner);
		return 1;
	}
	return 0;
}

/* RST
	.. attribute:: claimers

		(RO) An :class:`array` of players that have military influence over this
		field sorted by the amount of influence they have. Note that this does
		not necessarily mean that claimers[1] is also the owner of the field, as
		a field that houses a surrounded military building is owned by the
		surrounded player, but others have more military influence over it.

		Note: The one currently owning the field is in :attr:`owner`.
*/
int LuaField::get_claimers(lua_State * L) {
	EditorGameBase & egbase = get_egbase(L);
	Map & map = egbase.map();

	std::vector<PlrInfluence> claimers;

	iterate_players_existing(other_p, map.get_nrplayers(), egbase, plr)
		claimers.push_back
			(PlrInfluence(plr->player_number(), plr->military_influence
					(map.get_index(m_c, map.get_width()))
			)
		);

	std::stable_sort (claimers.begin(), claimers.end(), sort_claimers);

	lua_createtable(L, 1, 0); // We mostly expect one claimer per field.

	// Push the players with military influence
	uint32_t cidx = 1;
	for (const PlrInfluence& claimer : claimers) {
		if (claimer.second <= 0)
			continue;
		lua_pushuint32(L, cidx ++);
		get_factory(L).push_player(L, claimer.first);
		lua_rawset(L, -3);
	}

	return 1;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */
int LuaField::__eq(lua_State * L) {
	lua_pushboolean(L, (*get_user_class<LuaField>(L, -1))->m_c == m_c);
	return 1;
}

int LuaField::__tostring(lua_State * L) {
	const std::string pushme = (boost::format("Field(%i,%i)") % m_c.x % m_c.y).str();
	lua_pushstring(L, pushme);
	return 1;
}

/* RST
	.. function:: region(r1[, r2])

		Returns an array of all Fields inside the given region. If one argument
		is given it defines the radius of the region. If both arguments are
		specified, the first one defines the outer radius and the second one the
		inner radius and a hollow region is returned, that is all fields in the
		outer radius region minus all fields in the inner radius region.

		A small example:

		.. code-block:: lua

			f:region(1)

		will return an array with the following entries (Note: Ordering of the
		fields inside the array is not guaranteed):

		.. code-block:: lua

			{f, f.rn, f.ln, f.brn, f.bln, f.tln, f.trn}

		:returns: The array of the given fields.
		:rtype: :class:`array`
*/
int LuaField::region(lua_State * L) {
	uint32_t n = lua_gettop(L);

	if (n == 3) {
		uint32_t radius = luaL_checkuint32(L, -2);
		uint32_t inner_radius = luaL_checkuint32(L, -1);
		return m_hollow_region(L, radius, inner_radius);
	}

	uint32_t radius = luaL_checkuint32(L, -1);
	return m_region(L, radius);
}


/* RST
	.. method:: has_caps(capname)

		Returns :const:`true` if the field has this caps associated
		with it, otherwise returns false.

		:arg capname: can be either of

		* :const:`small`: Can a small building be build here?
		* :const:`medium`: Can a medium building be build here?
		* :const:`big`: Can a big building be build here?
		* :const:`mine`: Can a mine be build here?
		* :const:`port`: Can a port be build here?
		* :const:`flag`: Can a flag be build here?
		* :const:`walkable`: Is this field passable for walking bobs?
		* :const:`swimable`: Is this field passable for swimming bobs?
*/
int LuaField::has_caps(lua_State * L) {
	FCoords f = fcoords(L);
	std::string query = luaL_checkstring(L, 2);

	if (query == "walkable")
		lua_pushboolean(L, f.field->nodecaps() & MOVECAPS_WALK);
	else if (query == "swimmable")
		lua_pushboolean(L, f.field->nodecaps() & MOVECAPS_SWIM);
	else if (query == "small")
		lua_pushboolean(L, f.field->nodecaps() & BUILDCAPS_SMALL);
	else if (query == "medium")
		lua_pushboolean(L, f.field->nodecaps() & BUILDCAPS_MEDIUM);
	else if (query == "big")
		lua_pushboolean(L, (f.field->nodecaps() & BUILDCAPS_BIG) == BUILDCAPS_BIG);
	else if (query == "port") {
		lua_pushboolean
			(L, (f.field->nodecaps() & BUILDCAPS_PORT) && get_egbase(L).map().is_port_space(f));
	} else if (query == "mine")
		lua_pushboolean(L, f.field->nodecaps() & BUILDCAPS_MINE);
	else if (query == "flag")
		lua_pushboolean(L, f.field->nodecaps() & BUILDCAPS_FLAG);
	else
		report_error(L, "Unknown caps queried: %s!", query.c_str());

	return 1;
}

/*
 ==========================================================
 C METHODS
 ==========================================================
 */
int LuaField::m_region(lua_State * L, uint32_t radius)
{
	Map & map = get_egbase(L).map();
	MapRegion<Area<FCoords> > mr
		(map, Area<FCoords>(fcoords(L), radius));

	lua_newtable(L);
	uint32_t idx = 1;
	do {
		lua_pushuint32(L, idx++);
		const FCoords & loc = mr.location();
		to_lua<LuaField>(L, new LuaField(loc.x, loc.y));
		lua_settable(L, -3);
	} while (mr.advance(map));

	return 1;
}

int LuaField::m_hollow_region
	(lua_State * L, uint32_t radius, uint32_t inner_radius)
{
	Map & map = get_egbase(L).map();
	HollowArea<Area<> > har(Area<>(m_c, radius), inner_radius);

	MapHollowRegion<Area<> > mr(map, har);

	lua_newtable(L);
	uint32_t idx = 1;
	do {
		lua_pushuint32(L, idx++);
		to_lua<LuaField>(L, new LuaField(mr.location()));
		lua_settable(L, -3);
	} while (mr.advance(map));

	return 1;
}

const Widelands::FCoords LuaField::fcoords(lua_State * L) {
	return get_egbase(L).map().get_fcoords(m_c);
}


/* RST
PlayerSlot
----------

.. class:: PlayerSlot

	A player description as it is in the map. This contains information
	about the start position, the name of the player if this map is played
	as scenario and it's tribe. Note that these information can be different
	than the players actually valid in the game as in single player games,
	the player can choose most parameters freely.
*/
const char LuaPlayerSlot::className[] = "PlayerSlot";
const MethodType<LuaPlayerSlot> LuaPlayerSlot::Methods[] = {
	{nullptr, nullptr},
};
const PropertyType<LuaPlayerSlot> LuaPlayerSlot::Properties[] = {
	PROP_RO(LuaPlayerSlot, tribe_name),
	PROP_RO(LuaPlayerSlot, name),
	PROP_RO(LuaPlayerSlot, starting_field),
	{nullptr, nullptr, nullptr},
};

void LuaPlayerSlot::__persist(lua_State * L) {
	PERS_UINT32("player", m_plr);
}

void LuaPlayerSlot::__unpersist(lua_State * L) {
	UNPERS_UINT32("player", m_plr);
}

/*
 ==========================================================
 PROPERTIES
 ==========================================================
 */
/* RST
	.. attribute:: tribe_name

		(RO) The name of the tribe suggested for this player in this map
*/
int LuaPlayerSlot::get_tribe_name(lua_State * L) {
	lua_pushstring(L, get_egbase(L).get_map()->get_scenario_player_tribe(m_plr));
	return 1;
}

/* RST
	.. attribute:: name

		(RO) The name for this player as suggested in this map
*/
int LuaPlayerSlot::get_name(lua_State * L) {
	lua_pushstring(L, get_egbase(L).get_map()->get_scenario_player_name(m_plr));
	return 1;
}

/* RST
	.. attribute:: starting_field

		(RO) The starting_field for this player as set in the map.
		Note that it is not guaranteed that the HQ of the player is on this
		field as scenarios and starting conditions are free to place the HQ
		wherever it want. This field is only centered when the game starts.
*/
int LuaPlayerSlot::get_starting_field(lua_State * L) {
	to_lua<LuaField>(L, new LuaField(get_egbase(L).map().get_starting_pos(m_plr)));
	return 1;
}

/*
 ==========================================================
 LUA METHODS
 ==========================================================
 */

/*
 ==========================================================
 C METHODS
 ==========================================================
 */


/*
 * ========================================================================
 *                            MODULE FUNCTIONS
 * ========================================================================
 */

const static struct luaL_Reg wlmap [] = {
	{nullptr, nullptr}
};

void luaopen_wlmap(lua_State * L) {
	lua_getglobal(L, "wl");  // S: wl_table
	lua_pushstring(L, "map"); // S: wl_table "map"
	luaL_newlib(L, wlmap);  // S: wl_table "map" wl.map_table
	lua_settable(L, -3); // S: wl_table
	lua_pop(L, 1); // S:

	register_class<LuaMap>(L, "map");
	register_class<LuaTribeDescription>(L, "map");
	register_class<LuaMapObjectDescription>(L, "map");

	register_class<LuaBuildingDescription>(L, "map", true);
	add_parent<LuaBuildingDescription, LuaMapObjectDescription>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaConstructionSiteDescription>(L, "map", true);
	add_parent<LuaConstructionSiteDescription, LuaBuildingDescription>(L);
	add_parent<LuaConstructionSiteDescription, LuaMapObjectDescription>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaDismantleSiteDescription>(L, "map", true);
	add_parent<LuaDismantleSiteDescription, LuaBuildingDescription>(L);
	add_parent<LuaDismantleSiteDescription, LuaMapObjectDescription>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaProductionSiteDescription>(L, "map", true);
	add_parent<LuaProductionSiteDescription, LuaBuildingDescription>(L);
	add_parent<LuaProductionSiteDescription, LuaMapObjectDescription>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaMilitarySiteDescription>(L, "map", true);
	add_parent<LuaMilitarySiteDescription, LuaBuildingDescription>(L);
	add_parent<LuaMilitarySiteDescription, LuaMapObjectDescription>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaTrainingSiteDescription>(L, "map", true);
	add_parent<LuaTrainingSiteDescription, LuaProductionSiteDescription>(L);
	add_parent<LuaTrainingSiteDescription, LuaBuildingDescription>(L);
	add_parent<LuaTrainingSiteDescription, LuaMapObjectDescription>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaWarehouseDescription>(L, "map", true);
	add_parent<LuaWarehouseDescription, LuaBuildingDescription>(L);
	add_parent<LuaWarehouseDescription, LuaMapObjectDescription>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaWareDescription>(L, "map", true);
	add_parent<LuaWareDescription, LuaMapObjectDescription>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaWorkerDescription>(L, "map", true);
	add_parent<LuaWorkerDescription, LuaMapObjectDescription>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaField>(L, "map");
	register_class<LuaPlayerSlot>(L, "map");
	register_class<LuaMapObject>(L, "map");

	register_class<LuaBob>(L, "map", true);
	add_parent<LuaBob, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaWorker>(L, "map", true);
	add_parent<LuaWorker, LuaBob>(L);
	add_parent<LuaWorker, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaSoldier>(L, "map", true);
	add_parent<LuaSoldier, LuaWorker>(L);
	add_parent<LuaSoldier, LuaBob>(L);
	add_parent<LuaSoldier, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaShip>(L, "map", true);
	add_parent<LuaShip, LuaBob>(L);
	add_parent<LuaShip, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaBaseImmovable>(L, "map", true);
	add_parent<LuaBaseImmovable, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaPlayerImmovable>(L, "map", true);
	add_parent<LuaPlayerImmovable, LuaBaseImmovable>(L);
	add_parent<LuaPlayerImmovable, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaBuilding>(L, "map", true);
	add_parent<LuaBuilding, LuaPlayerImmovable>(L);
	add_parent<LuaBuilding, LuaBaseImmovable>(L);
	add_parent<LuaBuilding, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaPortDock>(L, "map", true);
	add_parent<LuaPortDock, LuaPlayerImmovable>(L);
	add_parent<LuaPortDock, LuaBaseImmovable>(L);
	add_parent<LuaPortDock, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaFlag>(L, "map", true);
	add_parent<LuaFlag, LuaPlayerImmovable>(L);
	add_parent<LuaFlag, LuaBaseImmovable>(L);
	add_parent<LuaFlag, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaRoad>(L, "map", true);
	add_parent<LuaRoad, LuaPlayerImmovable>(L);
	add_parent<LuaRoad, LuaBaseImmovable>(L);
	add_parent<LuaRoad, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaConstructionSite>(L, "map", true);
	add_parent<LuaConstructionSite, LuaBuilding>(L);
	add_parent<LuaConstructionSite, LuaPlayerImmovable>(L);
	add_parent<LuaConstructionSite, LuaBaseImmovable>(L);
	add_parent<LuaConstructionSite, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaWarehouse>(L, "map", true);
	add_parent<LuaWarehouse, LuaBuilding>(L);
	add_parent<LuaWarehouse, LuaPlayerImmovable>(L);
	add_parent<LuaWarehouse, LuaBaseImmovable>(L);
	add_parent<LuaWarehouse, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaProductionSite>(L, "map", true);
	add_parent<LuaProductionSite, LuaBuilding>(L);
	add_parent<LuaProductionSite, LuaPlayerImmovable>(L);
	add_parent<LuaProductionSite, LuaBaseImmovable>(L);
	add_parent<LuaProductionSite, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaMilitarySite>(L, "map", true);
	add_parent<LuaMilitarySite, LuaBuilding>(L);
	add_parent<LuaMilitarySite, LuaPlayerImmovable>(L);
	add_parent<LuaMilitarySite, LuaBaseImmovable>(L);
	add_parent<LuaMilitarySite, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table

	register_class<LuaTrainingSite>(L, "map", true);
	add_parent<LuaTrainingSite, LuaProductionSite>(L);
	add_parent<LuaTrainingSite, LuaBuilding>(L);
	add_parent<LuaTrainingSite, LuaPlayerImmovable>(L);
	add_parent<LuaTrainingSite, LuaBaseImmovable>(L);
	add_parent<LuaTrainingSite, LuaMapObject>(L);
	lua_pop(L, 1); // Pop the meta table
}

}
