/*
 * Copyright (C) 2002-2004, 2006-2009 by the Widelands Development Team
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

#ifndef TRAININGSITE_H
#define TRAININGSITE_H

#include "productionsite.h"
#include "soldiercontrol.h"
#include "tattribute.h"

struct TrainingSite_Window;

namespace Widelands {

struct TrainingSite_Descr : public ProductionSite_Descr {
	TrainingSite_Descr
		(char const * name, char const * descname,
		 const std::string & directory, Profile &, Section & global_s,
		 const Tribe_Descr & tribe);

	virtual Building & create_object() const;

	uint32_t get_max_number_of_soldiers() const throw () {
		return m_num_soldiers;
	}
	bool get_train_hp     () const throw () {return m_train_hp;}
	bool get_train_attack () const throw () {return m_train_attack;}
	bool get_train_defense() const throw () {return m_train_defense;}
	bool get_train_evade  () const throw () {return m_train_evade;}

	int32_t get_min_level(tAttribute) const;
	int32_t get_max_level(tAttribute) const;
	int32_t get_max_stall() const;
private:
	//  FIXME These variables should be per soldier type. They should be in a
	//  FIXME struct and there should be a vector, indexed by Soldier_Index,
	//  FIXME with that struct structs as element type.

	/** Maximum number of soldiers for a training site*/
	uint32_t m_num_soldiers;
	/** Number of rounds w/o successful training, after which a soldier is kicked out.**/
	uint32_t m_max_stall;
	/** Whether this site can train hitpoints*/
	bool m_train_hp;
	/** Whether this site can train attack*/
	bool m_train_attack;
	/** Whether this site can train defense*/
	bool m_train_defense;
	/** Whether this site can train evasion*/
	bool m_train_evade;

	/** Minimum hitpoints to which a soldier can drop at this site*/
	int32_t m_min_hp;
	/** Minimum attacks to which a soldier can drop at this site*/
	int32_t m_min_attack;
	/** Minimum defense to which a soldier can drop at this site*/
	int32_t m_min_defense;
	/** Minimum evasion to which a soldier can drop at this site*/
	int32_t m_min_evade;

	/** Maximum hitpoints a soldier can acquire at this site*/
	int32_t m_max_hp;
	/** Maximum attack a soldier can acquire at this site*/
	int32_t m_max_attack;
	/** Maximum defense a soldier can acquire at this site*/
	int32_t m_max_defense;
	/** Maximum evasion a soldier can acquire at this site*/
	int32_t m_max_evade;

	// Re-use of m_inputs to get the resources
	// TrainingMap m_programs;
};

/**
 * A building to change soldiers' abilities.
 * Soldiers can gain hitpoints, or experience in attack, defense and evasion.
 *
 * \note  A training site does not change influence areas. If you lose the
 *        surrounding strongholds, the training site will burn even if it
 *        contains soldiers!
 */
class TrainingSite : public ProductionSite, public SoldierControl {
	friend struct Map_Buildingdata_Data_Packet;
	MO_DESCR(TrainingSite_Descr);
	friend struct ::TrainingSite_Window;

	struct Upgrade {
		tAttribute attribute; // attribute for this upgrade
		std::string prefix; // prefix for programs
		int32_t min, max; // minimum and maximum program number (inclusive)
		uint32_t prio; // relative priority
		uint32_t credit; // whenever an upgrade gets credit >= 10, it can be run
		int32_t lastattempt; // level of the last attempt in this upgrade category

		// whether the last attempt in this upgrade category was successful
		bool lastsuccess;
		uint32_t failures;
	};

public:
	TrainingSite(const TrainingSite_Descr &);

	char const * type_name() const throw () {return "trainingsite";}
	virtual std::string get_statistics_string();

	virtual void init(Editor_Game_Base &);
	virtual void cleanup(Editor_Game_Base &);
	virtual void act(Game &, uint32_t data);

	virtual void add_worker   (Worker &);
	virtual void remove_worker(Worker &);

	bool get_build_heros() {
		return m_build_heros;
	}
	void set_build_heros(bool b_heros) {
		m_build_heros = b_heros;
	}
	void switch_heros() {
		m_build_heros = !m_build_heros;
		molog("BUILD_HEROS: %s", m_build_heros ? "TRUE" : "FALSE");
	}

	virtual void set_economy(Economy * e);

	// Begin implementation of SoldierControl
	virtual std::vector<Soldier *> presentSoldiers() const;
	virtual std::vector<Soldier *> stationedSoldiers() const;
	virtual uint32_t minSoldierCapacity() const throw ();
	virtual uint32_t maxSoldierCapacity() const throw ();
	virtual uint32_t soldierCapacity() const;
	virtual void setSoldierCapacity(uint32_t capacity);
	virtual void dropSoldier(Soldier &);
	int incorporateSoldier(Editor_Game_Base &, Soldier &);
	// End implementation of SoldierControl

	int32_t get_pri(enum tAttribute atr);
	void set_pri(enum tAttribute atr, int32_t prio);

	// These are for premature soldier kick-out
	void trainingAttempted(uint32_t type, uint32_t level);
	void trainingSuccessful(uint32_t type, uint32_t level);
	void trainingDone();


protected:
	virtual void create_options_window
		(Interactive_GameBase &, UI::Window * & registry);
	virtual void program_end(Game &, Program_Result);

private:
	void update_soldier_request();
	static void request_soldier_callback
		(Game &, Request &, Ware_Index, Worker *, PlayerImmovable &);

	void find_and_start_next_program(Game &);
	void start_upgrade(Game &, Upgrade &);
	void add_upgrade(tAttribute, const std::string & prefix);
	void calc_upgrades();

	void drop_unupgradable_soldiers(Game &);
	void drop_stalled_soldiers(Game &);
	Upgrade * get_upgrade(tAttribute);

private:
	/// Open requests for soldiers. The soldiers can be under way or unavailable
	Request * m_soldier_request;

	/** The soldiers currently at the training site*/
	std::vector<Soldier *> m_soldiers;

	/** Number of soldiers that should be trained concurrently.
	 * Equal or less to maximum number of soldiers supported by a training site.
	 * There is no guarantee there really are m_capacity soldiers in the
	 * building - some of them might still be under way or even not yet
	 * available*/
	uint32_t m_capacity;

	/** True, \b always upgrade already experienced soldiers first, when possible
	 * False, \b always upgrade inexperienced soldiers first, when possible */
	bool m_build_heros;

	std::vector<Upgrade> m_upgrades;
	Upgrade * m_current_upgrade;

	Program_Result m_result; /// The result of the last training program.

	// These are used for kicking out soldiers prematurely
	static const uint32_t training_state_multiplier = 12;
	typedef std::pair<uint32_t, uint32_t> TypeAndLevel_t;
	typedef std::map<TypeAndLevel_t, uint32_t> TrainFailCount_t;
	TrainFailCount_t training_failure_count; // FIXME: should this go to a savegame?
	uint32_t max_stall_val;
	void init_kick_state(const tAttribute&, const TrainingSite_Descr&);


};

}

#endif
