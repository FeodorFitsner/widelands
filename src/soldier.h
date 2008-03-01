/*
 * Copyright (C) 2002-2004, 2006-2008 by the Widelands Development Team
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

#ifndef SOLDIER_H
#define SOLDIER_H

#include "worker.h"

struct RGBColor;

namespace Widelands {

// Constants used to launch attacks
#define WEAKEST   0
#define STRONGEST 1

class Editor_Game_Base;
class AttackController;

#define HP_FRAMECOLOR RGBColor(255, 255, 255)

struct Soldier_Descr : public Worker_Descr {
	Soldier_Descr(const Tribe_Descr &, const std::string & soldier_name);

	virtual Worker_Type get_worker_type() const {return SOLDIER;}

	virtual void load_graphics();

	uint32_t get_max_hp_level          () const {return m_max_hp_level;}
	uint32_t get_max_attack_level      () const {return m_max_attack_level;}
	uint32_t get_max_defense_level     () const {return m_max_defense_level;}
	uint32_t get_max_evade_level       () const {return m_max_evade_level;}

	uint32_t get_min_hp                () const {return m_min_hp;}
	uint32_t get_max_hp                () const {return m_max_hp;}
	uint32_t get_min_attack            () const {return m_min_attack;}
	uint32_t get_max_attack            () const {return m_max_attack;}
	uint32_t get_defense               () const {return m_defense;}
	uint32_t get_evade                 () const {return m_evade;}

	uint32_t get_hp_incr_per_level     () const {return m_hp_incr;}
	uint32_t get_attack_incr_per_level () const {return m_attack_incr;}
	uint32_t get_defense_incr_per_level() const {return m_defense_incr;}
	uint32_t get_evade_incr_per_level  () const {return m_evade_incr;}

	uint32_t get_hp_level_pic     (const uint32_t level) const
	{assert(level <= m_max_hp_level);      return m_hp_pics     [level];}
	uint32_t get_attack_level_pic (const uint32_t level) const
	{assert(level <= m_max_attack_level);  return m_attack_pics [level];}
	uint32_t get_defense_level_pic(const uint32_t level) const
	{assert(level <= m_max_defense_level); return m_defense_pics[level];}
	uint32_t get_evade_level_pic  (const uint32_t level) const
	{assert(level <= m_max_evade_level);   return m_evade_pics  [level];}



	uint32_t get_rand_anim(const char * const name) const;
protected:
	virtual Bob * create_object() const;
	virtual void parse
		(char const * dir, Profile *, becomes_map_t &, EncodeData const *);

	//  start values
	uint32_t m_min_hp;
	uint32_t m_max_hp;
	uint32_t m_min_attack;
	uint32_t m_max_attack;
	uint32_t m_defense;
	uint32_t m_evade;

	//  per level increases
	uint32_t m_hp_incr;
	uint32_t m_attack_incr;
	uint32_t m_defense_incr;
	uint32_t m_evade_incr;

	//  max levels
	uint32_t m_max_hp_level;
	uint32_t m_max_attack_level;
	uint32_t m_max_defense_level;
	uint32_t m_max_evade_level;

	//  level pictures
	std::vector<uint32_t>    m_hp_pics;
	std::vector<uint32_t>    m_attack_pics;
	std::vector<uint32_t>    m_evade_pics;
	std::vector<uint32_t>    m_defense_pics;
	std::vector<std::string> m_hp_pics_fn;
	std::vector<std::string> m_attack_pics_fn;
	std::vector<std::string> m_evade_pics_fn;
	std::vector<std::string> m_defense_pics_fn;
};

class Soldier : public Worker {
	friend struct Map_Bobdata_Data_Packet;
	MO_DESCR(Soldier_Descr);

public:
	Soldier(const Soldier_Descr &);

	virtual void init(Editor_Game_Base *);

	void set_level
		(uint32_t hp, uint32_t attack, uint32_t defense, uint32_t evade);
	void set_hp_level     (uint32_t);
	void set_attack_level (uint32_t);
	void set_defense_level(uint32_t);
	void set_evade_level  (uint32_t);
	uint32_t get_level (tAttribute) const;
	uint32_t get_hp_level     () const {return m_hp_level;}
	uint32_t get_attack_level () const {return m_attack_level;}
	uint32_t get_defense_level() const {return m_defense_level;}
	uint32_t get_evade_level  () const throw () {return m_evade_level;}

	/// Draw this soldier
	virtual void draw
		(const Editor_Game_Base &, RenderTarget &, const Point) const;

	//  Information function from description.
	uint32_t get_max_hp_level () const throw ()
	{return descr().get_max_hp_level();}
	uint32_t get_max_attack_level () const throw ()
	{return descr().get_max_attack_level();}
	uint32_t get_max_defense_level() const throw ()
	{return descr().get_max_defense_level();}
	uint32_t get_max_evade_level  () const throw ()
	{return descr().get_max_evade_level();}

	uint32_t get_current_hitpoints() const {return m_hp_current;}
	uint32_t get_max_hitpoints    () const {return m_hp_max;}
	uint32_t get_min_attack       () const {return m_min_attack;}
	uint32_t get_max_attack       () const {return m_max_attack;}
	uint32_t get_defense          () const {return m_defense;}
	uint32_t get_evade            () const {return m_evade;}

	uint32_t get_hp_level_pic     () const
	{return descr().get_hp_level_pic     (m_hp_level);}
	uint32_t get_attack_level_pic () const
	{return descr().get_attack_level_pic (m_attack_level);}
	uint32_t get_defense_level_pic() const
	{return descr().get_defense_level_pic(m_defense_level);}
	uint32_t get_evade_level_pic  () const
	{return descr().get_evade_level_pic  (m_evade_level);}

	bool have_tattributes() const;
	int32_t get_tattribute(uint32_t attr) const;

	/// Sets a random animation of desired type and start playing it.
	void start_animation
		(Editor_Game_Base*, const char * const animname, const uint32_t time);

	/// Heal quantity of hit points instantly
	void heal (uint32_t);

	void damage (uint32_t); /// Damage quantity of hit points

public: // Worker-specific redefinitions
	void startTaskMoveToBattle(Game *, Flag *, Coords);
	void startTaskMoveHome(Game *);

	void log_general_info(Editor_Game_Base *);

	void set_attack_ctrl(AttackController* ctrl) {m_attack_ctrl = ctrl;};

private:
	void moveToBattleUpdate (Game *, State *);
	void moveToBattleSignal (Game *, State *);

	void moveHomeUpdate     (Game *, State *);
	void moveHomeSignal     (Game *, State *);
protected:
	static Task taskMoveToBattle;
	static Task taskMoveHome;

private:
	uint32_t m_hp_current;
	uint32_t m_hp_max;
	uint32_t m_min_attack;
	uint32_t m_max_attack;
	uint32_t m_defense;
	uint32_t m_evade;

	uint32_t m_hp_level;
	uint32_t m_attack_level;
	uint32_t m_defense_level;
	uint32_t m_evade_level;

	AttackController * m_attack_ctrl;
};

};

#endif
