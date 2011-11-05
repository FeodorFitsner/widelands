/*
 * Copyright (C) 2010 by the Widelands Development Team
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

#include "ship.h"

#include "economy/fleet.h"
#include "game.h"
#include "game_data_error.h"
#include "map.h"
#include "tribe.h"

namespace Widelands {

Ship_Descr::Ship_Descr
	(const char * given_name, const char * descname,
	 const std::string & directory, Profile & prof, Section & global_s,
	 const Widelands::Tribe_Descr & tribe)
: Descr(given_name, descname, directory, prof, global_s, &tribe)
{
	m_sail_anims.parse
		(*this,
		 directory,
		 prof,
		 (name() + "_sail_??").c_str(),
		 prof.get_section("sail"));
}

uint32_t Ship_Descr::movecaps() const throw ()
{
	return MOVECAPS_SWIM;
}

Bob & Ship_Descr::create_object() const
{
	return *new Ship(*this);
}


Ship::Ship(const Ship_Descr & descr) :
	Bob(descr),
	m_fleet(0)
{
}

Bob::Type Ship::get_bob_type() const throw ()
{
	return SHIP;
}

void Ship::init_auto_task(Game & game)
{
	start_task_shipidle(game);
}

void Ship::init(Editor_Game_Base & egbase)
{
	Bob::init(egbase);

	init_fleet(egbase);
}

/**
 * Create the initial singleton @ref Fleet to which we belong.
 * The fleet code will take automatically merge us into a larger
 * fleet, if one is reachable.
 */
void Ship::init_fleet(Editor_Game_Base & egbase)
{
	assert(get_owner() != 0);
	Fleet * fleet = new Fleet(*get_owner());
	fleet->add_ship(this);
	fleet->init(egbase);
	// fleet calls the set_fleet function appropriately
}

void Ship::cleanup(Editor_Game_Base & egbase)
{
	if (m_fleet)
		m_fleet->remove_ship(egbase, this);

	Bob::cleanup(egbase);
}

/**
 * This function is to be called only by @ref Fleet.
 */
void Ship::set_fleet(Fleet * fleet)
{
	m_fleet = fleet;
}


struct FindBobShip : FindBob {
	virtual bool accept(Bob * bob) const
	{
		return bob->get_bob_type() == Bob::SHIP;
	}
};

void Ship::wakeup_neighbours(Game & game)
{
	FCoords position = get_position();
	Area<FCoords> area(position, 1);
	std::vector<Bob *> ships;
	game.map().find_bobs(area, &ships, FindBobShip());

	for
		(std::vector<Bob *>::const_iterator it = ships.begin();
		 it != ships.end(); ++it)
	{
		if (*it == this)
			continue;

		static_cast<Ship *>(*it)->shipidle_wakeup(game);
	}
}


/**
 * Standard behaviour of ships while idle.
 *
 * ivar1 = helper flag for coordination
 */
const Bob::Task Ship::taskShipIdle = {
	"shipidle",
	static_cast<Bob::Ptr>(&Ship::shipidle_update),
	0,
	0,
	true // unique task
};

void Ship::start_task_shipidle(Game & game)
{
	push_task(game, taskShipIdle);
	top_state().ivar1 = 0;
}

void Ship::shipidle_wakeup(Game & game)
{
	if (get_state(taskShipIdle))
		send_signal(game, "wakeup");
}

void Ship::shipidle_update(Game & game, Bob::State & state)
{
	// Handle signals
	std::string signal = get_signal();
	if (!signal.empty()) {
		if (signal == "wakeup") {
			signal_handled();
		} else {
			send_signal(game, "fail");
			pop_task(game);
			return;
		}
	}

	if (state.ivar1) {
		// We've just completed one step, so give neighbours
		// a chance to move away first
		wakeup_neighbours(game);
		state.ivar1 = 0;
		schedule_act(game, 25);
		return;
	}

	// Check if we should move away from ships and shores
	FCoords position = get_position();
	Map & map = game.map();
	unsigned int dirs[LAST_DIRECTION + 1];
	unsigned int dirmax = 0;

	for (Direction dir = 0; dir <= LAST_DIRECTION; ++dir) {
		FCoords node = dir ? map.get_neighbour(position, dir) : position;
		dirs[dir] = node.field->nodecaps() & MOVECAPS_WALK ? 10 : 0;

		Area<FCoords> area(node, 0);
		std::vector<Bob *> ships;
		game.map().find_bobs(area, &ships, FindBobShip());

		for (std::vector<Bob *>::const_iterator it = ships.begin(); it != ships.end(); ++it) {
			if (*it == this)
				continue;

			dirs[dir] += 3;
		}

		dirmax = std::max(dirmax, dirs[dir]);
	}

	if (dirmax) {
		unsigned int prob[LAST_DIRECTION + 1];
		unsigned int totalprob = 0;

		// The probability for moving into a given direction is also
		// affected by the "close" directions.
		for (Direction dir = 0; dir <= LAST_DIRECTION; ++dir) {
			prob[dir] = 10 * dirmax - 10 * dirs[dir];

			if (dir > 0) {
				unsigned int delta = std::min(prob[dir], dirs[(dir % 6) + 1] + dirs[1 + ((dir - 1) % 6)]);
				prob[dir] -= delta;
			}

			totalprob += prob[dir];
		}

		unsigned int rnd = game.logic_rand() % totalprob;
		Direction dir = 0;
		while (rnd >= prob[dir]) {
			rnd -= prob[dir];
			++dir;
		}

		if (dir == 0 || dir > LAST_DIRECTION) {
			start_task_idle(game, descr().main_animation(), 1500);
			return;
		}

		FCoords neighbour = map.get_neighbour(position, dir);
		if (!(neighbour.field->nodecaps() & MOVECAPS_SWIM)) {
			start_task_idle(game, descr().main_animation(), 1500);
			return;
		}

		state.ivar1 = 1;
		start_task_move(game, dir, &descr().get_sail_anims(), false);
		return;
	}

	// No desire to move around, so sleep
	start_task_idle(game, descr().main_animation(), -1);
}

void Ship::log_general_info(const Editor_Game_Base & egbase)
{
	Bob::log_general_info(egbase);

	molog("Fleet: %u\n", m_fleet? m_fleet->serial() : 0);
}


/*
==============================

Load / Save implementation

==============================
*/

#define SHIP_SAVEGAME_VERSION 1

Ship::Loader::Loader()
{
}

const Bob::Task * Ship::Loader::get_task(const std::string & name)
{
	if (name == "shipidle") return &taskShipIdle;
	return Bob::Loader::get_task(name);
}

void Ship::Loader::load_finish()
{
	Bob::Loader::load_finish();

	Ship & ship = get<Ship>();
	// For robustness, in case our fleet did not get restored from the savegame
	// for whatever reason
	if (!ship.m_fleet)
		ship.init_fleet(egbase());
}


Map_Object::Loader * Ship::load
	(Editor_Game_Base & egbase, Map_Map_Object_Loader & mol, FileRead & fr)
{
	std::auto_ptr<Loader> loader(new Loader);

	try {
		// The header has been peeled away by the caller

		uint8_t const version = fr.Unsigned8();
		if (1 <= version && version <= SHIP_SAVEGAME_VERSION) {
			std::string owner = fr.CString();
			std::string name = fr.CString();
			const Ship_Descr * descr = 0;

			egbase.manually_load_tribe(owner);

			if (const Tribe_Descr * tribe = egbase.get_tribe(owner))
				descr = dynamic_cast<const Ship_Descr *>
					(tribe->get_bob_descr(name));

			if (!descr)
				throw game_data_error
					("undefined ship %s/%s", owner.c_str(), name.c_str());

			loader->init(egbase, mol, descr->create_object());
			loader->load(fr);
		} else
			throw game_data_error(_("unknown/unhandled version %u"), version);
	} catch (const std::exception & e) {
		throw wexception(_("loading ship: %s"), e.what());
	}

	return loader.release();
}

void Ship::save
	(Editor_Game_Base & egbase, Map_Map_Object_Saver & mos, FileWrite & fw)
{
	fw.Unsigned8(header_Ship);
	fw.Unsigned8(SHIP_SAVEGAME_VERSION);

	fw.CString(descr().get_owner_tribe()->name());
	fw.CString(descr().name());

	Bob::save(egbase, mos, fw);
}

} // namespace Widelands
