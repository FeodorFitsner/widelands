/*
 * Copyright (C) 2008-2012 by the Widelands Development Team
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

#ifndef WL_NETWORK_NETHOST_H
#define WL_NETWORK_NETHOST_H

#include "logic/game_controller.h"
#include "logic/game_settings.h"
#include "logic/widelands.h"
#include "network/network.h"

struct ChatMessage;
struct NetHostImpl;
struct Client;

/**
 * NetHost manages the lifetime of a network game in which this computer
 * acts as the host.
 *
 * This includes running the game setup screen and the actual game after
 * launch, as well as dealing with the actual network protocol.
 */
struct NetHost : public GameController, private SyncCallback {
	NetHost (const std::string & playername, bool internet = false);
	virtual ~NetHost ();

	void run(bool autostart = false);
	const std::string & get_local_playername() const;
	int16_t get_local_playerposition();

	// GameController interface
	void think() override;
	void send_player_command(Widelands::PlayerCommand &) override;
	int32_t get_frametime() override;
	GameController::GameType get_game_type() override;

	uint32_t real_speed() override;
	uint32_t desired_speed() override;
	void set_desired_speed(uint32_t speed) override;
	bool is_paused() override;
	void set_paused(bool paused) override;
	// End GameController interface

	// Pregame-related stuff
	const GameSettings & settings();
	bool can_launch();
	void set_scenario(bool);
	void set_map
		(const std::string & mapname,
		 const std::string & mapfilename,
		 uint32_t            maxplayers,
		 bool                savegame = false);
	void set_player_state    (uint8_t number, PlayerSettings::State state, bool host = false);
	void set_player_tribe    (uint8_t number, const std::string & tribe, bool const random_tribe = false);
	void set_player_init     (uint8_t number, uint8_t index);
	void set_player_ai       (uint8_t number, const std::string & name, bool const random_ai = false);
	void set_player_name     (uint8_t number, const std::string & name);
	void set_player          (uint8_t number, PlayerSettings);
	void set_player_number   (uint8_t number);
	void set_player_team     (uint8_t number, Widelands::TeamNumber team);
	void set_player_closeable(uint8_t number, bool closeable);
	void set_player_shared   (uint8_t number, uint8_t shared);
	void switch_to_player    (uint32_t user,  uint8_t number);
	void set_win_condition_script   (std::string);

	// just visible stuff for the select mapmenu
	void set_multiplayer_game_settings();

	// Chat-related stuff
	void send(ChatMessage msg);

	//  Host command related stuff
	int32_t check_client(std::string name);
	void kick_user(uint32_t, std::string);
	void split_command_array
		(const std::string & cmdarray, std::string & cmd, std::string & arg1, std::string & arg2);
	void handle_dserver_command(std::string, std::string);
	void dserver_send_maps_and_saves(Client &);

	void report_result(uint8_t player, Widelands::PlayerEndResult result, const std::string & info) override;

	void force_pause() {
		m_forced_pause = true;
		update_network_speed();
	}

	void end_forced_pause() {
		m_forced_pause = false;
		update_network_speed();
	}

	bool forced_pause() {return m_forced_pause;}

	bool is_dedicated() {return m_is_dedicated;}

private:
	NetTransferFile * file;

	void send_system_message_code
		(const std::string &,
		 const std::string & a = "", const std::string & b = "", const std::string & c = "");
	void request_sync_reports();
	void check_sync_reports();
	void syncreport() override;

	void clear_computer_players();
	void init_computer_player(Widelands::PlayerNumber p);
	void init_computer_players();

	void handle_packet(uint32_t i, RecvPacket &);
	void handle_network ();
	void send_file_part(TCPsocket, uint32_t);

	void check_hung_clients();
	void broadcast_real_speed(uint32_t speed);
	void update_network_speed();

	std::string get_computer_player_name(uint8_t playernum);
	bool has_user_name
		(const std::string & name,
		 uint8_t             ignoreplayer = UserSettings::none());
	void welcome_client(uint32_t number, std::string & playername);
	void committed_network_time(int32_t time);
	void receive_client_time(uint32_t number, int32_t time);

	void broadcast(SendPacket &);
	void write_setting_map(SendPacket &);
	void write_setting_player(SendPacket &, uint8_t number);
	void write_setting_all_players(SendPacket &);
	void write_setting_user(SendPacket &, uint32_t number);
	void write_setting_all_users(SendPacket &);
	bool write_map_transfer_info(SendPacket &, std::string);

	void disconnect_player_controller
		(uint8_t number,
		 const std::string & name);
	void disconnect_client
		(uint32_t number,
		 const std::string & reason,
		 bool sendreason = true,
		 const std::string & arg = "");
	void reaper();

	NetHostImpl * d;
	bool m_internet;
	bool m_is_dedicated;
	std::string m_password;
	std::string m_dedicated_motd;
	bool m_forced_pause;
};


#endif  // end of include guard: WL_NETWORK_NETHOST_H
