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

#ifndef __S__GAME_H
#define __S__GAME_H

#include "cmd_queue.h"
#include "editor_game_base.h"
#include "random.h"
#include "save_handler.h"

struct Flag;
struct Path;
struct PlayerImmovable;
namespace UI {struct ProgressWindow;};

class ReplayWriter;

#define WLGF_SUFFIX ".wgf"
#define WLGF_MAGIC      "WLgf"

/** class Game
 *
 * This class manages the entire lifetime of a game session, from creating the
 * game and setting options, selecting maps to the actual playing phase and the
 * final statistics screen(s).
 */
enum {
	gs_none    = 0, //  not connected, nothing
	gs_menu,        //  in the setup menu(s)
	gs_loading,     //  menu has completed, but the game is still loading
	gs_running      //  in-game
};

class FileRead;
class FileWrite;
class Player;
class Interactive_Player;
class Computer_Player;
class Map_Loader;
class BaseCommand;
class PlayerCommand;
class NetGame;

struct Game : public Editor_Game_Base {
public:
	struct General_Stats {
		std::vector< uint > land_size;
		std::vector< uint > nr_workers;
		std::vector< uint > nr_buildings;
		std::vector< uint > nr_wares;
		std::vector< uint > productivity;
		std::vector< uint > nr_kills;
		std::vector< uint > miltary_strength;
	};
	typedef std::vector<General_Stats> General_Stats_vector;

public:
	friend class Cmd_Queue; // this class handles the commands
	friend class Game_Game_Class_Data_Packet;
	friend class Game_Player_Info_Data_Packet;
	friend class Game_Loader;
	friend class Game_Main_Menu_Load_Game;

	// This friend is for legacy reasons and should probably be removed
	// at least after summer 2008, maybe even earlier.
	friend class Game_Interactive_Player_Data_Packet;

	Game(void);
	~Game(void);

	// life cycle
	bool run_splayer_map_direct(const char* mapname, bool scenario);
	bool run_single_player ();
	bool run_campaign ();
	bool run_multi_player (NetGame*);

	/**
	 * Loads a game from filename and runs it. If filename is empty, a dialog is
	 * shown that lets the user chose a file to load. Returns false if the user
	 * cancels the dialog. Otherwise returns the result of running the game.
	 */
	bool run_load_game
		(const bool is_splayer, std::string filename = std::string());

	void load_map (const char*);

	void think(void);

	/**
	 * \return \c true if the game is completely loaded and running (or paused)
	 * or \c false otherwise.
	 */
	bool is_loaded() { return m_state == gs_running; }

	bool can_start();

	void cleanup_for_load
		(const bool flush_graphics = true, const bool flush_animations = true);

	// in-game logic
	Cmd_Queue * get_cmdqueue() {return &cmdqueue;}

	// Start using logic_rand() for the actual gamelogic (e.g. critter).
	// Do NOT use for random events in the UI or other display code.
	// This will allow us to plug another PRNG in here for game playbacks
	// and other fancy stuff.
	uint logic_rand() {return rng.rand();}

	/// Generate a random location within radius from location.
	Coords random_location(Coords location, uchar radius);

	void logic_rand_seed (const uint seed) {rng.seed (seed);}

	int get_speed() const { return m_speed; }
	void set_speed(int speed);

	bool get_allow_cheats();

	virtual void player_immovable_notification (PlayerImmovable*, losegain_t);
	virtual void player_field_notification (const FCoords&, losegain_t);

	void enqueue_command (BaseCommand * const);

	void send_player_command (PlayerCommand*);

	void send_player_bulldoze (PlayerImmovable*);
	void send_player_build (int, const Coords&, int);
	void send_player_build_flag (int, const Coords&);
	void send_player_build_road (int, Path &);
	void send_player_flagaction (Flag*, int);
	void send_player_start_stop_building (Building*);
	void send_player_enhance_building (Building*, int);
	void send_player_change_training_options(Building*, int, int);
	void send_player_drop_soldier(Building*, int);
	void send_player_change_soldier_capacity(Building*, int);
	void send_player_enemyflagaction
		(const Flag * const,
		 const int action,
		 const Player_Number,
		 const int num_soldiers,
		 const int type);

	Interactive_Player* get_ipl(void) { return ipl; }

	// If this has a netgame, return it
	NetGame* get_netgame( void ) { return m_netgame; }

	SaveHandler* get_save_handler() { return &m_savehandler; }

	// Statistics
	const General_Stats_vector & get_general_statistics() const {
		return m_general_stats;
	}

	void ReadStatistics(FileRead& fr, uint version);
	void WriteStatistics(FileWrite& fw);

private:
	void init_player_controllers ();
	bool run (UI::ProgressWindow & loader_ui, bool = false);
	void sample_statistics();

private:
	Map_Loader                   * m_maploader;

	NetGame                      * m_netgame;

	int                            m_state;
	int                            m_speed; //  frametime multiplier

	RNG                            rng;

	Interactive_Player           * ipl;
	std::vector<Computer_Player *> cpl;
	Cmd_Queue                      cmdqueue;

	SaveHandler                    m_savehandler;

	ReplayWriter* m_replaywriter;

	int m_realtime; // the real time (including) pauses in milliseconds

	uint m_last_stats_update;
	General_Stats_vector m_general_stats;
};

inline Coords Game::random_location(Coords location, uchar radius) {
	const ushort s = radius * 2 + 1;
	location.x += logic_rand() % s - radius;
	location.y += logic_rand() % s - radius;
	return location;
}

#endif // __S__GAME_H
