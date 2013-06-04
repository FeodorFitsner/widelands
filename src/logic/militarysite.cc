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

#include "militarysite.h"

#include "battle.h"
#include "economy/flag.h"
#include "economy/request.h"
#include "editor_game_base.h"
#include "findbob.h"
#include "game.h"
#include "i18n.h"
#include "message_queue.h"
#include "player.h"
#include "profile/profile.h"
#include "soldier.h"
#include "tribe.h"
#include "worker.h"

#include "log.h"

#include "upcast.h"

#include <libintl.h>

#include <clocale>
#include <cstdio>

namespace Widelands {

MilitarySite_Descr::MilitarySite_Descr
	(char        const * const _name,
	 char        const * const _descname,
	 const std::string & directory, Profile & prof,  Section & global_s,
	 const Tribe_Descr & _tribe)
:
	ProductionSite_Descr
		(_name, _descname, directory, prof, global_s, _tribe),
m_conquer_radius     (0),
m_num_soldiers       (0),
m_heal_per_second    (0)
{
	m_conquer_radius      = global_s.get_safe_int("conquers");
	m_num_soldiers        = global_s.get_safe_int("max_soldiers");
	m_heal_per_second     = global_s.get_safe_int("heal_per_second");
	if (m_conquer_radius > 0)
		m_workarea_info[m_conquer_radius].insert(descname() + _(" conquer"));
}

/**
===============
Create a new building of this type
===============
*/
Building & MilitarySite_Descr::create_object() const {
	return *new MilitarySite(*this);
}


/*
=============================

class MilitarySite

=============================
*/

MilitarySite::MilitarySite(const MilitarySite_Descr & ms_descr) :
ProductionSite(ms_descr),
m_soldier_upgrade_required_min(0),
m_soldier_upgrade_required_max(252),
m_soldier_normal_request(0),
m_soldier_upgrade_request(NULL),
m_didconquer  (false),
m_capacity    (ms_descr.get_max_number_of_soldiers()),
m_nexthealtime(0),
soldier_upgrade_try(false),
doing_upgrade_request(false)
{
	preferAnySoldiers();
}


MilitarySite::~MilitarySite()
{
	assert(!m_soldier_normal_request);
}


/**
===============
Display number of soldiers.
===============
*/
std::string MilitarySite::get_statistics_string()
{
	char buffer[255];
	std::string str;
	uint32_t present = presentSoldiers().size();
	uint32_t total = stationedSoldiers().size();

	if (present == total) {
		snprintf
			(buffer, sizeof(buffer),
			 ngettext("%u soldier", "%u soldiers", total),
			 total);
	} else {
		snprintf
			(buffer, sizeof(buffer),
			 ngettext("%u(+%u) soldier", "%u(+%u) soldiers", total),
			 present, total - present);
	}
	str = buffer;

	if (m_capacity > total) {
		snprintf(buffer, sizeof(buffer), " (+%u)", m_capacity - total);
		str += buffer;
	}

	return str;
}


void MilitarySite::init(Editor_Game_Base & egbase)
{
	ProductionSite::init(egbase);

	upcast(Game, game, &egbase);

	const std::vector<Worker*>& ws = get_workers();
	container_iterate_const(std::vector<Worker *>, ws, i)
		if (upcast(Soldier, soldier, *i.current)) {
			soldier->set_location_initially(*this);
			assert(not soldier->get_state()); //  Should be newly created.
			if (game)
				soldier->start_task_buildingwork(*game);
		}
	update_soldier_request();

	//  schedule the first healing
	m_nexthealtime = egbase.get_gametime() + 1000;
	if (game)
		schedule_act(*game, 1000);
}


/**
===============
Change the economy for the wares queues.
Note that the workers are dealt with in the PlayerImmovable code.
===============
*/
void MilitarySite::set_economy(Economy * const e)
{
	ProductionSite::set_economy(e);

	if (m_soldier_normal_request && e)
		m_soldier_normal_request->set_economy(e);
	if (m_soldier_upgrade_request && e)
		m_soldier_upgrade_request->set_economy(e);
}

/**
===============
Cleanup after a military site is removed
===============
*/
void MilitarySite::cleanup(Editor_Game_Base & egbase)
{
	// unconquer land
	if (m_didconquer)
		egbase.unconquer_area
			(Player_Area<Area<FCoords> >
			 	(owner().player_number(),
			 	 Area<FCoords>
			 	 	(egbase.map().get_fcoords(get_position()), get_conquers())),
			 m_defeating_player);

	ProductionSite::cleanup(egbase);

	// Note that removing workers during ProductionSite::cleanup can generate
	// new requests; that's why we delete it at the end of this function.
	delete m_soldier_normal_request;
	m_soldier_normal_request = 0;
	if (m_soldier_upgrade_request)
		delete m_soldier_upgrade_request;
	m_soldier_upgrade_request = 0;
}


/*
===============
Takes one soldier and adds him to ours

returns 0 on succes, -1 if there was no room for this soldier
===============
*/
int MilitarySite::incorporateSoldier(Editor_Game_Base & egbase, Soldier & s)
{

	if (s.get_location(egbase) != this)
	{
		s.set_location(this);
	}

	if (stationedSoldiers().size()  > descr().get_max_number_of_soldiers())
	{
		return incorporateUpgradedSoldier(egbase, s);
	}

	if (not m_didconquer) {
		conquer_area(egbase);
		// Building is now occupied - idle animation should be played
		start_animation(egbase, descr().get_animation("idle"));

		if (upcast(Game, game, &egbase)) {
			char message[256];
			snprintf
				(message, sizeof(message),
				 _("Your soldiers occupied your %s."),
				 descname().c_str());
			send_message
				(*game,
				 "site_occupied",
				 descname(),
				 message);
		}
	}

	if (upcast(Game, game, &egbase)) {
		// Bind the worker into this house, hide him on the map
		s.reset_tasks(*game);
		s.start_task_buildingwork(*game);
	}

	// Make sure the request count is reduced or the request is deleted.
	update_soldier_request_impl(true);

	return 0;
}

/*
 * Kicks out the least wanted soldier --
 * If player prefers zero-level guys, the most
 * trained soldier is the "weakest guy".
 */

bool
MilitarySite::drop_weakest_soldier(bool new_soldier_has_arrived, Soldier * newguy)
{
	std::vector<Soldier *> present = presentSoldiers();
	if (new_soldier_has_arrived or 1 < present.size())
	{
		bool heros = soldier_trainlevel_hero == soldier_preference;
		const int32_t multiplier = heros ? -1:1;
		static const int32_t level_offset = 10000;
		int32_t worst_soldier_level = 0;
		if (new_soldier_has_arrived)
			worst_soldier_level = level_offset + multiplier * newguy->get_level(atrTotal);
		Soldier* kickoutCandidate = NULL;
		for (uint32_t i = 0; i < present.size(); ++i)
		{
			int32_t this_soldier_level = level_offset + multiplier* present[i]->get_level(atrTotal);
			if (this_soldier_level > worst_soldier_level)
			{
				worst_soldier_level = this_soldier_level;
				kickoutCandidate = present[i];
			}
		}
		if (kickoutCandidate)
		{
			Game & game = ref_cast<Game, Editor_Game_Base>(owner().egbase());
			kickoutCandidate->reset_tasks(game);
			kickoutCandidate->start_task_leavebuilding(game, true);
			return true;
		}
	}
	return false;
}

// This find room for a soldier in an already full occupied military building.
int
MilitarySite::incorporateUpgradedSoldier(Editor_Game_Base & egbase, Soldier & s)
{

	if (drop_weakest_soldier(true, &s))
	{
		Game & game = ref_cast<Game, Editor_Game_Base>(egbase);
		s.set_location(this);
		s.reset_tasks(game);
		s.start_task_buildingwork(game);
		return 0;
	}
	return -1;
}
/*
===============
Called when our soldier arrives.
===============
*/
void MilitarySite::request_soldier_callback
	(Game            &       game,
	 Request         &,
	 Ware_Index,
	 Worker          * const w,
	 PlayerImmovable &       target)
{
	MilitarySite & msite = ref_cast<MilitarySite, PlayerImmovable>(target);
	Soldier      & s     = ref_cast<Soldier,      Worker>         (*w);

	msite.incorporateSoldier(game, s);
}


/**
 * Update the request for soldiers and cause soldiers to be evicted
 * as appropriate.
 */
void MilitarySite::update_normal_soldier_request()
{
	std::vector<Soldier *> present = presentSoldiers();
	uint32_t const stationed = stationedSoldiers().size();

	if (stationed < m_capacity) {
		if (!m_soldier_normal_request) {
			m_soldier_normal_request =
				new Request
					(*this,
					 tribe().safe_worker_index("soldier"),
					 MilitarySite::request_soldier_callback,
					 wwWORKER);
			m_soldier_normal_request->set_requirements (m_soldier_requirements);
		}

		m_soldier_normal_request->set_count(m_capacity - stationed);
	} else {
		delete m_soldier_normal_request;
		m_soldier_normal_request = 0;
	}

	if (m_capacity < present.size()) {
		Game & game = ref_cast<Game, Editor_Game_Base>(owner().egbase());
		for (uint32_t i = 0; i < present.size() - m_capacity; ++i) {
			Soldier & soldier = *present[i];
			soldier.reset_tasks(game);
			soldier.start_task_leavebuilding(game, true);
		}
	}
}
void MilitarySite::update_upgrade_soldier_request()
{
	bool reqch = update_upgrade_requirements();
	if (not soldier_upgrade_try)
		return;
	bool dosomething = reqch;

	if (NULL != m_soldier_upgrade_request)
	{
		if (not (m_soldier_upgrade_request->is_open()))
			dosomething = false;
		if (0 == m_soldier_upgrade_request->get_count())
			dosomething = true;
	}
	else
		dosomething = true;

	if (dosomething)
	{
		if (NULL != m_soldier_upgrade_request)
		{
			delete m_soldier_upgrade_request;
			m_soldier_upgrade_request = NULL;
		}

		m_soldier_upgrade_request =
				new Request
				(*this,
				tribe().safe_worker_index("soldier"),
				MilitarySite::request_soldier_callback,
				wwWORKER);


		m_soldier_upgrade_request->set_requirements (m_soldier_upgrade_requirements);

		m_soldier_upgrade_request->set_count(1);

	}
}

void MilitarySite::update_soldier_request_impl(bool incd)
{
	int32_t sc = soldierCapacity();
	int32_t sss = stationedSoldiers().size();

	if (doing_upgrade_request)
	{
		if (incd) // update requests always ask for one soldier at time!
		if (m_soldier_upgrade_request)
		{
			delete m_soldier_upgrade_request;
			m_soldier_upgrade_request = NULL;
		}
		if (sc > sss)
		{
			// Somebody is killing my soldiers in the middle of upgrade -- bad luck!
			if (NULL != m_soldier_upgrade_request)
			if (m_soldier_upgrade_request->is_open() or 0 == m_soldier_upgrade_request->get_count())
			{
				// Economy was not able to find the soldiers I need. Discarding request.

				delete m_soldier_upgrade_request;
				m_soldier_upgrade_request = NULL;
			}
			if (NULL == m_soldier_upgrade_request)
			{
				doing_upgrade_request = false;
				update_normal_soldier_request();
			}
			// else -- ohno please help me! Player is in trouble -- evil grin
		}
		else
		if (sc < sss) // player is reducing capacity
		{
			if (m_soldier_upgrade_request)
				delete m_soldier_upgrade_request;
			m_soldier_upgrade_request = NULL;
			doing_upgrade_request = false;
			update_normal_soldier_request();
		}
		else // capacity == stationed size
		{
			bool kick = false;
			if (NULL != m_soldier_upgrade_request)
			if (not m_soldier_upgrade_request->is_open())
			{
				// A new guy is arriving -- let's make room now.
				kick = true;
				drop_weakest_soldier(false, NULL);
			}
			if (not kick)
				update_upgrade_soldier_request();
		}
	}
	else // not doing upgrade request
	{
		if ((sc != sss) or (NULL != m_soldier_normal_request))
		{
			update_normal_soldier_request();
		}
		if ((sc == sss) and (NULL == m_soldier_normal_request))
		if (soldier_trainlevel_any != soldier_preference)
		{
			//log ("msited %4x debu usri switching to upgrade\n",
			//  (uint16_t)(((unsigned long) ((void*)this))&0xffff));
			int32_t pss = presentSoldiers().size();
			if (pss == sc)
			{
				doing_upgrade_request = true;
				update_upgrade_soldier_request();
			}
			// Note -- if there are non-present stationed soldiers, nothing gets
			// called. Therefore, I revisit this routine periodically without apparent
			// reason, hoping that all stationed soldiers would be present.
		}
	}
}
void MilitarySite::update_soldier_request()
{
	update_soldier_request_impl(false);
}

/*
===============
Advance the program state if applicable.
===============
*/


void MilitarySite::act(Game & game, uint32_t const data)
{
	// TODO: do all kinds of stuff, but if you do nothing, let
	// ProductionSite::act() handle all this. Also note, that some ProductionSite
	// commands rely, that ProductionSite::act() is not called for a certain
	// period (like cmdAnimation). This should be reworked.
	// Maybe a new queueing system like MilitaryAct could be introduced.

	ProductionSite::act(game, data);

	int32_t timeofgame = game.get_gametime();

	if (NULL != m_soldier_normal_request && NULL != m_soldier_upgrade_request)
	{
		log ("f978 MilitarySite::act: error: TWO REQUESTS ACTIVE!\n");
		exit (-1);
	}

	// I do not get a callback when stationed, non-present soldier returns --
	// Therefore I must poll in some occasions. Let's do that rather infrequently,
	// to keep the game lightweight.
	if ((soldier_trainlevel_any != soldier_preference) or doing_upgrade_request)
		if (timeofgame > next_swap_soldiers_time)
			{
				next_swap_soldiers_time = timeofgame + soldier_upgrade_try ? 20000 : 100000;
				update_soldier_request();
			}

	if (m_nexthealtime <= game.get_gametime()) {
		uint32_t total_heal = descr().get_heal_per_second();
		std::vector<Soldier *> soldiers = presentSoldiers();

		for (uint32_t i = 0; i < soldiers.size(); ++i) {
			Soldier & s = *soldiers[i];

			// The healing algorithm is totally arbitrary
			if (s.get_current_hitpoints() < s.get_max_hitpoints()) {
				s.heal(total_heal);
				break;
			}
		}

		m_nexthealtime = game.get_gametime() + 1000;
		schedule_act(game, 1000);
	}
}


/**
 * The worker is about to be removed.
 *
 * After the removal of the worker, check whether we need to request
 * new soldiers.
 */
void MilitarySite::remove_worker(Worker & w)
{
	ProductionSite::remove_worker(w);

	if (upcast(Soldier, soldier, &w))
		popSoldierJob(soldier, 0, 0);

	update_soldier_request();
}


/**
 * Called by soldiers in the building.
 */
bool MilitarySite::get_building_work(Game & game, Worker & worker, bool)
{
	if (upcast(Soldier, soldier, &worker)) {
		// Evict soldiers that have returned home if the capacity is too low
		if (m_capacity < presentSoldiers().size()) {
			worker.reset_tasks(game);
			worker.start_task_leavebuilding(game, true);
			return true;
		}

		bool stayhome;
		uint8_t retreat;
		if
			(Map_Object * const enemy
			 =
			 popSoldierJob(soldier, &stayhome, &retreat))
		{
			if (upcast(Building, building, enemy)) {
				soldier->start_task_attack
					(game, *building, retreat);
				return true;
			} else if (upcast(Soldier, opponent, enemy)) {
				if (!opponent->getBattle()) {
					soldier->start_task_defense
						(game, stayhome, retreat);
					if (stayhome)
						opponent->send_signal(game, "sleep");
					return true;
				}
			} else
				throw wexception("MilitarySite::get_building_work: bad SoldierJob");
		}
	}

	return false;
}


/**
 * \return \c true if the soldier is currently present and idle in the building.
 */
bool MilitarySite::isPresent(Soldier & soldier) const
{
	return
		soldier.get_location(owner().egbase()) == this                     &&
		soldier.get_state() == soldier.get_state(Worker::taskBuildingwork) &&
		soldier.get_position() == get_position();
}

std::vector<Soldier *> MilitarySite::presentSoldiers() const
{
	std::vector<Soldier *> soldiers;

	const std::vector<Worker *>& w = get_workers();
	container_iterate_const(std::vector<Worker *>, w, i)
		if (upcast(Soldier, soldier, *i.current))
			if (isPresent(*soldier))
				soldiers.push_back(soldier);

	return soldiers;
}

std::vector<Soldier *> MilitarySite::stationedSoldiers() const
{
	std::vector<Soldier *> soldiers;

	const std::vector<Worker *>& w = get_workers();
	container_iterate_const(std::vector<Worker *>, w, i)
		if (upcast(Soldier, soldier, *i.current))
			soldiers.push_back(soldier);

	return soldiers;
}

uint32_t MilitarySite::minSoldierCapacity() const throw () {
	return 1;
}
uint32_t MilitarySite::maxSoldierCapacity() const throw () {
	return descr().get_max_number_of_soldiers();
}
uint32_t MilitarySite::soldierCapacity() const
{
	return m_capacity;
}

void MilitarySite::setSoldierCapacity(uint32_t const capacity) {
	assert(minSoldierCapacity() <= capacity);
	assert                        (capacity <= maxSoldierCapacity());
	assert(m_capacity != capacity);
	m_capacity = capacity;
	update_soldier_request();
}

void MilitarySite::dropSoldier(Soldier & soldier)
{
	Game & game = ref_cast<Game, Editor_Game_Base>(owner().egbase());

	if (!isPresent(soldier)) {
		// This can happen when the "drop soldier" player command is delayed
		// by network delay or a client has bugs.
		molog("MilitarySite::dropSoldier(%u): not present\n", soldier.serial());
		return;
	}
	if (presentSoldiers().size() <= minSoldierCapacity()) {
		molog("cannot drop last soldier(s)\n");
		return;
	}

	soldier.reset_tasks(game);
	soldier.start_task_leavebuilding(game, true);

	update_soldier_request();
}


void MilitarySite::conquer_area(Editor_Game_Base & egbase) {
	assert(not m_didconquer);
	egbase.conquer_area
		(Player_Area<Area<FCoords> >
		 	(owner().player_number(),
		 	 Area<FCoords>
		 	 	(egbase.map().get_fcoords(get_position()), get_conquers())));
	m_didconquer = true;
}


bool MilitarySite::canAttack()
{
	return m_didconquer;
}

void MilitarySite::aggressor(Soldier & enemy)
{
	Game & game = ref_cast<Game, Editor_Game_Base>(owner().egbase());
	Map  & map  = game.map();
	if
		(enemy.get_owner() == &owner() ||
		 enemy.getBattle() ||
		 get_conquers()
		 <=
		 map.calc_distance(enemy.get_position(), get_position()))
		return;

	if
		(map.find_bobs
		 	(Area<FCoords>(map.get_fcoords(base_flag().get_position()), 2),
		 	 0,
		 	 FindBobEnemySoldier(&owner())))
		return;

	// We're dealing with a soldier that we might want to keep busy
	// Now would be the time to implement some player-definable
	// policy as to how many soldiers are allowed to leave as defenders
	std::vector<Soldier *> present = presentSoldiers();

	if (1 < present.size())
		container_iterate_const(std::vector<Soldier *>, present, i)
			if (!haveSoldierJob(**i.current)) {
				SoldierJob sj;
				sj.soldier  = *i.current;
				sj.enemy = &enemy;
				sj.stayhome = false;
				sj.retreat = owner().get_retreat_percentage();
				m_soldierjobs.push_back(sj);
				(*i.current)->update_task_buildingwork(game);
				return;
			}

	// Inform the player, that we are under attack by adding a new entry to the
	// message queue - a sound will automatically be played.
	informPlayer(game, true);
}

bool MilitarySite::attack(Soldier & enemy)
{
	Game & game = ref_cast<Game, Editor_Game_Base>(owner().egbase());

	std::vector<Soldier *> present = presentSoldiers();
	Soldier * defender = 0;

	if (!present.empty()) {
		// Find soldier with greatest hitpoints
		uint32_t current_max = 0;
		container_iterate_const(std::vector<Soldier *>, present, i)
			if ((*i.current)->get_current_hitpoints() > current_max) {
				defender = *i.current;
				current_max = defender->get_current_hitpoints();
			}
	} else {
		// If one of our stationed soldiers is currently walking into the
		// building, give us another chance.
		std::vector<Soldier *> stationed = stationedSoldiers();
		container_iterate_const(std::vector<Soldier *>, stationed, i)
			if ((*i.current)->get_position() == get_position()) {
				defender = *i.current;
				break;
			}
	}

	if (defender) {
		popSoldierJob(defender); // defense overrides all other jobs

		SoldierJob sj;
		sj.soldier = defender;
		sj.enemy = &enemy;
		sj.stayhome = true;
		sj.retreat = 0;         // Flag defenders could not retreat
		m_soldierjobs.push_back(sj);

		defender->update_task_buildingwork(game);

		// Inform the player, that we are under attack by adding a new entry to
		// the message queue - a sound will automatically be played.
		informPlayer(game);

		return true;
	} else {
		// The enemy has defeated our forces, we should inform the player
		const Coords coords = get_position();
		{
			char message[2048];
			snprintf
				(message, sizeof(message),
				 _("The enemy defeated your soldiers at the %s."),
				 descname().c_str());
			send_message
				(game,
				 "site_lost",
				 _("Militarysite lost!"),
				 message);
		}

		// Now let's see whether the enemy conquers our militarysite, or whether
		// we still hold the bigger military presence in that area (e.g. if there
		// is a fortress one or two points away from our sentry, the fortress has
		// a higher presence and thus the enemy can just burn down the sentry.
		if (military_presence_kept(game)) {
			// Okay we still got the higher military presence, so the attacked
			// militarysite will be destroyed.
			set_defeating_player(enemy.owner().player_number());
			schedule_destroy(game);
			return false;
		}

		// The enemy conquers the building
		// In fact we do not conquer it, but place a new building of same type at
		// the old location.
		Player            * enemyplayer = enemy.get_owner();
		const Tribe_Descr & enemytribe  = enemyplayer->tribe();
		std::string bldname = name();

		// Has this building already a suffix? == conquered building?
		std::string::size_type const dot = bldname.rfind('.');
		if (dot >= bldname.size()) {
			// Add suffix, if the new owner uses another tribe than we.
			if (enemytribe.name() != owner().tribe().name())
				bldname += "." + owner().tribe().name();
		} else if (enemytribe.name() == bldname.substr(dot + 1, bldname.size()))
			bldname = bldname.substr(0, dot);
		Building_Index bldi = enemytribe.safe_building_index(bldname.c_str());

		// Now we destroy the old building before we place the new one.
		set_defeating_player(enemy.owner().player_number());
		schedule_destroy(game);

		enemyplayer->force_building(coords, bldi);
		BaseImmovable * const newimm = game.map()[coords].get_immovable();
		upcast(MilitarySite, newsite, newimm);
		newsite->reinit_after_conqueration(game);

		// Of course we should inform the victorious player as well
		char message[2048];
		snprintf
			(message, sizeof(message),
			 _("Your soldiers defeated the enemy at the %s."),
			 newsite->descname().c_str());
		newsite->send_message
			(game,
			 "site_defeated",
			 _("Enemy at site defeated!"),
			 message);

		return false;
	}
}

/// Initialises the militarysite after it was "conquered" (the old was replaced)
void MilitarySite::reinit_after_conqueration(Game & game)
{
	clear_requirements();
	conquer_area(game);
	update_soldier_request();
	start_animation(game, descr().get_animation("idle"));
}

/// Calculates whether the military presence is still kept and \returns true if.
bool MilitarySite::military_presence_kept(Game & game)
{
	// collect information about immovables in the area
	std::vector<ImmovableFound> immovables;

	// Search in a radius of 3 (needed for big militarysites)
	FCoords const fc = game.map().get_fcoords(get_position());
	game.map().find_immovables(Area<FCoords>(fc, 3), &immovables);

	for (uint32_t i = 0; i < immovables.size(); ++i)
		if (upcast(MilitarySite const, militarysite, immovables[i].object))
			if
				(this       !=  militarysite          and
				 &owner  () == &militarysite->owner() and
				 get_size() <=  militarysite->get_size() and
				 militarysite->m_didconquer)
				return true;
	return false;
}

/// Informs the player about an attack of his opponent.
void MilitarySite::informPlayer(Game & game, bool const discovered)
{
	char message[2048];
	snprintf
		(message, sizeof(message),
		 discovered ?
		 _("Your %s discovered an aggressor.</p>") :
		 _("Your %s is under attack.</p>"),
		 descname().c_str());

	// Add a message as long as no previous message was send from a point with
	// radius <= 5 near the current location in the last 60 seconds
	send_message
		(game,
		 "under_attack",
		 _("You are under attack"),
		 message,
		 60 * 1000, 5);
}


/*
   MilitarySite::set_requirements

   Easy to use, overwrite with given requirements.
*/
void MilitarySite::set_requirements (const Requirements & r)
{
	m_soldier_requirements = r;
}

/*
   MilitarySite::clear_requirements

   This should cancel any requirement pushed at this house
*/
void MilitarySite::clear_requirements ()
{
	m_soldier_requirements = Requirements();
}

void MilitarySite::sendAttacker
	(Soldier & soldier, Building & target, uint8_t retreat)
{
	assert(isPresent(soldier));

	if (haveSoldierJob(soldier))
		return;

	SoldierJob sj;
	sj.soldier  = &soldier;
	sj.enemy    = &target;
	sj.stayhome = false;
	sj.retreat  = retreat;
	m_soldierjobs.push_back(sj);

	soldier.update_task_buildingwork
		(ref_cast<Game, Editor_Game_Base>(owner().egbase()));
}


bool MilitarySite::haveSoldierJob(Soldier & soldier)
{
	container_iterate_const(std::vector<SoldierJob>, m_soldierjobs, i)
		if (i.current->soldier == &soldier)
			return true;

	return false;
}


/**
 * \return the enemy, if any, that the given soldier was scheduled
 * to attack, and remove the job.
 */
Map_Object * MilitarySite::popSoldierJob
	(Soldier * const soldier, bool * const stayhome, uint8_t * const retreat)
{
	container_iterate(std::vector<SoldierJob>, m_soldierjobs, i)
		if (i.current->soldier == soldier) {
			Map_Object * const enemy = i.current->enemy.get(owner().egbase());
			if (stayhome)
				*stayhome = i.current->stayhome;
			if (retreat)
				*retreat = i.current->retreat;
			m_soldierjobs.erase(i.current);
			return enemy;
		}
	return 0;
}


bool
MilitarySite::update_upgrade_requirements()
{
	// Fixme -- here are bugs -- find and fix
	bool heros = true;
	switch (soldier_preference)
	{
		case soldier_trainlevel_hero:
			heros = true;
			break;
		case soldier_trainlevel_rookie:
			heros = false;
			break;
		default:
			log("MilitarySite::swapSoldiers: error: Unknown player preference %d.\n", soldier_preference);
			soldier_upgrade_try = false;
			return false;
	}

	std::vector<Soldier *> svec = stationedSoldiers();
	int32_t multiplier = heros ? -1:1;
	int32_t wg_level = 0;
	int32_t level_offset = 10000;
	int32_t wg_actual_level = heros ? 0 : 101;
	for (uint32_t i = 0; i < svec.size(); ++i)
	{
		int32_t this_soldier_level = level_offset + multiplier* svec[i]->get_level(atrTotal);
		if (this_soldier_level > wg_level)
		{
			wg_level = this_soldier_level;
			wg_actual_level = svec[i]->get_level(atrTotal);

		}
	}
	soldier_upgrade_try = true;
	if (! heros)
		if (level_offset == wg_level)
			{
				soldier_upgrade_try = false;
				return false;
			}
	int32_t reqmin = heros ? 1 + wg_actual_level : 0;
	int32_t reqmax = heros ? 10000 : wg_actual_level - 1;

	bool maxchanged = reqmax != static_cast<int32_t>(m_soldier_upgrade_required_max);
	bool minchanged = reqmin != static_cast<int32_t>(m_soldier_upgrade_required_min);

	if (maxchanged or minchanged)
	{
		if (NULL == m_soldier_normal_request or (m_soldier_normal_request->is_open()))
		{
			if (m_soldier_normal_request)
			{
				delete m_soldier_normal_request;
				m_soldier_normal_request = 0;
			}
			m_soldier_upgrade_requirements = RequireAttribute(atrTotal, reqmin, reqmax);
			m_soldier_upgrade_required_max = reqmax;
			m_soldier_upgrade_required_min = reqmin;

			return true;
		}
	}

	return false;
}

void
MilitarySite::preferSkilledSoldiers()
{
	soldier_preference = soldier_trainlevel_hero;
}

void
MilitarySite::preferAnySoldiers()
{
	soldier_preference = soldier_trainlevel_any;
}
void
MilitarySite::preferCheapSoldiers()
{
	soldier_preference = soldier_trainlevel_rookie;
}

bool
MilitarySite::preferringSkilledSoldiers() const
{
	return  soldier_trainlevel_hero == soldier_preference;
}

bool
MilitarySite::preferringAnySoldiers() const
{
	return  soldier_trainlevel_any == soldier_preference;
}
bool
MilitarySite::preferringCheapSoldiers() const
{
	return  soldier_trainlevel_rookie == soldier_preference;
}


}
