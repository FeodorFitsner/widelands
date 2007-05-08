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

#include "productionsite.h"

#include "editor_game_base.h"
#include "error.h"
#include "game.h"
#include "helper.h"
#include "i18n.h"
#include "interactive_player.h"
#include "map.h"
#include "player.h"
#include "production_program.h"
#include "profile.h"
#include "soldier.h"
#include "transport.h"
#include "tribe.h"
#include "wexception.h"
#include "worker.h"
#include "warelist.h"
#include "world.h"
#include "sound_handler.h"

static const size_t STATISTICS_VECTOR_LENGTH = 10;



/*
==============================================================================

ProductionSite BUILDING

==============================================================================
*/

ProductionSite_Descr::ProductionSite_Descr
(const Tribe_Descr & tribe_descr, const std::string & productionsite_name)
: Building_Descr(tribe_descr, productionsite_name)
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

   // Stopabple defaults to true for Production sites
   m_stopable=true;

	Building_Descr::parse(directory, prof, encdata);

	// Get inputs and outputs
	while (sglobal->get_next_string("output", &string))
		m_output.insert(string);

	Section* s=prof->get_section("inputs");
	if (s) {
		// This house obviously requests wares and works on them
		Section::Value* val;
		while((val=s->get_next_val(0))) {
			int idx=tribe().get_ware_index(val->get_name());
			if (idx == -1)
				throw wexception("Error in [inputs], ware %s is unknown!",
					val->get_name());

			Item_Ware_Descr* ware= tribe().get_ware_descr(idx);

			Input input(ware,val->get_int());
			m_inputs.push_back(input);
		}
	}

	// Are we only a production site?
	// If not, we might not have a worker
   std::string workerstr="";
	if (is_only_production_site())
		workerstr = sglobal->get_safe_string("worker");
	else
		workerstr = sglobal->get_string("worker", "");

   std::vector<std::string> workers;
	split_string(workerstr, workers, ",");
   uint i;
   std::vector<std::string> amounts;
   for(i=0; i<workers.size(); i++) {
      amounts.resize(0);
      remove_spaces(&workers[i]);
		split_string(workers[i], amounts, "*");
      uint j;
      for(j=0; j<amounts.size(); j++)
         remove_spaces(&amounts[j]);

      int amount=1;
		if (amounts.size() == 2) {
         char *endp;
			amount = strtol(amounts[1].c_str(), &endp, 0);
			if (endp && *endp)
				throw wexception("Bad amount in worker line: %s", amounts[1].c_str());
      }
      Worker_Info m= { amounts[0], amount };
      m_workers.push_back(m);
   }

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
const ProductionProgram * ProductionSite_Descr::get_program
(const std::string & program_name) const
{
	const ProgramMap::const_iterator it = m_programs.find(program_name);
	if (it == m_programs.end()) throw wexception
		("%s has no program '%s'", name().c_str(), program_name.c_str());
	return it->second;
}

/*
===============
ProductionSite_Descr::create_object

Create a new building of this type
===============
*/
Building* ProductionSite_Descr::create_object() const
{return new ProductionSite(*this);}


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
ProductionSite::ProductionSite(const ProductionSite_Descr & ps_descr) :
Building            (ps_descr),
m_fetchfromflag     (0),
m_program_timer     (false),
m_program_time      (0),
m_post_timer        (50),
m_statistics        (STATISTICS_VECTOR_LENGTH, false),
m_statistics_changed(true),
m_last_stat_percent (0)
{}


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
   if (!m_workers.size())
		return _("(not occupied)");
	else if (m_worker_requests.size()) {
      char buf[1000];
      sprintf(buf, "Waiting for %i workers!", (int)m_worker_requests.size());
      return buf;
   }

	if (m_statistics_changed)
		calc_statistics();

   if (m_stop)
		return _("(stopped)");
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

	const std::string trend =
		lastPercOk > percOk ? _("UP") : lastPercOk < percOk ? _("DOWN") : "=";

	if (0 < percOk and percOk < 100) snprintf
		(m_statistics_buf, sizeof(m_statistics_buf),
		 "%.0f%% %s", percOk, trend.c_str());
	else snprintf(m_statistics_buf, sizeof(m_statistics_buf), "%.0f%%", percOk);

   m_last_stat_percent = (char)percOk;

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

	if (Game * const game = dynamic_cast<Game * const>(g)) {
		// Request worker
		if (!m_workers.size()) {
			const std::vector<ProductionSite_Descr::Worker_Info>* info=descr().get_workers();
         uint i;
         int j;
         for(i=0; i<info->size(); i++)
            for(j=0; j< ((*info)[i]).how_many; j++)
					request_worker(((*info)[i]).name.c_str());
      }

		// Init input ware queues
		const std::vector<Input>* inputs = descr().get_inputs();

		for(uint i = 0; i < inputs->size(); i++) {
			WaresQueue* wq = new WaresQueue(this);

			m_input_queues.push_back(wq);
			//wq->set_callback(&ConstructionSite::wares_queue_callback, this);
			wq->init
				(owner().tribe().get_safe_ware_index
				 ((*inputs)[i].ware_descr().name().c_str()),
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
	if (Economy * const old = get_economy()) {
		for(uint i = 0; i < m_input_queues.size(); i++)
			m_input_queues[i]->remove_from_economy(old);
	}

	Building::set_economy(e);
	{
		std::vector<Request *>::const_iterator m_worker_requests_end =
			m_worker_requests.end();
		for
			(std::vector<Request *>::const_iterator it = m_worker_requests.begin();
			 it != m_worker_requests_end;
			 ++it)
			if (*it) (*it)->set_economy(e);
	}

	if (e) {
		std::vector<WaresQueue *>::const_iterator m_input_queues_end =
			m_input_queues.end();
		for
			(std::vector<WaresQueue *>::const_iterator it = m_input_queues.begin();
			 it != m_input_queues_end;
			 ++it)
			(*it)->add_to_economy(e);
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
   if (m_worker_requests.size()) {
      uint i=0;
      for(i=0; i<m_worker_requests.size(); i++) {
         delete m_worker_requests[i];
         m_worker_requests[i]=0;
      }
      m_workers.resize(0);
   }

   if (m_workers.size()) {
      uint i=0;
      for(i=0; i<m_workers.size(); i++) {
         Worker* w = m_workers[i];

         m_workers[i] = 0;
			if (g->objects().object_still_available(w)) w->set_location(0);
      }
      m_workers.resize(0);
   }

   // Cleanup the wares queues
   for(uint i = 0; i < m_input_queues.size(); i++) {
      m_input_queues[i]->cleanup();
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
   uint i=0;
   for(i=0; i<m_workers.size(); i++) {
      if (m_workers[i] == w) {
         m_workers[i] = 0;
			request_worker(w->name().c_str());
         m_workers.erase(m_workers.begin() + i);
         break;
      }
   }

	Building::remove_worker(w);
}


/*
===============
ProductionSite::request_worker

Issue the worker requests
===============
*/
void ProductionSite::request_worker(const char * const worker_name) {
	assert(worker_name);

	int wareid = owner().tribe().get_safe_worker_index(worker_name);

   m_worker_requests.push_back(new Request(this, wareid, &ProductionSite::request_worker_callback, this, Request::WORKER));
}


/*
===============
ProductionSite::request_worker_callback [static]

Called when our worker arrives.
===============
*/
void ProductionSite::request_worker_callback
(Game* g, Request* rq, int, Worker* w, void* data)
{
	ProductionSite* psite = (ProductionSite*)data;

	assert(w);
	assert(w->get_location(g) == psite);

	{
		std::vector<Request *> & worker_requests = psite->m_worker_requests;
		std::vector<Request *>::iterator it = worker_requests.begin();
		//  Assume that rq must be in worker_requests.
		assert(worker_requests.size());
		while (*it != rq) {++it; assert(it != worker_requests.end());}
		worker_requests.erase(it);
	}

	psite->m_workers.push_back(w);

	delete rq;

	Worker * const main_worker = psite->m_workers[0];
	const bool can_start_working = psite->can_start_working();
	const bool w_is_not_main_worker = w != main_worker;
	if (not can_start_working or w_is_not_main_worker)
      w->start_task_idle(g, 0, -1); // bind the worker into this house, hide him on the map

	if (can_start_working) {
		if (w_is_not_main_worker) main_worker->send_signal(g, "wakeup");
      psite->m_workers[0]->start_task_buildingwork();
   }
}


/*
===============
ProductionSite::act

Advance the program state if applicable.
===============
*/
void ProductionSite::act(Game* g, uint data)
{
	Building::act(g, data);

	if (m_program_timer && (int)(g->get_gametime() - m_program_time) >= 0) {
		m_program_timer = false;

		if (!m_program.size())
		{
			program_start(g, "work");
			return;
		}

		State* state = get_current_program();

		assert(state);

		if (state->ip >= state->program->get_size()) {
			program_end(g, true);
			return;
		}

		if (m_anim != descr().get_animation("idle")) {
			// Restart idle animation, which is the default
			start_animation(g, descr().get_animation("idle"));
		}

		program_act(g);
	}
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
	State* state = get_current_program();
	const ProductionAction* action = state->program->get_action(state->ip);

	molog("PSITE: program %s#%i\n", state->program->get_name().c_str(), state->ip);

	if (m_stop) {
		program_end(g,false);
		m_program_timer = true;
		m_program_time = schedule_act(g,20000);
		return;
	}
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

			m_workers[0]->update_task_buildingwork(g);  // Always main worker is doing stuff
			return;

	case ProductionAction::actConsume: {
            std::vector<std::string> wares;
		split_string(action->sparam1, wares, ",");

            uint j=0;
            bool consumed=false;
            for(j=0; j<wares.size(); j++) {
               molog("  Consuming(%s)\n", wares[j].c_str());
					const std::vector<Input> & inputs = *descr().get_inputs();
					const std::vector<Input>::size_type inputs_size =
						inputs.size();
					for
						(std::vector<std::string>::size_type i = 0;
						 i < inputs_size;
						 ++i)
						if (inputs[i].ware_descr().name() == wares[j]) {
                     WaresQueue* wq = m_input_queues[i];
							if
								(static_cast<const int>(wq->get_filled())
								 >=
								 action->iparam1)
                     {
                        // Okay
                        wq->set_filled(wq->get_filled()-action->iparam1);
								wq->update();
                        consumed=true;
                        break;
                     }
                  }
					if (consumed) break;
            }
				if (not consumed) {
               molog("   Consuming failed, program restart\n");
               program_end(g, false);
               return;
            }
            molog("  Consume done!\n");

            program_step();
            m_program_timer = true;
            m_program_time = schedule_act(g, 10);
            return;
         }

	case ProductionAction::actCheck: {
            std::vector<std::string> wares;
		split_string(action->sparam1, wares, ",");

            uint j=0;
            bool found=false;
            for(j=0; j<wares.size(); j++) {
               molog("  Checking(%s)\n", wares[j].c_str());
					const std::vector<Input> & inputs = *descr().get_inputs();
					const std::vector<Input>::size_type inputs_size =
						inputs.size();
					for
						(std::vector<std::string>::size_type i = 0;
						 i < inputs_size;
						 ++i)
						if (inputs[i].ware_descr().name() == wares[j]) {
                     WaresQueue* wq = m_input_queues[i];
							if
								(static_cast<const int>(wq->get_filled())
								 >=
								 action->iparam1)
                     {
                        // okay, do nothing
                        molog("    okay\n");
                        found=true;
                        break;
                     }
                  }
					if (found) break;
            }
				if (not found) {
               molog("   Checking failed, program restart\n");
               program_end(g, false);
               return;
            }
            molog("  Check done!\n");

            program_step();
            m_program_timer = true;
            m_program_time = schedule_act(g, 10);
            return;
         }

      case ProductionAction::actProduce:
		{
			molog("  Produce(%s)\n", action->sparam1.c_str());

			int wareid = get_owner()->tribe().get_safe_ware_index(action->sparam1.c_str());

			WareInstance* item = new WareInstance(wareid,  get_owner()->tribe().get_ware_descr(wareid));
			item->init(g);

         // For statistics, inform the user that a ware was produced
         // Ware statistics are only cached for the interactive user
         // since other tribes would have other types of wares
         if(g->get_ipl()->get_player_number()==get_owner()->get_player_number())
            g->get_ipl()->ware_produced(wareid);

			m_workers[0]->set_carried_item(g,item);

			// get the worker to drop the item off
			// get_building_work() will advance the program
			m_workers[0]->update_task_buildingwork(g);
			return;
		}

		case ProductionAction::actMine: {
         Map & map = *g->get_map();
         uchar res;

         molog("  Mine '%s'", action->sparam1.c_str());

         res = map.get_world()->get_resource(action->sparam1.c_str());
         if(static_cast<signed char>(res)==-1)
            throw wexception("ProductionAction::actMine: Should mine resource %s, which doesn't exist in world. Tribe is not compatible"
                  " with world!!\n",  action->sparam1.c_str());

         // Select one of the fields randomly
         uint totalres = 0;
         uint totalchance = 0;
         uint totalstart = 0;
         int pick;

			{
				MapRegion<Area<FCoords> > mr
					(map,
					 Area<FCoords>
					 (map.get_fcoords(get_position()), action->iparam1));
				do {
					uchar fres        = mr.location().field->get_resources();
					uint amount       = mr.location().field->get_resources_amount();
					uint start_amount =
						mr.location().field->get_starting_res_amount();

			      // In the future, we might want to support amount = 0 for
			      // fields that can produce an infinite amount of resources.
			      // Rather -1 or something similar. not 0
			      if (fres != res) {
				      amount = 0;
				      start_amount=0;
			      }

			      totalres += amount;
			      totalstart += start_amount;
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
				} while (mr.advance(map));
	      }

         // how much is digged
         int digged_percentage=100;
			if (totalstart) digged_percentage = 100 - totalres * 100 / totalstart;
			if (not totalres) digged_percentage = 100;
         molog("  Mine has already digged %i percent (%i/%i)!\n", digged_percentage, totalres, totalstart);

			if (digged_percentage<action->iparam2) {
            // Mine can produce normally
            if (totalres == 0) {
               molog("  Run out of resources\n");
               program_end(g, false);
               return;
            }

            // Second pass through fields
            pick = g->logic_rand() % totalchance;

				{
					MapRegion<Area<FCoords> > mr
						(map,
						 Area<FCoords>
						 (map.get_fcoords(get_position()), action->iparam1));
					do {
						uchar fres  = mr.location().field->get_resources();
						uint amount = mr.location().field->get_resources_amount();

			         if (fres != res)
				         amount = 0;

			         pick -= 8*amount;
						if (pick < 0) {
				         assert(amount > 0);

				         --amount;

							mr.location().field->set_resources(res,amount);
				         break;
			         }
					} while (mr.advance(map));
	         }

            if (pick >= 0) {
               molog("  Not successful this time\n");
               program_end(g, false);
               return;
            }

            molog("  Mined one item\n");
         } else {
            // Mine has reached it's limits, still try to produce something
            // but independant of sourrunding resources. Do not decrease resources further
				assert(action->iparam3 >= 0);
				if
					(g->logic_rand() % 100
					 >=
					 static_cast<const uint>(action->iparam3))
				{
               // Not successfull
               molog("  Not successful this time in fallback programm\n");
               program_end(g, false);
               return;
            }
         }

         // Done successfull
         program_step();
         m_program_timer = true;
         m_program_time = schedule_act(g, 10);
         return;
      }

		case ProductionAction::actCall:
			molog("  Call %s\n", action->sparam1.c_str());

			program_step();
			program_start(g, action->sparam1);
			return;

		case ProductionAction::actSet:
			molog("  Set %08X, unset %08X\n", action->iparam1, action->iparam2);

			state->flags = (state->flags | action->iparam1) & ~action->iparam2;

			program_step();
			m_program_timer = true;
			m_program_time = schedule_act(g, 10);
			return;
		case ProductionAction::actCheckSoldier:
			{
				const std::vector<Soldier *> & soldiers = get_soldiers();
				const std::vector<Soldier *>::const_iterator soldiers_end =
					soldiers.end();

				molog ("  Checking soldier (%s) level %d)\n",
					action->sparam1.c_str(), action->iparam1);

				for
					(std::vector<Soldier *>::const_iterator it = soldiers.begin();
					 ;
					 ++it)
				{
					if (it == soldiers_end) {
						molog("   Checking failed, program restart\n");
						program_end(g, false);
						return;
					}
					if (action->sparam1 == "hp") {
						if
							((*it)->get_hp_level()
							 ==
							 static_cast<const uint>(action->iparam1))
							break;
					} else if (action->sparam1 == "attack") {
						if
							((*it)->get_attack_level()
							 ==
							 static_cast<const uint>(action->iparam1))
							break;
					} else if (action->sparam1 == "defense") {
						if
							((*it)->get_defense_level()
							 ==
							 static_cast<const uint>(action->iparam1))
							break;
					} else if (action->sparam1 == "evade") {
						if
							((*it)->get_evade_level()
							 ==
							 static_cast<const uint>(action->iparam1))
							break;
					}
				}
				molog("    okay\n"); // okay, do nothing

	         molog("  Check done!\n");

				program_step();
				m_program_timer = true;
				m_program_time = schedule_act(g, 10);
				return;
			}
		case ProductionAction::actTrain:
			{
				const std::vector<Soldier *> & soldiers = get_soldiers();
				const std::vector<Soldier *>::const_iterator soldiers_end =
					soldiers.end();
				std::vector<Soldier *>::const_iterator it = soldiers.begin();

				molog ("  Training soldier's %s (%d to %d)",
					action->sparam1.c_str(),action->iparam1, action->iparam2);



				for (;; ++it) {
					if (it == soldiers_end) {
						molog("   ��Training failed!!, program restart\n");
						program_end(g, false);
						return;
					}
					if (action->sparam1 == "hp") {
						if
							((*it)->get_hp_level()
							 ==
							 static_cast<const uint>(action->iparam1))
							break;
					} else if (action->sparam1 == "attack") {
						if
							((*it)->get_attack_level()
							 ==
							 static_cast<const uint>(action->iparam1))
							break;
					} else if (action->sparam1 == "defense") {
						if
							((*it)->get_defense_level()
							 ==
							 static_cast<const uint>(action->iparam1))
							break;
					} else if (action->sparam1 == "evade") {
						if
							((*it)->get_evade_level()
							 ==
							 static_cast<const uint>(action->iparam1))
							break;
					}
				}
				molog("    okay\n"); // okay, do nothing

				try {
					if (action->sparam1 == "hp")
						(*it)->set_hp_level (action->iparam2);

					else if (action->sparam1 == "attack")
						(*it)->set_attack_level (action->iparam2);

					else if (action->sparam1 == "defense")
						(*it)->set_defense_level (action->iparam2);

					else if (action->sparam1 == "evade")
						(*it)->set_evade_level (action->iparam2);

				} catch (...) {
					throw wexception ("Fail training soldier!!\n");
				}
	         molog("  Training done!\n");

			program_step();
			m_program_timer = true;
			m_program_time = schedule_act(g, 10);
			return;
		}
		case ProductionAction::actPlayFX:
		{
			g_sound_handler.play_fx(action->sparam1, m_position, action->iparam1);

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

	if (m_workers.size())
		m_workers[0]->update_task_buildingwork(g);

	return true;
}

/*
 * returns true if this production site could
 * theoretically start working (if all workers
 * are present)
 */
bool ProductionSite::can_start_working() {return not m_worker_requests.size();}

/*
===============
ProductionSite::get_building_work

There's currently nothing to do for the worker.
Note: we assume that the worker is inside the building when this is called.
===============
*/
bool ProductionSite::get_building_work(Game* g, Worker* w, bool success)
{
	assert(w == m_workers[0]);

	State* state = get_current_program();

	// If unsuccessful: Check if we need to abort current program
	if (!success && state)
	{
		const ProductionAction* action = state->program->get_action(state->ip);

		if (action->type == ProductionAction::actWorker) {
			program_end(g, false);
			state = 0;
		}
	}

	// Default actions first

	if (WareInstance * const item = w->fetch_carried_item(g)) {
		const char * const ware_name = item->get_ware_descr()->name().c_str();
		if (not descr().is_output(ware_name)) molog
			("PSITE: WARNING: carried item %s is not an output item\n", ware_name);

		molog("ProductionSite::get_building_work: start dropoff\n");

		w->start_task_dropoff(g, item);
		return true;
	}

	if (m_fetchfromflag) {
		m_fetchfromflag--;
		w->start_task_fetchfromflag();
		return true;
	}

	// Start program if we haven't already done so
	if (!state)
	{
		m_program_timer = true;
		m_program_time = schedule_act(g, 10);
	}
	else if (state->ip < state->program->get_size())
	{
		const ProductionAction* action = state->program->get_action(state->ip);

		if (action->type == ProductionAction::actWorker) {
			if (state->phase == 0)
			{
				w->start_task_program(action->sparam1);
				state->phase++;
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
void ProductionSite::program_step(const uint phase) {
	State* state = get_current_program();

	assert(state);

	state->ip++;
	state->phase = 0;
}


/*
===============
ProductionSite::program_start

Push the given program onto the stack and schedule acting.
===============
*/
void ProductionSite::program_start(Game* g, std::string program_name)
{
	State state;

	state.program = descr().get_program(program_name);
	state.ip = 0;
	state.phase = 0;
   state.flags = 0;

	m_program.push_back(state);

	m_program_timer = true;
	m_program_time = schedule_act(g, 10);
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
	assert(m_program.size());

	bool dostats = true;

	if (get_current_program()->flags & ProductionAction::pfNoStats)
		dostats = false;

	do
	{
		bool caught = get_current_program()->flags & ProductionAction::pfCatch;

		m_program.pop_back();

		if (caught)
			break;
	} while(m_program.size());

	if (dostats)
		add_statistics_value(success);

   // if succesfull, the workers gain experience
	if (success) {
      uint i=0;
      for(i=0; i<m_workers.size(); i++)
         m_workers[i]->gain_experience(g);
   }

   m_program_timer = true;
	m_program_time = schedule_act(g, m_post_timer);
}
