/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
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

#ifndef TRAININGSITE_H
#define TRAININGSITE_H

#include "productionsite.h"

class TrainingSite_Window;

class TrainingSite_Descr:public ProductionSite_Descr {
      public:

	TrainingSite_Descr(Tribe_Descr * tribe, const char *name);
	 virtual ~ TrainingSite_Descr();

	virtual void parse(const char *directory, Profile * prof, const EncodeData * encdata);
	virtual Building *create_object();

	virtual bool is_only_production_site(void) {
		return true;
	} inline int get_max_number_of_soldiers(void) {
		return m_num_soldiers;
	}
	inline bool get_train_hp(void) {
		return m_train_hp;
	}
	inline bool get_train_attack(void) {
		return m_train_attack;
	}
	inline bool get_train_defense(void) {
		return m_train_defense;
	}
	inline bool get_train_evade(void) {
		return m_train_evade;
	}

	int get_min_level(tAttribute);
	int get_max_level(tAttribute);
      private:
	/** Maximum number of soldiers for a training site*/
	int m_num_soldiers;
	/** Whether this site can train hitpoints*/
	bool m_train_hp;
	/** Whether this site can train attack*/
	bool m_train_attack;
	/** Whether this site can train defense*/
	bool m_train_defense;
	/** Whether this site can train evasion*/
	bool m_train_evade;

	/** Minimum hitpoints to which a soldier can drop at this site*/
	int m_min_hp;
	/** Minimum attacks to which a soldier can drop at this site*/
	int m_min_attack;
	/** Minimum defense to which a soldier can drop at this site*/
	int m_min_defense;
	/** Minimum evasion to which a soldier can drop at this site*/
	int m_min_evade;

	/** Maximum hitpoints a soldier can acquire at this site*/
	int m_max_hp;
	/** Maximum attack a soldier can acquire at this site*/
	int m_max_attack;
	/** Maximum defense a soldier can acquire at this site*/
	int m_max_defense;
	/** Maximum evasion a soldier can acquire at this site*/
	int m_max_evade;

	// Re-use of m_inputs to get the resources
	// TrainingMap m_programs;
};

/**
 * A building to change soldiers' abilities.
 * Soldiers can gain hitpoints, or experience in attack, defense and evasion.
 *
 * \note A training site does not change influence areas. If you lose the surrounding strongholds, the
 *	 training site will burn even if it contains soldiers!
 */
class TrainingSite:public ProductionSite {
	friend class Widelands_Map_Buildingdata_Data_Packet;
	 MO_DESCR(TrainingSite_Descr);
	friend class TrainingSite_Window;
      public:
	 TrainingSite(TrainingSite_Descr * descr);
	 virtual ~ TrainingSite();

	virtual int get_building_type(void) {
		return Building::TRAININGSITE;
	} virtual std::string get_statistics_string();

	virtual void init(Editor_Game_Base * g);
	virtual void cleanup(Editor_Game_Base * g);
	virtual void act(Game * g, uint data);

	inline bool get_build_heros(void) {
		return m_build_heros;
	}
	inline void set_build_heros(bool b_heros) {
		m_build_heros = b_heros;
	}
	inline void switch_heros(void) {
		m_build_heros = !m_build_heros;
		molog("BUILD_HEROS: %s", m_build_heros ? "TRUE" : "FALSE");
	}

	virtual void set_economy(Economy * e);

	virtual std::vector < Soldier * >*get_soldiers(void) {
		return &m_soldiers;
	}

	inline TrainingSite_Descr *get__descr() {
		return get_descr();
	}

	virtual void drop_soldier(uint nr);
	uint get_pri(enum tAttribute atr);
	void add_pri(enum tAttribute atr);
	void sub_pri(enum tAttribute atr);
	uint get_capacity() {
		return m_capacity;
	}
	virtual void soldier_capacity_up() {
		change_soldier_capacity(1);
	}
	virtual void soldier_capacity_down() {
		change_soldier_capacity(-1);
	}
      protected:
	virtual void change_soldier_capacity(int);
	virtual UIWindow *create_options_window(Interactive_Player * plr, UIWindow ** registry);

      private:
	void request_soldier(Game * g);
	static void request_soldier_callback(Game * g, Request * rq, int ware, Worker * w, void *data);

	void program_start(Game * g, std::string name);
	void program_end(Game * g, bool success);
	void find_and_start_next_program(Game * g);
	void calc_list_upgrades(Game * g);

	void call_soldiers(Game * g);
	void drop_soldier(Game * g, uint nr);
	void drop_unupgradable_soldiers(Game * g);

	void modif_priority(enum tAttribute, int);
      private:
	/** Open requests for soldiers. The soldiers can be under way or unavailable*/
	std::vector < Request * >m_soldier_requests;

	/** The soldiers currently at the training site*/
	std::vector < Soldier * >m_soldiers;

	/** Number of soldiers that should be trained concurrently.
	 * Equal or less to maximum number of soldiers supported by a training site. There is no
	 * guarantee there really are m_capacity soldiers in the building - some of them might
	 * still be under way or even not yet available*/
	uint m_capacity;

	/** Number of available soldiers + number of requested soldiers. \todo Factor out this variable?*/
	uint m_total_soldiers;

	/** Possible upgrades to be gained at this site*/
	std::vector < std::string > m_list_upgrades;

	/** True, \b always upgrade already experienced soldiers first
	 * False, \b always upgrade inexperienced soldiers first*/
	bool m_build_heros;

	/** Priority for upgrading hitpoints (0 to 2) 0 = don't upgrade, 2 = upgrade urgently
	 * \sa m_pri_attack, m_pri_defense, m_pri_evade*/
	uint m_pri_hp;

	/** Priority for upgrading hitpoints (0 to 2) 0 = don't upgrade, 2 = upgrade urgently
	 * \sa m_pri_hp, m_pri_defense, m_pri_evade*/
	uint m_pri_attack;

	/** Priority for upgrading hitpoints (0 to 2) 0 = don't upgrade, 2 = upgrade urgently
	 * \sa m_pri_hp, m_pri_attack, m_pri_evade*/
	uint m_pri_defense;

	/** Priority for upgrading hitpoints (0 to 2) 0 = don't upgrade, 2 = upgrade urgently
	 * \sa m_pri_hp, m_pri_attack, m_pri_defense*/
	uint m_pri_evade;

	/** Modificator for \ref m_pri_hp, needed to prevent infinite loop when upgrading is not
	 * possible immediately*/
	int m_pri_hp_mod;
	/** Modificator for \ref m_pri_attack, needed to prevent infinite loop when upgrading is not
	 * possible immediately*/
	int m_pri_attack_mod;
	/** Modificator for \ref m_pri_defense, needed to prevent infinite loop when upgrading is not
	 * possible immediately*/
	int m_pri_defense_mod;
	/** Modificator for \ref m_pri_evade, needed to prevent infinite loop when upgrading is not
	 * possible immediately*/
	int m_pri_evade_mod;

	/** Whether the last training program was finished successfully*/
	bool m_success;

	/** The training program that is currently executing*/
	std::string m_prog_name;
};

#endif
