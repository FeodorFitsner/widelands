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

#ifndef __BOB_H
#define __BOB_H

#include "animation.h"


/// \todo (Antonio Trueba#1#): Get rid of forward declarations (cleanup of other headers needed)
class Map;
class Route;
class Transfer;
class Tribe_Descr;


/**
 * BobProgramBase is only used that
 * get_name always works
 */

struct BobProgramBase {
	virtual ~BobProgramBase() {}
	virtual std::string get_name() const = 0;
};


/**
 * Bobs are moving map objects: Animals, humans, ships...
 */
struct Bob : public Map_Object {
	friend class Widelands_Map_Bobdata_Data_Packet;
	friend class Widelands_Map_Bob_Data_Packet;

		struct State;
		typedef void (Bob::*Ptr)(Game*, State*);
		enum Type {CRITTER,	WORKER};

		struct Task {
			const char* name;

			Ptr update;
			Ptr signal;
			Ptr mask;
		};

	struct State {
		State(const Task * const the_task = 0) :
			task    (the_task),
			ivar1   (0),
			ivar2   (0),
			ivar3   (0),
			coords  (Coords::Null()),
			diranims(0),
			path    (0),
			transfer(0),
			route   (0),
			program (0)
		{}

			const Task           * task;
			int                    ivar1;
			int                    ivar2;
			union                  {int ivar3; Uint32 ui32var3;};
			Object_Ptr             objvar1;
			std::string            svar1;

			Coords                 coords;
			const DirAnimations  * diranims;
			Path                 * path;
			Transfer             * transfer;
			Route                * route;
			const BobProgramBase * program; // pointer to current program class
		};

		struct Descr: public Map_Object_Descr {
			friend class Widelands_Map_Bobdata_Data_Packet; // To write it to a file

				Descr(const Tribe_Descr * const tribe,
				      const std::string & bob_name): m_name(bob_name),
                                                     m_owner_tribe(tribe)
						{m_default_encodedata.clear();}

				virtual ~Descr() {};
				const std::string & name() const throw () {return m_name;}
				Bob *create(Editor_Game_Base *g, Player *owner, Coords coords);
				bool is_world_bob() const {return not m_owner_tribe;}

				inline const char* get_picture(void) const
						{return m_picture.c_str();}

				inline const EncodeData& get_default_encodedata() const
						{return m_default_encodedata;}

				const Tribe_Descr * const get_owner_tribe() const throw ()
						{return m_owner_tribe;}

				static Descr *create_from_dir(const char *name,
											  const char *directory,
											  Profile *prof,
											  Tribe_Descr* tribe);

				uint vision_range() const;

			protected:
				virtual Bob * create_object() const = 0;
				virtual void parse(const char *directory, Profile *prof,
								   const EncodeData *encdata);

				const std::string   m_name;
				std::string         m_picture;
				EncodeData          m_default_encodedata;
				const Tribe_Descr * const m_owner_tribe; //  0 if world bob
		};

		MO_DESCR(Descr);

		uint get_current_anim() const {return m_anim;}
		int get_animstart() const {return m_animstart;}

		virtual int get_type() const throw () {return BOB;}
		virtual Type get_bob_type() const throw () = 0;
		virtual uint get_movecaps() {return 0;}
		const std::string & name() const throw () {return descr().name();}

		virtual void init(Editor_Game_Base*);
		virtual void cleanup(Editor_Game_Base*);
		virtual void act(Game*, uint data);
		void schedule_destroy(Game* g);
		void schedule_act(Game* g, uint tdelta);
		void skip_act();
		void force_skip_act();
		Point calc_drawpos(const Editor_Game_Base &, const Point) const;
		void set_owner(Player *player);
		Player * get_owner() const {return m_owner;}
		void set_position(Editor_Game_Base* g, Coords f);
		inline const FCoords& get_position() const {return m_position;}
		Bob * get_next_bob() const throw () {return m_linknext;}
		bool is_world_bob() const throw () {return descr().is_world_bob();}

		uint vision_range() const { return descr().vision_range(); }

		virtual void draw(const Editor_Game_Base &, RenderTarget &,
		                  const Point) const;


		// For debug
		virtual void log_general_info(Editor_Game_Base* egbase);

		// default tasks
		void reset_tasks(Game*);
		void send_signal(Game*, std::string sig);
		void start_task_idle(Game*, uint anim, int timeout);

		bool start_task_movepath(Game*, const Coords dest, const int persist,
								 const DirAnimations &,
								 const bool forceonlast = false,
								 const int only_step = -1);

		void start_task_movepath(const Path &, const DirAnimations &,
		                         const bool forceonlast = false,
		                         const int only_step = -1);

		bool start_task_movepath(const Map &, const Path &, const int index,
		                         const DirAnimations &,
		                         const bool forceonlast = false,
		                         const int only_step = -1);

		void start_task_forcemove(const int dir, const DirAnimations &);

		// higher level handling (task-based)
		inline State* get_state()
				{return m_stack.size() ? &m_stack[m_stack.size() - 1] : 0;}

		State & top_state()
				{assert(m_stack.size()); return m_stack[m_stack.size() - 1];}

		inline std::string get_signal() {return m_signal;}
		State* get_state(Task* task);
		void push_task(const Task & task);
		void pop_task();

		/**
		 * Simply set the signal string without calling any functions.
		 *
		 * You should use this function to unset a signal, or to set a signal
		 * just before calling pop_task().
		 */
		void set_signal(std::string sig) {m_signal = sig;};

		/// Automatically select a task.
		virtual void init_auto_task(Game*) {};

		// low level animation and walking handling
		void set_animation(Editor_Game_Base* g, uint anim);
		int start_walk(Game* g, WalkingDir dir, uint anim, bool force = false);

		/**
		 * Call this from your task_act() function that was scheduled after
		 * start_walk().
		 */
		void end_walk() {m_walking = IDLE;}

		/// \return true if we're currently walking
		bool is_walking() {return m_walking != IDLE;}


	protected:
		Bob(const Descr & descr);
		virtual ~Bob(void);


	private:
		void do_act(Game* g, bool signalhandling);
		void idle_update(Game* g, State* state);
		void idle_signal(Game* g, State* state);
		void movepath_update(Game* g, State* state);
		void movepath_signal(Game* g, State* state);
		void forcemove_update(Game* g, State* state);

		static Task taskIdle;
		static Task taskMovepath;
		static Task taskForcemove;

		Player   * m_owner; // can be 0
		FCoords    m_position; // where are we right now?
		Bob      * m_linknext; // next object on this field
		Bob    * * m_linkpprev;
		uint       m_actid; // CMD_ACT counter, used to eliminate spurious act()s
		uint       m_anim;
		int        m_animstart; ///< gametime when the animation was started
		WalkingDir m_walking;
		int        m_walkstart; ///< start time (used for interpolation)
		int        m_walkend;   ///< end time (used for interpolation)

		// Task framework variables
		std::vector<State> m_stack;

		bool        m_stack_dirty;
		bool        m_sched_init_task; ///< if init_auto_task was scheduled
		bool        m_in_act; ///< if do_act is currently running (blocking signals)
		std::string m_signal;
};

#endif
