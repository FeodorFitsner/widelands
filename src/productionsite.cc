/*
 * Copyright (C) 2002-2004 by Widelands Development Team
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

#include "editor_game_base.h"
#include "error.h"
#include "game.h"
#include "map.h"
#include "player.h"
#include "productionsite.h"
#include "profile.h"
#include "transport.h"
#include "tribe.h"
#include "util.h"
#include "wexception.h"
#include "worker.h"


static const size_t STATISTICS_VECTOR_LENGTH = 10;


/*
=============================

class Input

This class descripes, how many items of a certain
ware can be stored in a house.
This class will be extended to support ordering of
certain wares directly or releasing some wares
out of a building

=============================
*/
class Input {
public:
	Input(Ware_Descr* ware, int max) : m_ware(ware), m_max(max) {}
	~Input(void) {}

	inline void set_max(int n) { m_max = n; }
	inline int get_max(void) const { return m_max; }
	inline Ware_Descr* get_ware() const { return m_ware; }

private:
	Ware_Descr* m_ware;
	int m_max;
};


/*
==============================================================================

class ProductionProgram

==============================================================================
*/

struct ProductionAction {
	enum Type {
		actSleep,   // iparam1 = sleep time in milliseconds
		actWorker,  // sparam1 = worker program to run
		actConsume, // sparam1 = consume this ware, has to be an input
		actAnimate, // sparam1 = activate this animation until timeout
		actProduce, // sparem1 = ware to produce. the worker carries it outside
		actCheck,   // sparam1 = check if the given input ware is available
		actMine,    // iparam1 = mineXXX type to mine for
	};

	enum {
		mineCoal,
		mineGold,
		mineIron
	};

	Type        type;
	int         iparam1;
	int         iparam2;
	std::string	sparam1;
};

/*
class ProductionProgram
-----------------------
Holds a series of actions to perform for production.
*/
class ProductionProgram {
public:
	ProductionProgram(std::string name);

	std::string get_name() const { return m_name; }
	int get_size() const { return m_actions.size(); }
	const ProductionAction* get_action(int idx) const {
		assert(idx >= 0 && (uint)idx < m_actions.size());
		return &m_actions[idx];
	}

	void parse(std::string directory, Profile* prof, std::string name,
		ProductionSite_Descr* building, const EncodeData* encdata);

private:
	std::string                   m_name;
	std::vector<ProductionAction> m_actions;
};


/*
===============
ProductionProgram::ProductionProgram
===============
*/
ProductionProgram::ProductionProgram(std::string name)
{
	m_name = name;
}


/*
===============
ProductionProgram::parse

Parse a program. The building is parsed completly. hopefully
===============
*/
void ProductionProgram::parse(std::string directory, Profile* prof,
	std::string name, ProductionSite_Descr* building, const EncodeData* encdata)
{
	Section* sprogram = prof->get_safe_section(name.c_str());

	for(uint idx = 0; ; ++idx) {
		char buf[32];
		const char* string;
		std::vector<std::string> cmd;

		snprintf(buf, sizeof(buf), "%i", idx);
		string = sprogram->get_string(buf, 0);
		if (!string)
			break;

		split_string(string, &cmd, " \t\r\n");
		if (!cmd.size())
			continue;

		ProductionAction act;

		if (cmd[0] == "sleep") {
			char* endp;

			if (cmd.size() != 2)
				throw wexception("Line %i: Usage: sleep <time in ms>", idx);

			act.type = ProductionAction::actSleep;
			act.iparam1 = strtol(cmd[1].c_str(), &endp, 0);

			if (endp && *endp)
				throw wexception("Line %i: bad integer '%s'", idx, cmd[1].c_str());
		} else if (cmd[0] == "consume") {
			if(cmd.size() != 2)
				throw wexception("Line %i: Usage: consume <ware>", idx);


			Section* s=prof->get_safe_section("inputs");
			if(!s->get_string(cmd[1].c_str(), 0))
				throw wexception("Line %i: Ware %s is not in [inputs]\n", idx,
					cmd[1].c_str());

			act.type = ProductionAction::actConsume;
			act.sparam1 = cmd[1];
		}  else if (cmd[0] == "check") {
			if(cmd.size() != 2)
				throw wexception("Line %i: Usage: checking <ware>", idx);


			Section* s=prof->get_safe_section("inputs");
			if(!s->get_string(cmd[1].c_str(), 0))
				throw wexception("Line %i: Ware %s is not in [inputs]\n", idx,
					cmd[1].c_str());

			act.type = ProductionAction::actCheck;
			act.sparam1 = cmd[1];
		} else if (cmd[0] == "produce") {
			if(cmd.size() != 2)
				throw wexception("Line %i: Usage: produce <ware>", idx);

			if(!building->is_output(cmd[1]))
				throw wexception("Line %i: Ware %s is not in [outputs]\n", idx,
					cmd[1].c_str());

			act.type = ProductionAction::actProduce;
			act.sparam1 = cmd[1];
		} else if (cmd[0] == "worker") {
			if (cmd.size() != 2)
				throw wexception("Line %i: Usage: worker <program name>", idx);

			act.type = ProductionAction::actWorker;
			act.sparam1 = cmd[1];
		} else if (cmd[0] == "animation") {
			char* endp;

			if (cmd.size() != 3)
				throw wexception("Usage: animation <name> <time>");

			act.type = ProductionAction::actAnimate;

			// dynamically allocate animations here
			Section* s = prof->get_safe_section(cmd[1].c_str());
			act.iparam1 = g_anim.get(directory.c_str(), s, 0, encdata);

			if (cmd[1] == "idle")
				/* XXX */
				throw wexception("Idle animation is default, no calling senseful!");

			act.iparam2 = strtol(cmd[2].c_str(), &endp, 0);
			if (endp && *endp)
				throw wexception("Bad duration '%s'", cmd[2].c_str());

			if (act.iparam2 <= 0)
				throw wexception("animation duration must be positive");
		} else if (cmd[0] == "mine") {
			if (cmd.size() != 2)
				throw wexception("Usage: mine <resource>");

			act.type = ProductionAction::actMine;

			if (cmd[1] == "coal")
				act.iparam1 = ProductionAction::mineCoal;
			else if (cmd[1] == "gold")
				act.iparam1 = ProductionAction::mineGold;
			else if (cmd[1] == "iron")
				act.iparam1 = ProductionAction::mineIron;
			else
				throw wexception("Resource must be gold, coal or iron (not '%s')",
					cmd[1].c_str());
		} else
			throw wexception("Line %i: unknown command '%s'", idx, cmd[0].c_str());

		m_actions.push_back(act);
	}

	// Check for numbering problems
	if (sprogram->get_num_values() != m_actions.size())
		throw wexception("Line numbers appear to be wrong");
}


/*
==============================================================================

ProductionSite BUILDING

==============================================================================
*/

ProductionSite_Descr::ProductionSite_Descr(Tribe_Descr* tribe, const char* name)
	: Building_Descr(tribe, name)
{
}

ProductionSite_Descr::~ProductionSite_Descr()
{
	while(m_programs.size()) {
		delete m_programs.begin()->second;
		m_programs.erase(m_programs.begin());
	}
}


/*
===============
ProductionSite_Descr::parse

Parse the additional information necessary for production buildings
===============
*/
void ProductionSite_Descr::parse(const char* directory, Profile* prof,
	const EncodeData* encdata)
{
	Section* sglobal = prof->get_section("global");
	const char* string;

	Building_Descr::parse(directory, prof, encdata);

	// Get inputs and outputs
	while (sglobal->get_next_string("output", &string))
		m_output.insert(string);

	Section* s=prof->get_section("inputs");
	if (s) {
		// This house obviously requests wares and works on them
		Section::Value* val;
		while((val=s->get_next_val(0))) {
			int idx=get_tribe()->get_ware_index(val->get_name());
			if (idx == -1)
				throw wexception("Error in [inputs], ware %s is unknown!",
					val->get_name());

			Item_Ware_Descr* ware= get_tribe()->get_ware_descr(idx);

			Input input(ware,val->get_int());
			m_inputs.push_back(input);
		}
	}

	// Are we only a production site?
	// If not, we might not have a worker
	if (is_only_production_site())
		m_worker_name = sglobal->get_safe_string("worker");
	else
		m_worker_name = sglobal->get_string("worker", "");

	// Get programs
	while(sglobal->get_next_string("program", &string)) {
		ProductionProgram* program = 0;

		try
		{
			program = new ProductionProgram(string);
			program->parse(directory, prof, string, this, encdata);
			m_programs[program->get_name()] = program;
		}
		catch(std::exception& e)
		{
			delete program;
			throw wexception("Error in program %s: %s", string, e.what());
		}
	}
}


/*
===============
ProductionSite_Descr::get_program

Get the program of the given name.
===============
*/
const ProductionProgram* ProductionSite_Descr::get_program(std::string name)
	const
{
	ProgramMap::const_iterator it = m_programs.find(name);

	if (it == m_programs.end())
		throw wexception("%s has no program '%s'", get_name(), name.c_str());

	return it->second;
}

/*
===============
ProductionSite_Descr::create_object

Create a new building of this type
===============
*/
Building* ProductionSite_Descr::create_object()
{
	return new ProductionSite(this);
}


/*
==============================

IMPLEMENTATION

==============================
*/

/*
===============
ProductionSite::ProductionSite
===============
*/
ProductionSite::ProductionSite(ProductionSite_Descr* descr)
	: Building(descr), m_statistics(STATISTICS_VECTOR_LENGTH, false)
{
	m_worker = 0;
	m_worker_request = 0;

	m_fetchfromflag = 0;

	m_program = 0;
	m_program_ip = 0;
	m_program_phase = 0;
	m_program_timer = false;
	m_program_time = 0;
	m_statistics_changed = true;
}


/*
===============
ProductionSite::~ProductionSite
===============
*/
ProductionSite::~ProductionSite()
{
}

/*
===============
ProductionSite::get_statistic_string

Display whether we're occupied.
===============
*/
std::string ProductionSite::get_statistics_string()
{
	if (!m_worker)
		return "(not occupied)";
	if (m_statistics_changed)
		calc_statistics();
	return m_statistics_buf;
}

/*
===============
ProductionSite::calc_statistic

Calculate statistic.
===============
*/
void ProductionSite::calc_statistics()
{
	uint pos;
	uint ok = 0;
	uint lastOk = 0;

	for(pos = 0; pos < STATISTICS_VECTOR_LENGTH; ++pos) {
		if (m_statistics[pos]) {
			ok++;
			if (pos >= STATISTICS_VECTOR_LENGTH / 2)
				lastOk++;
		}
	}
	double percOk = (ok * 100) / STATISTICS_VECTOR_LENGTH;
	double lastPercOk = (lastOk * 100) / (STATISTICS_VECTOR_LENGTH / 2);

	const char* trendBuf;
	if (lastPercOk > percOk)
		trendBuf = "UP";
	else if (lastPercOk < percOk)
		trendBuf = "DOWN";
	else
		trendBuf = "=";

	if (percOk > 0 && percOk < 100)
		snprintf(m_statistics_buf, sizeof(m_statistics_buf), "%.0f%% %s",
			percOk, trendBuf);
	else
		snprintf(m_statistics_buf, sizeof(m_statistics_buf), "%.0f%%", percOk);
	molog("stat: lastOk: %.0f%% percOk: %.0f%% trend: %s\n",
		lastPercOk, percOk, trendBuf);
	m_statistics_changed = false;
}


/*
===============
ProductionSite::add_statistic_value

Add a value to statistic vector.
===============
*/
void ProductionSite::add_statistics_value(bool val)
{
	m_statistics_changed = true;
	m_statistics.erase(m_statistics.begin(),m_statistics.begin() + 1);
	m_statistics.push_back(val);
}

/*
===============
ProductionSite::init

Initialize the production site.
===============
*/
void ProductionSite::init(Editor_Game_Base* g)
{
	Building::init(g);

	if (g->is_game()) {
		// Request worker
		if (!m_worker)
			request_worker((Game*)g);

		// Init input ware queues
		const std::vector<Input>* inputs =
			((ProductionSite_Descr*)get_descr())->get_inputs();

		for(uint i = 0; i < inputs->size(); i++) {
			WaresQueue* wq = new WaresQueue(this);

			m_input_queues.push_back(wq);
			//wq->set_callback(&ConstructionSite::wares_queue_callback, this);
			wq->init((Game*)g,
				g->get_safe_ware_id((*inputs)[i].get_ware()->get_name()),
				(*inputs)[i].get_max());
		}
	}
}

/*
===============
ProductionSite::set_economy

Change the economy for the wares queues.
Note that the workers are dealt with in the PlayerImmovable code.
===============
*/
void ProductionSite::set_economy(Economy* e)
{
	Economy* old = get_economy();
	uint i;

	if (old) {
		for(i = 0; i < m_input_queues.size(); i++)
			m_input_queues[i]->remove_from_economy(old);
	}

	Building::set_economy(e);
	if (m_worker_request)
		m_worker_request->set_economy(e);

	if (e) {
		for(i = 0; i < m_input_queues.size(); i++)
			m_input_queues[i]->add_to_economy(e);
	}
}

/*
===============
ProductionSite::cleanup

Cleanup after a production site is removed
===============
*/
void ProductionSite::cleanup(Editor_Game_Base* g)
{
	// Release worker
	if (m_worker_request) {
		delete m_worker_request;
		m_worker_request = 0;
	}

	if (m_worker) {
		Worker* w = m_worker;

		m_worker = 0;
		w->set_location(0);
	}


	// Cleanup the wares queues
	for(uint i = 0; i < m_input_queues.size(); i++) {
		m_input_queues[i]->cleanup((Game*)g);
		delete m_input_queues[i];
	}
	m_input_queues.clear();


	Building::cleanup(g);
}


/*
===============
ProductionSite::remove_worker

Intercept remove_worker() calls to unassign our worker, if necessary.
===============
*/
void ProductionSite::remove_worker(Worker* w)
{
	if (m_worker == w) {
		m_worker = 0;
		request_worker((Game*)get_owner()->get_game());
	}

	Building::remove_worker(w);
}


/*
===============
ProductionSite::request_worker

Issue the worker request
===============
*/
void ProductionSite::request_worker(Game* g)
{
	assert(!m_worker);
	assert(!m_worker_request);
	// no worker for this house. it's not a productionsite only
	if (!get_descr()->get_worker_name().size()) return;

	int wareid = g->get_safe_ware_id(get_descr()->get_worker_name().c_str());

	m_worker_request =
		new Request(this, wareid, &ProductionSite::request_worker_callback, this);
}


/*
===============
ProductionSite::request_worker_callback [static]

Called when our worker arrives.
===============
*/
void ProductionSite::request_worker_callback(Game* g, Request* rq, int ware,
	Worker* w, void* data)
{
	ProductionSite* psite = (ProductionSite*)data;

	assert(w);
	assert(w->get_location(g) == psite);
	assert(rq == psite->m_worker_request);

	psite->m_worker = w;

	delete rq;
	psite->m_worker_request = 0;

	w->start_task_buildingwork(g);
}


/*
===============
ProductionSite::act

Advance the program state if applicable.
===============
*/
void ProductionSite::act(Game* g, uint data)
{
	if (m_program_timer && (int)(g->get_gametime() - m_program_time) >= 0) {
		if (!m_program)
		{
			m_program = get_descr()->get_program("work");
			m_program_ip = 0;
			m_program_phase = 0;
		}

		if (m_program_ip >= m_program->get_size()) {
			program_end(g, true);
			return;
		}

		m_program_timer = false;

		if (m_anim != get_descr()->get_idle_anim()) {
			// Restart idle animation, which is the default
			start_animation(g, get_descr()->get_idle_anim());
		}

		program_act(g);
	}
	Building::act(g, data);
}


/*
===============
ProductionSite::program_act

Perform the current program action.

Pre: The program is running and in a valid state.
Post: (Potentially indirect) scheduling for the next step has been done.
===============
*/
void ProductionSite::program_act(Game* g)
{
	const ProductionAction* action = m_program->get_action(m_program_ip);

	molog("PSITE: program %s#%i\n", m_program->get_name().c_str(), m_program_ip);

	switch(action->type) {
		case ProductionAction::actSleep:
			molog("  Sleep(%i)\n", action->iparam1);

			program_step();
			m_program_timer = true;
			m_program_time = schedule_act(g, action->iparam1);
			return;

		case ProductionAction::actAnimate:
			molog("  Animate(%i,%i)\n", action->iparam1, action->iparam2);

			start_animation(g, action->iparam1);

			program_step();
			m_program_timer = true;
			m_program_time = schedule_act(g, action->iparam2);
			return;

		case ProductionAction::actWorker:
			molog("  Worker(%s)\n", action->sparam1.c_str());

			m_worker->update_task_buildingwork(g);
			return;

		case ProductionAction::actConsume:
			molog("  Consume(%s)\n", action->sparam1.c_str());
			for(uint i=0; i<get_descr()->get_inputs()->size(); i++) {
				if (strcmp((*get_descr()->get_inputs())[i].get_ware()->get_name(),
					action->sparam1.c_str()) == 0) {
					WaresQueue* wq=m_input_queues[i];
					if(wq->get_filled())
					{
						wq->set_filled(wq->get_filled()-1);
						wq->update(g);
					}
					else
					{
						molog("   Consume failed, program restart\n");
						program_end(g, false);
						return;
					}
					break;
				}
			}
			molog("  Consume done!\n");
			program_step();
			m_program_timer = true;
			m_program_time = schedule_act(g, 10);
			return;

		case ProductionAction::actCheck:
			molog("  Checking(%s)\n", action->sparam1.c_str());

			for(uint i=0; i<get_descr()->get_inputs()->size(); i++) {
				if (strcmp((*get_descr()->get_inputs())[i].get_ware()->get_name(),
					action->sparam1.c_str()) == 0) {
					WaresQueue* wq = m_input_queues[i];
					if(wq->get_filled())
					{
						// okay, do nothing
						molog("    okay\n");
					}
					else
					{
						molog("   Checking failed, program restart\n");
						program_end(g, false);
						return;
					}
					break;
				}
			}

			molog("  Check done!\n");

			program_step();
			m_program_timer = true;
			m_program_time = schedule_act(g, 10);
			return;

		case ProductionAction::actProduce:
		{
			molog("  Produce(%s)\n", action->sparam1.c_str());

			int wareid = g->get_safe_ware_id(action->sparam1.c_str());

			WareInstance* item = new WareInstance(wareid);
			item->init(g);
			m_worker->set_carried_item(g,item);

			// get the worker to drop the item off
			// get_building_work() will advance the program
			m_worker->update_task_buildingwork(g);
			return;
		}

		case ProductionAction::actMine:
		{
			Map* map = g->get_map();
			MapRegion mr;
			uchar res;

			molog("  Mine %u\n", action->iparam1);

			switch(action->iparam1) {
			case ProductionAction::mineCoal:
				res = Resource_Coal;
				break;
			case ProductionAction::mineGold:
				res = Resource_Gold;
				break;
			case ProductionAction::mineIron:
				res = Resource_Iron;
				break;
			default:
				throw wexception("ProductionAction::actMine: bad iparam = %u",
					action->iparam1);
			}

			// Select one of the fields randomly
			uint totalres = 0;
			uint totalchance = 0;
			int pick;
			Field* f;

			mr.init(map, get_position(), 2);

			while((f = mr.next())) {
				uchar fres = f->get_resources();
				uint amount;

				if ((fres & Resource_TypeMask) != res)
					amount = 0;
				else
				{
					amount = fres & Resource_AmountMask;

					// In the future, we might want to support amount = 0 for
					// fields that can produce an infinite amount of resources.
					if (amount == 0)
						f->set_resources(0);
				}

				totalres += amount;
				totalchance += 8 * amount;

				// Add penalty for fields that are running out
				if (amount == 0)
					// we already know it's completely empty, so punish is less
					totalchance += 1;
				else if (amount <= 2)
					totalchance += 6;
				else if (amount <= 4)
					totalchance += 4;
				else if (amount <= 6)
					totalchance += 2;
			}

			if (totalres == 0) {
				molog("  Run out of resources\n");
				program_end(g, false);
				return;
			}

			// Second pass through fields
			pick = g->logic_rand() % totalchance;

			mr.init(map, get_position(), 2);

			while((f = mr.next())) {
				uchar fres = f->get_resources();
				uint amount;

				if ((fres & Resource_TypeMask) != res)
					amount = 0;
				else
					amount = fres & Resource_AmountMask;

				pick -= 8*amount;
				if (pick < 0) {
					assert(amount > 0);

					amount--;

					if (!amount)
						fres = 0;
					else
						fres = res | amount;

					f->set_resources(fres);
					break;
				}
			}

			if (pick >= 0) {
				molog("  Not successful this time\n");
				program_end(g, false);
				return;
			}

			molog("  Mined one item\n");

			program_step();
			m_program_timer = true;
			m_program_time = schedule_act(g, 10);
			return;
		}
	}
}


/*
===============
ProductionSite::fetch_from_flag

Remember that we need to fetch an item from the flag.
===============
*/
bool ProductionSite::fetch_from_flag(Game* g)
{
	m_fetchfromflag++;

	if (m_worker)
		m_worker->update_task_buildingwork(g);

	return true;
}


/*
===============
ProductionSite::get_building_work

There's currently nothing to do for the worker.
Note: we assume that the worker is inside the building when this is called.
===============
*/
bool ProductionSite::get_building_work(Game* g, Worker* w, bool success)
{
	assert(w == m_worker);

	// If unsuccessful: Check if we need to abort current program
	if (!success && m_program && m_program_ip < m_program->get_size())
	{
		const ProductionAction* action = m_program->get_action(m_program_ip);

		if (action->type == ProductionAction::actWorker)
			program_end(g, false);
	}

	// Default actions first
	WareInstance* item = w->fetch_carried_item(g);

	if (item) {
		if (!get_descr()->is_output(item->get_ware_descr()->get_name()))
			molog("PSITE: WARNING: carried item %s is not an output item\n",
					item->get_ware_descr()->get_name());

		molog("ProductionSite::get_building_work: start dropoff\n");

		w->start_task_dropoff(g, item);
		return true;
	}

	if (m_fetchfromflag) {
		m_fetchfromflag--;
		w->start_task_fetchfromflag(g);
		return true;
	}

	// Start program if we haven't already done so
	if (!m_program)
	{
		m_program_timer = true;
		m_program_time = schedule_act(g, 10);
	}
	else if (m_program_ip < m_program->get_size())
	{
		const ProductionAction* action = m_program->get_action(m_program_ip);

		if (action->type == ProductionAction::actWorker) {
			if (m_program_phase == 0)
			{
				w->start_task_program(g, action->sparam1);
				m_program_phase++;
				return true;
			}
			else
			{
				program_step();
				m_program_timer = true;
				m_program_time = schedule_act(g, 10);
			}
		} else if (action->type == ProductionAction::actProduce) {
			// Worker returned home after dropping item
			program_step();
			m_program_timer = true;
			m_program_time = schedule_act(g, 10);
		}
	}

	return false;
}


/*
===============
ProductionSite::program_step

Advance the program to the next step, but does not schedule anything.
===============
*/
void ProductionSite::program_step()
{
	m_program_ip++;
	m_program_phase = 0;
}


/*
===============
ProductionSite::program_end

Ends the current program now and updates the productivity statistics.

Pre: Any program is running.
Post: No program is running, acting is scheduled.
===============
*/
void ProductionSite::program_end(Game* g, bool success)
{
	molog("ProductionSite: program ends (%s)\n", success ? "success" : "failure");

	m_program = 0;
	m_program_ip = 0;
	m_program_phase = 0;
	m_program_timer = true;
	m_program_time = schedule_act(g, 10);

	add_statistics_value(success);
}
