/*
 * Copyright (C) 2002-2004 by the Widelands Development Team
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

#ifndef PRODUCTIONSITE_H
#define PRODUCTIONSITE_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "building.h"
#include "types.h"

class Input;
class ProductionProgram;
class Request;
class WaresQueue;


/*
ProductionSite
--------------
Every building that is part of the economics system is a production site.

A production site has a worker.
A production site can have one (or more) output wares types (in theory it
  should be possible to burn wares for some virtual result such as "mana", or
  maybe even just for the fun of it, although that's not planned).
A production site can have one (or more) input wares types. Every input
  wares type has an associated store.
*/

class ProductionSite_Descr : public Building_Descr {
	typedef std::map<std::string, ProductionProgram*> ProgramMap;

public:
	ProductionSite_Descr(Tribe_Descr* tribe, const char* name);
	virtual ~ProductionSite_Descr();

	virtual void parse(const char* directory, Profile* prof,
		const EncodeData* encdata);
	virtual Building* create_object();

	std::string get_worker_name() const { return m_worker_name; }
	bool is_output(std::string name) const {
		return m_output.find(name) != m_output.end();
	}
	const std::set<std::string>* get_outputs() const { return &m_output; }
	const std::vector<Input>* get_inputs() const { return &m_inputs; }
	const ProductionProgram* get_program(std::string name) const;

	virtual bool is_only_production_site(void) { return true; }

private:
	std::string           m_worker_name; // name of worker type
	std::vector<Input>    m_inputs;
	std::set<std::string> m_output;      // output wares type names
	ProgramMap            m_programs;
};

class ProductionSite : public Building {
	MO_DESCR(ProductionSite_Descr);

public:
	ProductionSite(ProductionSite_Descr* descr);
	virtual ~ProductionSite();

	virtual std::string get_statistics_string();

	virtual void init(Editor_Game_Base* g);
	virtual void cleanup(Editor_Game_Base* g);
	virtual void act(Game* g, uint data);

	virtual void remove_worker(Worker* w);

	virtual bool fetch_from_flag(Game* g);
	virtual bool get_building_work(Game* g, Worker* w, bool success);

	virtual void set_economy(Economy* e);

	inline std::vector<WaresQueue*>* get_warequeues(void) {
		return &m_input_queues;
	}

protected:
	virtual UIWindow* create_options_window(Interactive_Player* plr,
		UIWindow** registry);

private:
	void request_worker(Game* g);
	static void request_worker_callback(Game* g, Request* rq, int ware,
		Worker* w, void* data);

	void program_act(Game* g);
	void program_step();
	void program_end(Game* g, bool success);
	void add_statistics_value(bool val);

	void calc_statistics();

private:
	Request* m_worker_request;
	Worker*  m_worker;

	int m_fetchfromflag; // # of items to fetch from flag

	const ProductionProgram* m_program;       // currently running program
	int                      m_program_ip;    // instruction pointer
	int                      m_program_phase; // micro-step index
																						// (instruction dependent)
	bool                     m_program_timer; // execute next instruction based
																						// on pointer
	int                      m_program_time;	// timer time

	std::vector<WaresQueue*> m_input_queues; //  input queues for all inputs
	std::vector<bool>        m_statistics;
	bool                     m_statistics_changed;
	char                     m_statistics_buf[40];
};

#endif
