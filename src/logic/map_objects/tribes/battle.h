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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef WL_LOGIC_MAP_OBJECTS_TRIBES_BATTLE_H
#define WL_LOGIC_MAP_OBJECTS_TRIBES_BATTLE_H

#include "logic/map_objects/map_object.h"

namespace Widelands {
class Soldier;

class BattleDescr : public MapObjectDescr {
public:
	BattleDescr(char const* const _name, char const* const _descname)
		: MapObjectDescr(MapObjectType::BATTLE, _name, _descname) {
	}
	~BattleDescr() override {
	}

private:
	DISALLOW_COPY_AND_ASSIGN(BattleDescr);
};

/**
 * Manages the battle between two opposing soldiers.
 *
 * A \ref Battle object is created using the \ref create() function as soon as
 * a soldier decides he wants to attack someone else. A battle always has both
 * Soldiers defined, the battle object must be destroyed as soon as there is no
 * other Soldier to battle anymore.
 */
class Battle : public MapObject {
public:
	const BattleDescr& descr() const;

	Battle(); //  for loading an existing battle from a savegame
	Battle(Game &, Soldier &, Soldier &); //  to create a new battle in the game

	// Implements MapObject.
	void init(EditorGameBase &) override;
	void cleanup(EditorGameBase &) override;
	bool has_new_save_support() override {return true;}
	void save(EditorGameBase &, MapObjectSaver &, FileWrite &) override;
	static MapObject::Loader * load
		(EditorGameBase &, MapObjectLoader &, FileRead &);

	// Cancel this battle immediately and schedule destruction.
	void cancel(Game &, Soldier &);

	// Returns true if the battle should not be interrupted.
	bool locked(Game &);

	// The two soldiers involved in this fight.
	Soldier * first() {return m_first;}
	Soldier * second() {return m_second;}

	// Returns the other soldier involved in this battle. CHECKs that the given
	// soldier is participating in this battle. Can return nullptr, but I have
	// no idea what that means.
	Soldier * opponent(Soldier &);

	// Called by the battling soldiers once they've met on a common node and are
	// idle.
	void get_battle_work(Game &, Soldier &);

private:
	struct Loader : public MapObject::Loader {
		virtual void load(FileRead &);
		void load_pointers() override;

		Serial m_first;
		Serial m_second;
	};

	void calculate_round(Game &);

	Soldier * m_first;
	Soldier * m_second;

	/**
	 * Gametime when the battle was created.
	 */
	int32_t m_creationtime;

	/**
	 * 1 if only the first soldier is ready, 2 if only the second soldier
	 * is ready, 3 if both are ready.
	 */
	uint8_t m_readyflags;

	/**
	 * Damage pending to apply. Damage is applied at end of round so animations
	 * can show current action.
	 */
	uint32_t m_damage;

	/**
	 * \c true if the first soldier is the next to strike.
	 */
	bool m_first_strikes;

	/**
	 * \c true if the last turn attacker damaged his opponent
	 */
	bool m_last_attack_hits;
};

}

#endif  // end of include guard: WL_LOGIC_MAP_OBJECTS_TRIBES_BATTLE_H
