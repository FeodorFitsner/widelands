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

#include "game.h"

#include "cmd_check_eventchain.h"
#include "computer_player.h"
#include "events/event_chain.h"
#include "fileread.h"
#include "filewrite.h"
#include "interactive_player.h"
#include "interactive_spectator.h"
#include "fullscreen_menu_launchgame.h"
#include "fullscreen_menu_loadgame.h"
#include "fullscreen_menu_loadreplay.h"
#include "fullscreen_menu_campaign_select.h"
#include "game_loader.h"
#include "game_tips.h"
#include "graphic.h"
#include "i18n.h"
#include "layered_filesystem.h"
#include "map_event_manager.h"
#include "map_trigger_manager.h"
#include "network.h"
#include "player.h"
#include "playercommand.h"
#include "productionsite.h"
#include "replay.h"
#include "soldier.h"
#include "sound/sound_handler.h"
#include "tribe.h"
#include "widelands_map_loader.h"
#include "wlapplication.h"

#include "ui_progresswindow.h"

#include <string>

Game::Game() :
m_state   (gs_none),
m_speed   (1),
cmdqueue  (this),
m_replayreader(0),
m_replaywriter(0),
m_realtime(WLApplication::get()->get_time())
{
	g_sound_handler.m_the_game = this;
	m_last_stats_update = 0;
}

Game::~Game()
{
	if (m_replayreader) {
		delete m_replayreader;
		m_replayreader = 0;
	}
	if (m_replaywriter) {
		delete m_replaywriter;
		m_replaywriter = 0;
	}
	g_sound_handler.m_the_game = NULL;
}


/**
 * Returns true if cheat codes have been activated (single-player only)
 */
bool Game::get_allow_cheats()
{
	return true;
}


/**
 * \return a pointer to the \ref Interactive_Player if any.
 * \note This function may return 0 (in particular, it will return 0 during
 * playback)
 */
Interactive_Player* Game::get_ipl()
{
	return dynamic_cast<Interactive_Player*>(get_iabase());
}


/** Game::can_start()
 *
 * Returns true if the game settings are valid.
 */
bool Game::can_start()
{
	int local_num;
	int i;

	if (!get_map())
		return false;

	// we need exactly one local player
	local_num = -1;
	for(i = 1; i <= MAX_PLAYERS; i++) {
		if (!get_player(i))
			continue;

		if (get_player(i)->get_type() == Player::Local) {
			if (local_num < 0)
				local_num = i;
			else
				return false;
		}
	}
	if (local_num < 0)
		return false;

	return true;
}

bool Game::run_splayer_map_direct(const char* mapname, bool scenario) {
	m_netgame = 0;
	m_state = gs_loading;

	assert(!get_map());

	Map *m = new Map();
	set_map(m);

	FileSystem* fs = g_fs->MakeSubFileSystem( mapname );
	m_maploader = new Widelands_Map_Loader(*fs, m);
	UI::ProgressWindow loaderUI;
	GameTips tips (loaderUI);

	// Loading the locals for the campaign
	std::string camp_textdomain("");
	if( scenario )
		{
		loaderUI.step (_("Preloading a map")); // Must be printed before loading the scenarios textdomain, else it won't be translated.
		camp_textdomain.append(mapname);
		i18n::grab_textdomain(camp_textdomain.c_str());
		log("Loading the locals for scenario. file: %s.mo\n", mapname);
		m_maploader->preload_map(scenario);
		i18n::release_textdomain(); // To see a translated loaderUI-Texts
		}
	else // we are not loading a scenario, so no ingame texts to be translated.
		{
		loaderUI.step (_("Preloading a map"));
		m_maploader->preload_map(scenario);
		}

	const std::string background = m->get_background();
	if (background.size() > 0)
		loaderUI.set_background(background);
	loaderUI.step (_("Loading a world"));
	m_maploader->load_world();

    // We have to create the players here
	const Player_Number nr_players = m->get_nrplayers();
	for (Player_Number i = 1; i <= nr_players; ++i) {
		loaderUI.stepf (_("Adding player %u"), i);
		add_player
			(i,
			 i == 1 ? Player::Local : Player::AI,
			 m->get_scenario_player_tribe(i),
			 m->get_scenario_player_name(i));
	}

	set_iabase(new Interactive_Player(*this, 0));

	loaderUI.step (_("Loading a map")); // Must be printed before loading the scenarios textdomain, else it won't be translated.

	// Reload campaign textdomain
	if( scenario )
		i18n::grab_textdomain(camp_textdomain.c_str());

	m_maploader->load_map_complete(this, scenario); // if code==2 is a scenario
	delete m_maploader;
	m_maploader=0;

	if( scenario )
		i18n::release_textdomain();

	return run(loaderUI);
}


bool Game::run_single_player ()
{
	m_state = gs_menu;
	m_netgame=0;

	m_maploader=0;
	Fullscreen_Menu_LaunchGame lgm(this, 0, &m_maploader);
	const int code = lgm.run();

	if (code==0 || get_map()==0)
	    return false;

	g_gr->flush(PicMod_Menu);

	m_state = gs_loading;
	UI::ProgressWindow loaderUI(map().get_background());
	GameTips tips (loaderUI);

	set_iabase(new Interactive_Player(*this, 0));

	loaderUI.step(_("Loading a map"));
	// Now first, completly load the map
	m_maploader->load_map_complete(this, code==2); // if code==2 is a scenario
	delete m_maploader;
	m_maploader=0;

	return run(loaderUI);
}


/**
 * Run Campaign UI
 * Only the UI is loaded, real loading of the map will
 * take place in run_splayer_map_direct
 */
bool Game::run_campaign()
{

	m_state = gs_menu;
	m_netgame = 0;

	// set variables
	int campaign;
	int loop = 1;
	std::string campmapfile;

	// Campaign UI - Loop
	while (loop==1) {
		// First start UI for selecting the campaign
		Fullscreen_Menu_CampaignSelect select_campaign;
		if (select_campaign.run())
			campaign=select_campaign.get_campaign();
		if (campaign == -1) // Back was pressed
			return false;

		// Than start UI for the selected campaign
		Fullscreen_Menu_CampaignMapSelect select_campaignmap;
		if (select_campaignmap.run())
			campmapfile = select_campaignmap.get_map();
			campaign = select_campaign.get_campaign();
		if (campaign != -1) // Gets -1 if back was pressed
			loop=0;
	}

	// Load selected campaign-map-file
	return run_splayer_map_direct(campmapfile.c_str(),true);
}


/**
 * Load a game
 * argument defines if this is a single player game (true)
 * or networked (false)
 */
bool Game::run_load_game(const bool is_splayer, std::string filename) {
	assert(is_splayer); // TODO: net game saving not supported

	if (filename.empty()) {
		Fullscreen_Menu_LoadGame ssg(*this);
		if (ssg.run() > 0)
			filename = ssg.filename();
		else
			return false;
	}

	UI::ProgressWindow loaderUI;
	GameTips tips (loaderUI);

	// We have to create an empty map, otherwise nothing will load properly
	set_map(new Map);

	FileSystem * const fs = g_fs->MakeSubFileSystem(filename.c_str());

	m_state = gs_loading;

	set_iabase(new Interactive_Player(*this, 0));

	Game_Loader gl(*fs, this);
	loaderUI.step(_("Loading..."));
	gl.load_game();
	delete fs;

	return run(loaderUI, true);
}

bool Game::run_multi_player (NetGame* ng)
{
	m_state = gs_menu;
	m_netgame=ng;

	m_maploader=0;
	Fullscreen_Menu_LaunchGame lgm(this, m_netgame, &m_maploader);
	m_netgame->set_launch_menu (&lgm);
	const int code = lgm.run();
	m_netgame->set_launch_menu (0);

	if (code==0 || get_map()==0)
	    return false;

	UI::ProgressWindow loaderUI;
	g_gr->flush(PicMod_Menu);

	m_state = gs_loading;

	set_iabase(new Interactive_Player(*this, 0));

	// Now first, completly load the map
	loaderUI.step(_("Loading a map"));
	m_maploader->load_map_complete(this, false); // if code==2 is a scenario
	delete m_maploader;
	m_maploader=0;

	loaderUI.step(_("Initializing a network game"));
	m_netgame->begin_game();

	return run(loaderUI);
}


/**
 * Display the fullscreen menu to choose a replay,
 * then start the interactive replay.
 */
bool Game::run_replay()
{
	Fullscreen_Menu_LoadReplay rm(this);

	if (rm.run() <= 0)
		return false;

	log("Selected replay: %s\n", rm.filename().c_str());

	UI::ProgressWindow loaderUI;
	GameTips tips (loaderUI);

	// We have to create an empty map, otherwise nothing will load properly
	set_map(new Map);

	m_state = gs_loading;
	set_iabase(new Interactive_Spectator(this));

	loaderUI.step(_("Loading..."));

	m_replayreader = new ReplayReader(this, rm.filename());

	return run(loaderUI, true);
}


void Game::load_map (const char* filename)
{
	m_maploader = (new Map())->get_correct_loader(filename);
	assert (m_maploader!=0);
	m_maploader->preload_map(0);
	set_map (m_maploader->get_map());
}


/**
 * Called for every game after loading (from a savegame or just from a map
 * during single/multiplayer/scenario).
 *
 * Ensure that players and player controllers are setup properly (in particular
 * AI and the \ref Interactive_Player if any).
 */
void Game::postload()
{
	Editor_Game_Base::postload();

	assert(get_iabase() != 0);

	// Set up computer controlled players
	// unless we're watching a replay
	if (Interactive_Player* ipl = dynamic_cast<Interactive_Player*>(get_iabase())) {
		const Player_Number nr_players = map().get_nrplayers();
		for (Player_Number i = 1; i <= nr_players; ++i) {
			Player* player = get_player(i);

			if (player) {
				if (player->get_type() == Player::AI) {
					cpl.push_back (new Computer_Player(*this, i));
				} else if (player->get_type() == Player::Local) {
					ipl->set_player_number(i);
				}
			}
		}
	}

	get_iabase()->postload();
}


/**
 * This runs a game, including game creation phase.
 *
 * The setup and loading of a game happens (or rather: will happen) in three
 * stages.
 * 1.  First of all, the host (or single player) configures the game. During this
 *     time, only short descriptions of the game data (such as map headers )are
 *     loaded to minimize loading times.
 * 2a. Once the game is about to start and the configuration screen is finished,
 *     all logic data (map, tribe information, building information) is loaded
 *     during postload.
 * 2b. If a game is created, initial player positions are set. This step is
 *     skipped when a game is loaded.
 * 3.  After this has happened, the game graphics are loaded.
 *
 * \return true if a game actually took place, false otherwise
 */
bool Game::run(UI::ProgressWindow & loader_ui, bool is_savegame) {
	postload();

	if (not is_savegame) {
		std::string step_description = _("Creating player infrastructure");
		// Prepare the players (i.e. place HQs)
		const Player_Number nr_players = map().get_nrplayers();
		for (Player_Number i = 1; i <= nr_players; ++i) if
			(Player * const plr = get_player(i))
		{
			step_description += ".";
			loader_ui.step(step_description);
			plr->init(true);

			if (plr->get_type() == Player::Local)
				get_ipl()->move_view_to(map().get_starting_pos(i));
		}

		// Prepare the map, set default textures
		map().recalc_default_resources();
		map().get_mem().delete_unreferenced_events  ();
		map().get_mtm().delete_unreferenced_triggers();

		// Finally, set the scenario names and tribes to represent
		// the correct names of the players
		for (Player_Number curplr = 1; curplr <= nr_players; ++curplr) {
			const Player * const plr = get_player(curplr);
			const std::string                                             no_name;
			const std::string &  tribe_name = plr ? plr->tribe().name() : no_name;
			const std::string & player_name = plr ? plr->    get_name() : no_name;
			map().set_scenario_player_tribe(curplr,  tribe_name);
			map().set_scenario_player_name (curplr, player_name);
		}

		// Everything prepared, send the first trigger event
		// We lie about the sender here. Hey, what is one lie in a lifetime?
		enqueue_command (new Cmd_CheckEventChain(get_gametime(), -1));
	}

	load_graphics(loader_ui);

	g_sound_handler.change_music("ingame", 1000, 0);

	m_state = gs_running;

	if (!m_replayreader) {
		log("Starting replay writer\n");

		// Derive a replay filename from the current time
		time_t t;
		time(&t);
		char* current_time = ctime(&t);
		// Remove trailing newline
		std::string time_string(current_time, strlen(current_time)-1);
		SSS_T pos = std::string::npos;
		// ':' is not a valid file name character under Windows,
		// so we replace it with '.'
		while ((pos = time_string.find (':')) != std::string::npos) {
			time_string[pos] = '.';
		}

		std::string fname(REPLAY_DIR);
		fname += time_string;
		fname += REPLAY_SUFFIX;

		m_replaywriter = new ReplayWriter(this, fname);
		log("Replay writer has started\n");
	}

	get_iabase()->run();

	g_sound_handler.change_music("menu", 1000, 0);

	cleanup_objects();
	delete get_iabase();
	set_iabase(0);

	for (unsigned int i=0; i<cpl.size(); i++)
		delete cpl[i];

	g_gr->flush(PicMod_Game);
	g_anim.flush();

	m_state = gs_none;

	return true;
}


/**
 * think() is called by the UI objects initiated during Game::run()
 * during their modal loop.
 * Depending on the current state we advance game logic and stuff,
 * running the cmd queue etc.
 */
void Game::think(void)
{
	if (m_netgame!=0)
	    m_netgame->handle_network ();

	if (m_state == gs_running) {
		for (unsigned int i=0;i<cpl.size();i++)
			cpl[i]->think();

		if (!m_general_stats.size() ||
		    get_gametime() - m_last_stats_update > STATISTICS_SAMPLE_TIME) {
			sample_statistics();

			for (Player_Number curplr = 1; curplr <= get_map()->get_nrplayers(); ++curplr) {
				Player* plr = get_player(curplr);
				if (plr)
					plr->sample_statistics();
			}

			m_last_stats_update = get_gametime();
		}

		int frametime = -m_realtime;
		m_realtime =  WLApplication::get()->get_time();
		frametime += m_realtime;

		if (m_netgame!=0) {
			int max_frametime=m_netgame->get_max_frametime();

			if (frametime>max_frametime)
				frametime = max_frametime; //  wait for the next server message
			else if (max_frametime-frametime>500)
				//  we are too long behind network time, so hurry a little
				frametime += (max_frametime - frametime) / 2;
		}
		else
			frametime *= get_speed();

		// Maybe we are too fast...
		// Note that the time reported by WLApplication might jump backwards
		// when playback stops.
		if (frametime <= 0)
			return;

		// prevent frametime escalation in case the game logic is the performance bottleneck
		if (frametime > 1000)
			frametime = 1000;

		if (m_replayreader) {
			for(;;) {
				PlayerCommand* cmd = m_replayreader->GetPlayerCommand(get_gametime() + frametime);
				if (!cmd)
					break;

				enqueue_command(cmd);
			}

			if (m_replayreader->EndOfReplay())
				dynamic_cast<Interactive_Spectator*>(get_iabase())->end_of_game();
		}

		cmdqueue.run_queue(frametime, get_game_time_pointer());

		g_gr->animate_maptextures(get_gametime());

		// check if autosave is needed, but only if that is not a network game
		if (NULL == m_netgame)
			m_savehandler.think(this, m_realtime);
	}
}


/**
 * Change the game speed.
 */
void Game::set_speed(int speed)
{
	assert(speed >= 0);

	m_speed = speed;
}


void Game::player_immovable_notification (PlayerImmovable* pi, losegain_t lg)
{
	for (unsigned int i=0;i<cpl.size();i++)
		if (cpl[i]->get_player_number()==pi->get_owner()->get_player_number())
			if (lg==GAIN)
				cpl[i]->gain_immovable (pi);
			else
				cpl[i]->lose_immovable (pi);

	if (lg==GAIN)
		pi->get_owner()->gain_immovable (pi);
	else
		pi->get_owner()->lose_immovable (pi);
}

void Game::player_field_notification (const FCoords& fc, losegain_t lg)
{
	for (unsigned int i=0;i<cpl.size();i++)
		if (cpl[i]->get_player_number()==fc.field->get_owned_by())
			if (lg==GAIN)
				cpl[i]->gain_field (fc);
			else
				cpl[i]->lose_field (fc);
}

/**
 * Cleanup for load
 */
void Game::cleanup_for_load
(const bool flush_graphics, const bool flush_animations)
{
	m_state = gs_loading;

	Editor_Game_Base::cleanup_for_load(flush_graphics, flush_animations);
	for
		(std::vector<Tribe_Descr*>::iterator it = m_tribes.begin();
		 it != m_tribes.end();
		 ++it)
		delete *it;
	m_tribes.resize(0);
	get_cmdqueue()->flush();
	while(cpl.size()) {
		delete cpl[cpl.size()-1];
		cpl.pop_back();
	}

	// Statistics
	m_last_stats_update = 0;
	m_general_stats.clear();
}

/**
 * All player-issued commands must enter the queue through this function.
 * It takes the appropriate action, i.e. either add to the cmd_queue or send
 * across the network.
 */
void Game::send_player_command (PlayerCommand* pc)
{
	if (m_netgame and get_player(pc->get_sender())->get_type() == Player::Local)
		m_netgame->send_player_command (pc);
	else
		enqueue_command (pc);
}


/**
 * Actually enqueue a command.
 *
 * \note In a network game, player commands are only allowed to enter the
 * command queue after being accepted by the networking logic via
 * \ref send_player_command , so you must never enqueue a player command directly.
 */
void Game::enqueue_command (BaseCommand * const cmd)
{
	if (m_replaywriter) {
		PlayerCommand* plcmd = dynamic_cast<PlayerCommand*>(cmd);
		if (plcmd)
			m_replaywriter->SendPlayerCommand(plcmd);
	}

	cmdqueue.enqueue(cmd);
}

// we might want to make these inlines:
void Game::send_player_bulldoze (PlayerImmovable* pi)
{
	send_player_command (new Cmd_Bulldoze(get_gametime(), pi->get_owner()->get_player_number(), pi));
}

void Game::send_player_build (int pid, const Coords& coords, int id)
{
	send_player_command (new Cmd_Build(get_gametime(), pid, coords, id));
}

void Game::send_player_build_flag (int pid, const Coords& coords)
{
	send_player_command (new Cmd_BuildFlag(get_gametime(), pid, coords));
}

void Game::send_player_build_road (int pid, Path & path)
{
	send_player_command (new Cmd_BuildRoad(get_gametime(), pid, path));
}

void Game::send_player_flagaction (Flag* flag, int action)
{
	send_player_command (new Cmd_FlagAction(get_gametime(), flag->get_owner()->get_player_number(), flag, action));
}

void Game::send_player_start_stop_building (Building* b)
{
	send_player_command (new Cmd_StartStopBuilding(get_gametime(), b->get_owner()->get_player_number(), b));
}

void Game::send_player_enhance_building (Building* b, int id)
{
	assert(id!=-1);

	send_player_command (new Cmd_EnhanceBuilding(get_gametime(), b->get_owner()->get_player_number(), b, id));
}

void Game::send_player_change_training_options(Building* b, int atr, int val)
{

	send_player_command (new Cmd_ChangeTrainingOptions(get_gametime(), b->get_owner()->get_player_number(), b, atr, val));
}

void Game::send_player_drop_soldier (Building* b, int ser)
{
	assert(ser != -1);
	send_player_command (new Cmd_DropSoldier(get_gametime(), b->get_owner()->get_player_number(), b, ser));
}

void Game::send_player_change_soldier_capacity (Building* b, int val)
{
	send_player_command (new Cmd_ChangeSoldierCapacity(get_gametime(), b->get_owner()->get_player_number(), b, val));
}

/////////////////////// TESTING STUFF
void Game::send_player_enemyflagaction
(const Flag * const flag,
 const int action,
 const Player_Number who_attacks,
 const int num_soldiers,
 const int type)
{
	send_player_command (new Cmd_EnemyFlagAction(get_gametime(), who_attacks, flag, action, who_attacks, num_soldiers, type));
}


/**
 * Sample global statistics for the game.
 */
void Game::sample_statistics()
{
	// Update general stats
	Map* m = get_map();
	std::vector< uint > land_size; land_size.resize( m->get_nrplayers() );
	std::vector< uint > nr_buildings; nr_buildings.resize( m->get_nrplayers() );
	std::vector< uint > nr_kills; nr_kills.resize( m->get_nrplayers() );
	std::vector< uint > miltary_strength; miltary_strength.resize( m->get_nrplayers() );
	std::vector< uint > nr_workers; nr_workers.resize( m->get_nrplayers() );
	std::vector< uint > nr_wares; nr_wares.resize( m->get_nrplayers() );
	std::vector< uint > productivity; productivity.resize( m->get_nrplayers() );

	std::vector< uint > nr_production_sites; nr_production_sites.resize( m->get_nrplayers() );

	// We walk the map, to gain all needed informations
	for(ushort y = 0; y < m->get_height(); y++) {
		for(ushort x = 0; x < m->get_width(); x++) {
			Field* f = m->get_field( Coords( x, y ) );

			// First, ownership of this field
			if (f->get_owned_by())
				land_size[ f->get_owned_by()-1 ]++;

			// Get the immovable
			BaseImmovable* imm = f->get_immovable();
			if (imm && imm->get_type() == Map_Object::BUILDING) {
				Building* build = static_cast<Building*>(imm);
				if (build->get_position() == Coords(x,y)) { // only main location is intresting
					// Ok, count the building
					nr_buildings[ build->get_owner()->get_player_number() - 1 ]++;

					// If it is a productionsite, add its productivity
					if( build->get_building_type() == Building::PRODUCTIONSITE ) {
						nr_production_sites[  build->get_owner()->get_player_number() - 1 ]++;
						productivity[ build->get_owner()->get_player_number() - 1 ] += static_cast<ProductionSite*>( build )->get_statistics_percent();
					}
				}
			}

			// Now, walk the bobs
			if( f->get_first_bob() ) {
				Bob* b = f->get_first_bob();
				do {
					if( b->get_bob_type() == Bob::WORKER ) {
						Worker* w = static_cast<Worker*>(b);

						switch( w->get_worker_type() ) {
							case Worker_Descr::SOLDIER:
							{
								Soldier* s = static_cast<Soldier*>(w);
								uint calc_level = s->get_level(atrTotal) + 1; // So that level 0 loosers also count something
								miltary_strength[ s->get_owner()->get_player_number() -1 ] += calc_level;
							}
							break;

							default: break;
						}
					}
				} while( (b = b->get_next_bob() ) );
			}
		}
	}

	// Number of workers / wares
	for(uint i = 0; i < m->get_nrplayers(); i++) {
		Player* plr = get_player(i+1);

		uint wostock = 0;
		uint wastock = 0;

		for(uint j = 0; plr && j < plr->get_nr_economies(); j++) {
			Economy* eco = plr->get_economy_by_number( j );

			for( int wareid = 0; wareid < plr->tribe().get_nrwares(); wareid++)
				wastock += eco->stock_ware( wareid );
			for( int workerid = 0; workerid < plr->tribe().get_nrworkers(); workerid++) {
				if( plr->tribe().get_worker_descr( workerid )->get_worker_type() == Worker_Descr::CARRIER)
					continue;
				wostock += eco->stock_worker( workerid );
			}
		}
		nr_wares[ i ] = wastock;
		nr_workers[ i ] = wostock;
	}

	// Now, divide the statistics
	for(uint i = 0; i < m->get_nrplayers(); i++) {
		if (productivity[ i ])
			productivity[ i ] /= nr_production_sites[ i ];
	}

	// Now, push this on the general statistics
	m_general_stats.resize( m->get_nrplayers() );
	for( uint i = 0; i < m->get_nrplayers(); i++) {
		m_general_stats[i].land_size.push_back( land_size[i] );
		m_general_stats[i].nr_buildings.push_back( nr_buildings[i] );
		m_general_stats[i].nr_kills.push_back( nr_kills[i] );
		m_general_stats[i].miltary_strength.push_back( miltary_strength[i] );
		m_general_stats[i].nr_workers.push_back( nr_workers[i] );
		m_general_stats[i].nr_wares.push_back( nr_wares[i]  );
		m_general_stats[i].productivity.push_back( productivity[i] );
	}
}


/**
 * Read statistics data from a file.
 *
 * \param version indicates the kind of statistics file, which may be
 *   0 - old style statistics (from the time when statistics were kept in
 *       \ref Interactive_Player )
 *   1 - current version
 */
void Game::ReadStatistics(FileRead& fr, uint version)
{
	if (version == 0 || version == 1) {
		if (version >= 1) {
			m_last_stats_update = fr.Unsigned32();
		}

		// Read general statistics
		uint entries = fr.Unsigned16();
		m_general_stats.resize(get_map()->get_nrplayers());

		for(uint i = 0; i < get_map()->get_nrplayers(); i++) {
			if (get_player(i+1)) {
				m_general_stats[i].land_size.resize(entries);
				m_general_stats[i].nr_workers.resize(entries);
				m_general_stats[i].nr_buildings.resize(entries);
				m_general_stats[i].nr_wares.resize(entries);
				m_general_stats[i].productivity.resize(entries);
				m_general_stats[i].nr_kills.resize(entries);
				m_general_stats[i].miltary_strength.resize(entries);
			}
		}

		for(uint i = 0; i < get_map()->get_nrplayers(); i++) {
			if (!get_player(i+1))
				continue;

			for(uint j = 0; j < m_general_stats[i].land_size.size(); j++) {
				m_general_stats[i].land_size[j] = fr.Unsigned32();
				m_general_stats[i].nr_workers[j] = fr.Unsigned32();
				m_general_stats[i].nr_buildings[j] = fr.Unsigned32();
				m_general_stats[i].nr_wares[j] = fr.Unsigned32();
				m_general_stats[i].productivity[j] = fr.Unsigned32();
				m_general_stats[i].nr_kills[j] = fr.Unsigned32();
				m_general_stats[i].miltary_strength[j] = fr.Unsigned32();
			}
		}
	} else
		throw wexception("Unsupported version %i", version);
}


/**
 * Write general statistics to the given file.
 */
void Game::WriteStatistics(FileWrite& fw)
{
	fw.Unsigned32(m_last_stats_update);

	// General statistics
	// First, we write the size of the statistics arrays
	uint entries = 0;

	for(uint i = 0; i < get_map()->get_nrplayers(); i++) {
		if (get_player(i+1) && m_general_stats.size()) {
			entries = m_general_stats[i].land_size.size();
			break;
		}
	}

	fw.Unsigned16(entries);

	for(uint i = 0; i < get_map()->get_nrplayers(); i++) {
		if (!get_player(i+1))
			continue;

		for( uint j = 0; j < entries; j++) {
			fw.Unsigned32(m_general_stats[i].land_size[j]);
			fw.Unsigned32(m_general_stats[i].nr_workers[j]);
			fw.Unsigned32(m_general_stats[i].nr_buildings[j]);
			fw.Unsigned32(m_general_stats[i].nr_wares[j]);
			fw.Unsigned32(m_general_stats[i].productivity[j]);
			fw.Unsigned32(m_general_stats[i].nr_kills[j]);
			fw.Unsigned32(m_general_stats[i].miltary_strength[j]);
		}
	}
}
