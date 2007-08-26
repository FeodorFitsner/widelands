/*
 * Copyright (C) 2002-2004, 2006-2007 by the Widelands Development Team
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

#ifndef __S__CRITTER_BOB_H
#define __S__CRITTER_BOB_H

#include "bob.h"

class Critter_BobAction;
class Critter_BobProgram;

//
// Description
//
struct Critter_Bob_Descr : public Bob::Descr {
	Critter_Bob_Descr
		(const Tribe_Descr * const, const std::string & critter_bob_name);
	virtual ~Critter_Bob_Descr();

      virtual void parse(const char *directory, Profile *prof, const EncodeData *encdata);
	Bob * create_object() const;

	bool is_swimming() const throw () {return m_swimming;}
	const DirAnimations & get_walk_anims() const throw () {return m_walk_anims;}
	const std::string & descname() const throw () {return m_descname;}
	__attribute__ ((deprecated)) const char * get_descname() const throw () {return descname().c_str();}

      const Critter_BobProgram* get_program(std::string programname) const;

   private:
	std::string   m_descname;
	DirAnimations m_walk_anims;
	bool          m_swimming;
	typedef std::map<std::string, Critter_BobProgram *> ProgramMap;
      ProgramMap     m_programs;
};

class Critter_Bob : public Bob {
   friend class Widelands_Map_Bobdata_Data_Packet;
	friend class Critter_BobProgram;

   MO_DESCR(Critter_Bob_Descr);

public:
	Critter_Bob(const Critter_Bob_Descr &);
	virtual ~Critter_Bob();

	virtual Bob::Type get_bob_type() const throw () {return Bob::CRITTER;}
	uint get_movecaps() const throw ();

	virtual void init_auto_task(Game* g);

	void start_task_program(const std::string & name);
	const std::string & descname() const throw () {return descr().descname();}
	__attribute__ ((deprecated)) const char * get_descname() const throw ()  {return descname().c_str();}

private:
	void roam_update(Game* g, State* state);
	void roam_signal(Game* g, State* state);

   void program_update(Game* g, State* state);
	void program_signal(Game* g, State* state);

private:
   bool run_remove(Game* g, State* state, const Critter_BobAction* act);

private:
	static Task taskRoam;
	static Task taskProgram;
};

#endif // __S__CRITTER_BOB_H
