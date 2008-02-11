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

#ifndef GAME_H
#define GAME_H

#include "cmd_queue.h"
#include "editor_game_base.h"
#include "md5.h"
#include "random.h"
#include "save_handler.h"

namespace UI {struct ProgressWindow;};
class Computer_Player;
class Interactive_Base;
struct Game_Main_Menu_Load_Game;
struct NetGame;
struct WLApplication;

namespace Widelands {

struct Flag;
struct Path;
struct PlayerImmovable;

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

class Player;
class Map_Loader;
class PlayerCommand;
class ReplayReader;
class ReplayWriter;

struct GameInternals;

struct Game : public Editor_Game_Base {
	struct General_Stats {
		std::vector< uint32_t > land_size;
		std::vector< uint32_t > nr_workers;
		std::vector< uint32_t > nr_buildings;
		std::vector< uint32_t > nr_wares;
		std::vector< uint32_t > productivity;
		std::vector< uint32_t > nr_kills;
		std::vector< uint32_t > miltary_strength;
	};
	typedef std::vector<General_Stats> General_Stats_vector;

	friend class Cmd_Queue; // this class handles the commands
	friend class Game_Game_Class_Data_Packet;
	friend class Game_Player_Info_Data_Packet;
	friend class Game_Loader;
	friend struct ::Game_Main_Menu_Load_Game;
	friend struct ::WLApplication;

	// This friend is for legacy reasons and should probably be removed
	// at least after summer 2008, maybe even earlier.
	friend class Game_Interactive_Player_Data_Packet;

	Game();
	~Game();

	// life cycle
	bool run_splayer_map_direct(const char* mapname, bool scenario);
	bool run_single_player ();
	bool run_multi_player (NetGame*);
	bool run_replay();

	/**
	 * Loads a game from filename and runs it. If filename is empty, a dialog is
	 * shown that lets the user chose a file to load. Returns false if the user
	 * cancels the dialog. Otherwise returns the result of running the game.
	 */
	bool run_load_game
		(const bool is_splayer, std::string filename = std::string());

	void load_map (const char*);

	virtual void postload();
	void think();

	ReplayWriter * get_replaywriter() {return m_replaywriter;}

	/**
	 * \return \c true if the game is completely loaded and running (or paused)
	 * or \c false otherwise.
	 */
	bool is_loaded() {return m_state == gs_running;}

	bool can_start();

	void cleanup_for_load
		(const bool flush_graphics = true, const bool flush_animations = true);

	// in-game logic
	Cmd_Queue * get_cmdqueue() {return &cmdqueue;}
	RNG* get_rng() {return &rng;}

	uint32_t logic_rand();

	/// Generate a random location within radius from location.
	Coords random_location(Coords location, uint8_t radius);

	void logic_rand_seed (const uint32_t seed) {rng.seed (seed);}

	StreamWrite& syncstream();
	md5_checksum get_sync_hash() const;

	typedef uint8_t Speed;
	Speed get_speed() const {return m_speed;}
	void set_speed(Speed const speed) {m_speed = speed;}

	bool get_allow_cheats();

	virtual void player_immovable_notification (PlayerImmovable*, losegain_t);
	virtual void player_field_notification (const FCoords&, losegain_t);

	void enqueue_command (Command * const);

	void send_player_command (Widelands::PlayerCommand *);

	void send_player_bulldoze (PlayerImmovable*);
	void send_player_build (int32_t, const Coords&, int32_t);
	void send_player_build_flag (int32_t, const Coords&);
	void send_player_build_road (int32_t, Path &);
	void send_player_flagaction (Flag*, int32_t);
	void send_player_start_stop_building (Building*);
	void send_player_enhance_building (Building*, int32_t);
	void send_player_change_training_options(Building*, int32_t, int32_t);
	void send_player_drop_soldier(Building*, int32_t);
	void send_player_change_soldier_capacity(Building*, int32_t);
	void send_player_enemyflagaction
		(const Flag * const,
		 const int32_t action,
		 const Player_Number,
		 const int32_t num_soldiers,
		 const int32_t type);

	Interactive_Player* get_ipl();

	// If this has a netgame, return it
	NetGame* get_netgame() {return m_netgame;}

	SaveHandler* get_save_handler() {return &m_savehandler;}

	// Statistics
	const General_Stats_vector & get_general_statistics() const {
		return m_general_stats;
	}

	void ReadStatistics(FileRead& fr, uint32_t version);
	void WriteStatistics(FileWrite& fw);

private:
	bool run (UI::ProgressWindow & loader_ui, bool = false);
	void sample_statistics();

private:
	GameInternals* m;

	Map_Loader                   * m_maploader;

	NetGame                      * m_netgame;

	int32_t                            m_state;
	Speed                          m_speed; //  frametime multiplier

	RNG                            rng;

	std::vector<Computer_Player *> cpl;
	Cmd_Queue                      cmdqueue;

	SaveHandler                    m_savehandler;

	ReplayReader* m_replayreader;
	ReplayWriter* m_replaywriter;

	int32_t m_realtime; // the real time (including) pauses in milliseconds

	uint32_t m_last_stats_update;
	General_Stats_vector m_general_stats;

	uint32_t m_player_cmdserial;
};

inline Coords Game::random_location(Coords location, uint8_t radius) {
	const uint16_t s = radius * 2 + 1;
	location.x += logic_rand() % s - radius;
	location.y += logic_rand() % s - radius;
	return location;
}

};

#endif
