/*
 * Copyright (C) 2008-2010 by the Widelands Development Team
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

#ifndef NETHOST_H
#define NETHOST_H

#include "logic/widelands.h"
#include "gamecontroller.h"
#include "gamesettings.h"
#include "network.h"

struct ChatMessage;
struct NetHostImpl;

/**
 * NetHost manages the lifetime of a network game in which this computer
 * acts as the host.
 *
 * This includes running the game setup screen and the actual game after
 * launch, as well as dealing with the actual network protocol.
 */
struct NetHost : public GameController, private SyncCallback {
	NetHost (std::string const & playername, bool ggz = false);
	virtual ~NetHost ();

	void run(bool autostart = false);
	std::string const & getLocalPlayername() const;
	int16_t getLocalPlayerposition();

	// GameController interface
	void think();
	void sendPlayerCommand(Widelands::PlayerCommand &);
	int32_t getFrametime();
	std::string getGameDescription();

	uint32_t realSpeed();
	uint32_t desiredSpeed();
	void setDesiredSpeed(uint32_t speed);
	// End GameController interface

	// Pregame-related stuff
	GameSettings const & settings();
	bool canLaunch();
	void setScenario(bool); //  no scenario for multiplayer
	void setMap
		(std::string const & mapname,
		 std::string const & mapfilename,
		 uint32_t            maxplayers,
		 bool                savegame = false);
	void setPlayerState
		(uint8_t number,
		 PlayerSettings::State state,
		 bool host = false);
	void setPlayerTribe  (uint8_t number, std::string const & tribe);
	void setPlayerInit   (uint8_t number, uint8_t index);
	void setPlayerAI     (uint8_t number, std::string const & name);
	void setPlayerName   (uint8_t number, std::string const & name);
	void setPlayer       (uint8_t number, PlayerSettings);
	void setPlayerNumber (uint8_t number);
	void setPlayerReady  (uint8_t number, bool ready);
	bool getPlayerReady  (uint8_t number);
	void setPlayerTeam   (uint8_t number, Widelands::TeamNumber team);
	void setPlayerPartner(uint8_t number, uint8_t partner);
	void setWinCondition (std::string);

	// just visible stuff for the select mapmenu
	void setMultiplayerGameSettings();

	// Chat-related stuff
	void send(ChatMessage msg);

	// Host command releated stuff
	void kickUser(std::string, std::string);

	void forcePause() {
		m_forced_pause = true;
		updateNetworkSpeed();
	}

	void endForcedPause() {
		m_forced_pause = false;
		updateNetworkSpeed();
	}

	bool forcedPause() {return m_forced_pause;}

private:
	NetTransferFile * file;

	void sendSystemChat(char const * fmt, ...) PRINTF_FORMAT(2, 3);
	void requestSyncReports();
	void checkSyncReports();
	void syncreport();

	void clearComputerPlayers();
	void initComputerPlayer(Widelands::Player_Number p);
	void initComputerPlayers();

	void handle_packet(uint32_t i, RecvPacket &);
	void handle_network ();
	void sendFilePart(TCPsocket, uint32_t);

	void checkHungClients();
	void broadcastRealSpeed(uint32_t speed);
	void updateNetworkSpeed();

	std::string getComputerPlayerName(uint8_t playernum);
	bool haveUserName
		(std::string const & name,
		 uint8_t             ignoreplayer = UserSettings::none());
	void welcomeClient(uint32_t number, std::string const & playername);
	void committedNetworkTime(int32_t time);
	void recvClientTime(uint32_t number, int32_t time);

	void broadcast(SendPacket &);
	void writeSettingMap(SendPacket &);
	void writeSettingPlayer(SendPacket &, uint8_t number);
	void writeSettingAllPlayers(SendPacket &);
	void writeSettingUser(SendPacket &, uint32_t number);
	void writeSettingAllUsers(SendPacket &);
	bool writeMapTransferInfo(SendPacket &, std::string);

	void disconnectPlayer
		(uint8_t number,
		 std::string const & reason,
		 bool sendreason = true);
	void disconnectClient
		(uint32_t number,
		 std::string const & reason,
		 bool sendreason = true);
	void reaper();

	NetHostImpl * d;
	bool use_ggz;
	bool m_forced_pause;
};


#endif
