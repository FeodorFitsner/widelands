// Widelands server for GGZ
// Copyright (C) 2004 Josef Spillner <josef@ggzgamingzone.org>
// Copyright (C) 2009-2011 The Widelands Development Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

// Header file
#include "widelands_server.h"
#include "log.h"

// Widelands includes

#include "protocol_handler.h"
#include "statistics_handler.h"
#include "wlggz_exception.h"
#include "widelands_player.h"
#include "widelands_client.h"

// GGZ includes
#include <ggz.h>
#include <ggzdmod.h>

// System includes
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
#include <ctime>

#ifndef WLGGZ_GAME_FILES_DIRECTORY
#define WLGGZ_GAME_FILES_DIRECTORY "/var/ggzd/gamedata/Widelands"
#endif

ProtocolHandler wlproto;
StatisticsHandler wlstat;

ProtocolHandler & WidelandsServer::proto_handler()
{
	return wlproto;
}

StatisticsHandler & WidelandsServer::stat_handler()
{
	return wlstat;
}


// Constructor: inherit from ggzgameserver
WidelandsServer::WidelandsServer():
	GGZGameServer(),
	m_wlserver_ip(NULL),
	m_reported(false)
{
	wllog(DL_INFO, "");
	wllog(DL_INFO, "launched!");
}

// Destructor
WidelandsServer::~WidelandsServer()
{
	wllog(DL_INFO, "closing!");
	wllog
		(DL_DUMP, "Map %s (%i, %i)", wlstat.map().name().c_str(),
		 wlstat.map().w(), wlstat.map().h());
	wllog(DL_DUMP, "GameTime: %i", wlstat.game_end_time());

	std::string wincond = "unknown (ERROR)";
	switch (wlstat.map().gametype())
	{
		case gametype_endless:
			wincond = "endless";
			break;
		case gametype_defeatall:
			wincond = "defeat all";
			break;
		case gametype_collectors:
			wincond = "collectors";
			break;
		case gametype_tribes_together:
			wincond = "tribes together (deprecated)";
			break;
		case gametype_territorial_lord:
			wincond = "territorial lord";
			break;
		case gametype_wood_gnome:
			wincond = "wood gnome";
			break;
	}
	wllog(DL_DUMP, "Win Condition: %s", wincond.c_str());
	std::map<std::string, WidelandsClient *>::iterator it = m_clients.begin();
	while (it != m_clients.end())
	{
		wllog
			(DL_DUMP, "Client (ggz: %i, wl: %i) %s",
			 it->second->ggz_number(), it->second->wl_player_number(),
			 it->first.c_str());

		if (it->second->ggz_number() >= 0)
			stat_handler().dump_player(it->second->ggz_number());
		it++;
	}

	wllog(DL_DUMP, "");

	if (m_wlserver_ip)
		ggz_free(m_wlserver_ip);
}


// State change hook. Just to monitor state changes
void WidelandsServer::stateEvent()
{
	std::string statestr = "unkown state";
	switch (state())
	{
		case created:
			statestr = "created";
			break;
		case waiting:
			statestr = "waiting";
			break;
		case playing:
			statestr = "playing";
			break;
		case done:
			statestr = "done";
			break;
		case restored:
			statestr = "restored";
			break;
	}
	wllog(DL_DEBUG, "stateEvent: %s", statestr.c_str());
}

// Player join hook
void WidelandsServer::joinEvent(Client * const client)
{
	if (not client) {
		wllog(DL_INFO, "joinEvent without client ?!?\n");
		return;
	}

	wllog(DL_INFO, "joinEvent: %s", client->name.c_str());
	wlproto.send_greeter(client);

	m_clients[client->name] = new WidelandsClient(client->name, client->number);

	if (not m_wlserver_ip and client->number == 0)
	{
		WidelandsClient & player = *get_client_by_name(client->name, true);
		m_host_username = client->name;
		// This is the host
		// Take peer ip as host ip
		char * ip;

		//  Do not use IP provided by client. Instead, determine peer IP address.
		socklen_t addrsize = 256;
		struct sockaddr * const addr = static_cast<struct sockaddr *>(malloc(addrsize));
		int const ret = getpeername(client->fd, addr, &addrsize);

		//  FIXME: IPv4 compatibility?
		if (addr->sa_family == AF_INET6) {
			ip = static_cast<char *>(ggz_malloc(INET6_ADDRSTRLEN));
			inet_ntop
				(AF_INET6,
				 static_cast<void *>
				 	(&(reinterpret_cast<struct sockaddr_in6 *>(addr))->sin6_addr),
				 ip,
				 INET6_ADDRSTRLEN);
		} else if (addr->sa_family == AF_INET) {
			ip = static_cast<char *>(ggz_malloc(INET_ADDRSTRLEN));
			inet_ntop
				(AF_INET,
				 static_cast<void *>(&(reinterpret_cast<struct sockaddr_in *>(addr))->sin_addr),
				 ip,
				 INET_ADDRSTRLEN);
		} else {
			ip = NULL;
			wllog(DL_ERROR, "GAME: unreachable -> done!");
			ggz_write_int(client->fd, op_unreachable);
			set_state_done();
			return;
		}

		m_wlserver_ip = ggz_strdup(ip);
		ggz_free(ip);

		{
			// test for connectablity ???
			// This code only tests if the string m_wlserver_ip is usable.
			addrinfo * ai = 0;
			if (getaddrinfo(m_wlserver_ip, "7396", 0, &ai)) {
				wllog(DL_ERROR, "GAME: unreachable -> done!");
				ggz_write_int(client->fd, op_unreachable);
				changeState(GGZGameServer::done);
				return;
			}
			freeaddrinfo(ai);
		}

		wllog(DL_INFO, "GAME: reachable -> waiting!");
		changeState(GGZGameServer::waiting);
	}
	wllog
		(DL_DUMP, "Player %i \"%s\" (%s)", client->number,
		 client->name.c_str(), (client->spectator?"spectator":"player"));
	dump_clients();
}

void WidelandsServer::dump_clients()
{
	std::map<std::string, WidelandsClient *>::iterator it =
		m_clients.begin();
	while (it != m_clients.end()) {
		WidelandsClient & c = *it->second;
		if (&c)
			wllog
				(DL_DUMP, "Client \"%s\"(%i): wl-num: %i %s (%s)",
				 it->first.c_str(), c.ggz_number(), c.wl_player_number(),
				 c.support_build16_proto()?">b16proto":"b15proto",
				 is_host(&c)?"host":"client");
		else
			wllog(DL_DUMP, "Client \"%s\": no client structure", it->first.c_str());
		it++;
	}
}


void WidelandsServer::dataEvent(Client * client)
{
	if (not client) {
		wllog(DL_INFO, "dataEvent without client ?!?\n");
		return;
	}
	wlproto.process_data(client);
	if (stat_handler().have_stats())
		game_done();
}


// Player leave event
void WidelandsServer::leaveEvent(Client * client)
{
	if (not client) {
		wllog(DL_INFO, "leaveEvent without client ?!?\n");
		return;
	}

	bool host = false;
	WidelandsClient * plr = get_client_by_name(client->name);
	if (client->name == m_host_username) {
		send_debug = -1;
		host = true;
	}
	if (state() == GGZGameServer::waiting) {
		std::map<std::string, WidelandsClient *>::iterator it =
			m_clients.find(client->name);
		if (it != m_clients.end())
			m_clients.erase(it);
		else
			it++;
	}
	wllog
		(DL_INFO, "leaveEvent \"%s\" (%i), fd=%i",
		 client->name.c_str(), client->number, client->fd);
	dump_clients();
	if (host)
		set_state_done();
}

// Spectator join event (ignored)
void WidelandsServer::spectatorJoinEvent(Client * client)
{
	if (not client) {
		wllog(DL_INFO, "spectatorJoinEvent without client ?!?\n");
		return;
	}
	m_clients[client->name] = new WidelandsClient(client->name, client->number);
	wllog
		(DL_INFO, "spectatorJoinEvent \"%s\"", client->name.c_str());
	dump_clients();
}

// Spectator leave event (ignored)
void WidelandsServer::spectatorLeaveEvent(Client * client)
{
	if (not client) {
		wllog(DL_INFO, "spectatorLeaveEvent without client ?!?\n");
		return;
	}

	if (client->name == m_host_username) {
		send_debug = -1;
		set_state_done();
	}
	assert(m_clients[client->name]);
	delete m_clients[client->name];
	m_clients[client->name] = NULL;
	dump_clients();
}

// Spectator data event (ignored)
void WidelandsServer::spectatorDataEvent(Client * client)
{
	if (not client) {
		wllog(DL_INFO, "spectatorDataEvent without client ?!?\n");
		return;
	}
	wllog(DL_INFO, "spectatorDataEvent  \"%s\"", client->name.c_str());
}

// Error handling event
void WidelandsServer::errorEvent()
{
	wllog(DL_ERROR, "errorEvent");
}

void WidelandsServer::set_state_done()
{
	if (wlstat.have_stats())
		game_done();
	changeState(GGZGameServer::done);
}

void WidelandsServer::seatEvent(Seat * seat)
{
	if (not seat) {
		wllog(DL_FATAL, "seatEvent but seat is NULL");
		return;
	}
	if (not seat->client) {
		wllog(DL_FATAL, "seatEvent but seat->client is NULL");
		return;
	}

	wllog(DL_DEBUG, "seatEvent for %s(%d) fd=%i ", seat->client->name.c_str(), seat->number, seat->client->fd);

	/*if (seat->client->name.length()) {
		if (m_clients[seat->client->name])
			delete m_clients[seat->client->name];
		m_clients[seat->client->name] = new WidelandsClient(seat->client->name, seat->client->number);
	} else {
		std::map<std::string,WidelandsClient *>::iterator it = m_clients.begin();
		while (it != m_clients.end())
			if (it->second->ggz_number() == seat->client->number) {
				delete it->second;
				m_clients.erase(it);
			} else
				it++;
	}
*/
	dump_clients();
}

void WidelandsServer::spectatorEvent(Spectator * spectator)
{
	if (not spectator) {
		wllog(DL_FATAL, "spectatorEvent but spectator is NULL");
		return;
	}
	if (not spectator->client) {
		wllog(DL_FATAL, "spectatorEvent but spectator->client is NULL");
		return;
	}

	wllog
		(DL_DEBUG, "spectatorEvent for %s(%i) fd=%i ", spectator->client->name.c_str(),
		 spectator->number, spectator->client->fd);
/*
	if (spectator->client->name.length()) {
		wllog(DL_DEBUG, "Player \"%s\" stand up", spectator->client->name.c_str());
		if (m_clients[spectator->client->name])
			wllog
				(DL_FATAL, "spectator SeatEvent dor \"%s\" by client still in client list",
				 spectator->client->name.c_str());
	}
	else
		wllog(DL_DEBUG, "Unknown player stand up");
*/
	dump_clients();
}


void WidelandsServer::game_done()
{

	GGZGameResult results[g_wls->players()];
	int score[players()], teams[players()];
	for (int p = 0; p < players(); p++) {
		teams[p] = -1;
		results[p] = GGZ_GAME_NONE;
		score[p] = 0;
	}

	if (m_reported) {
		wllog(DL_INFO, "Game already reported. Do not report again");
		return;
	}

	stat_handler().evaluate();

	std::time_t curtime = time(0);

	std::string gamefile(WLGGZ_GAME_FILES_DIRECTORY);
	if (*(gamefile.end()) != '/')
		gamefile += "/";
	std::time_t t = std::time(0);
	gamefile += asctime(localtime(&curtime));
	gamefile = gamefile.substr(0, gamefile.length() - 1);
	gamefile += ".wlgame";

	std::fstream mfile;
	mfile.open(gamefile.c_str(), std::ios::in);
	if (mfile) {
		wllog
			(DL_WARN,
			 "File %s already exists. Do not write anything.", gamefile.c_str());
		mfile.close();
	} else {
		mfile.close();
		wllog(DL_INFO, "Writing game information to %s", gamefile.c_str());
		mfile.open(gamefile.c_str(), std::ios::out);
		if (not mfile)
			wllog(DL_INFO, "failed to open %s", gamefile.c_str());
	}

	if (mfile)
	{
		mfile << "Widelands game on " <<
			asctime(localtime(&curtime)) << std::endl;
		mfile << "Map: " << wlstat.map().name() << std::endl;
		mfile << "Map size: " << wlstat.map().w() << "x" << wlstat.map().h();
		mfile << std::endl;
		mfile << "Duration: " << (wlstat.game_end_time() / 1000) << std::endl;

		std::string wincond = "unknown (ERROR)";
		switch (wlstat.map().gametype())
		{
			case gametype_endless:
				wincond = "endless";
				break;
			case gametype_defeatall:
				wincond = "defeat all";
				break;
			case gametype_collectors:
				wincond = "collectors";
				break;
			case gametype_tribes_together:
				wincond = "tribes together (deprecated)";
				break;
			case gametype_territorial_lord:
				wincond = "territorial lord";
				break;
			case gametype_wood_gnome:
				wincond = "wood gnome";
				break;
		}

		mfile << "Win condition: " << wincond << " (" << wlstat.map().gametype() << ")" << std::endl;

		mfile << std::endl <<
			"#################################################################" <<
			std::endl << "Players:" << std::endl << std::endl;

		for (int i = 0; i <= stat_handler().highest_playernum(); i++)
		{
			WidelandsPlayer & player = *stat_handler().get_player(i);
			if (not &player)
				continue;
			std::string playername = player.name();
			wllog(DL_DEBUG, "Write player to file: %s", playername.c_str());

			mfile << "Player \"" << playername << "\"" << std::endl;
			mfile << "Widelands number: " << player.wlid() << std::endl;
			//mfile << "GGZ seat number: " << player.ggz_player_number() << std::endl;
			/*
			mfile << "Player type: ";
			if (player.is_spectator())
				mfile << "spectator ";
			if (player.is_player_or_bot())
				mfile << "player ";
			if (player.is_host())
				mfile << "host";
			*/
			mfile << std::endl;
			mfile << "Tribe: " << player.tribe() << std::endl;
			mfile << "Team: " << player.team() << std::endl;

			mfile << "Result: ";
			switch (player.result) {
				case gamestatresult_looser:
					mfile << "lost";
					break;
				case gamestatresult_winner:
					mfile << "won";
					break;
				case gamestatresult_leave:
					mfile << "left the game";
					break;
				default:
					mfile << "unknown result";
			}
			mfile << " (" << player.result << ")"<< std::endl;

			mfile << "Points: " << player.points << std::endl;

/*
			mfile << "Version: " << player.version() << std::endl;
			mfile << "Build: " << player.build() << std::endl;
*/

			mfile << std::endl <<
				"(land, buildings, milbuildingslost, civbuildingslost, "
				"buildingsdefeat, milbuildingsconq, economystrength, "
				"militarystrength, workers, wares, productivity, casualties, kills)"
				<< std::endl << std::endl;
			mfile << "Last values:" << std::endl << "(";
			mfile << player.last_stats.land << ", ";
			mfile << player.last_stats.buildings << ", ";
			mfile << player.last_stats.milbuildingslost << ", ";
			mfile << player.last_stats.civbuildingslost << ", ";
			mfile << player.last_stats.buildingsdefeat << ", ";
			mfile << player.last_stats.milbuildingsconq << ", ";
			mfile << player.last_stats.economystrength << ", ";
			mfile << player.last_stats.militarystrength << ", ";
			mfile << player.last_stats.workers << ", ";
			mfile << player.last_stats.wares << ", ";
			mfile << player.last_stats.productivity << ", ";
			mfile << player.last_stats.casualties << ", ";
			mfile << player.last_stats.kills << ")" << std::endl;

			mfile << std::endl << "Statistics (avg, min, max) every five minutes:"
				<< std::endl;

			for (int i = 0; i < player.stats.buildings.size(); i++)
			{
				try {
				mfile << i << ": (";
				mfile << player.stats.land.at(i).avg << " (" <<
					player.stats.land.at(i).min << ", " <<
					player.stats.land.at(i).max << "), ";
				mfile << player.stats.buildings.at(i).avg << " (" <<
					player.stats.buildings.at(i).min << ", " <<
					player.stats.buildings.at(i).max << "), ";
				mfile << player.stats.milbuildingslost.at(i).avg << " (" <<
					player.stats.milbuildingslost.at(i).min << ", " <<
					player.stats.milbuildingslost.at(i).max << "), ";
				mfile << player.stats.civbuildingslost.at(i).avg << " (" <<
					player.stats.civbuildingslost.at(i).min << ", " <<
					player.stats.civbuildingslost.at(i).max << "), ";
				mfile << player.stats.buildingsdefeat.at(i).avg << " (" <<
					player.stats.buildingsdefeat.at(i).min << ", " <<
					player.stats.buildingsdefeat.at(i).max << "), ";
				mfile << player.stats.milbuildingsconq.at(i).avg << " (" <<
					player.stats.milbuildingsconq.at(i).min << ", " <<
					player.stats.milbuildingsconq.at(i).max << "), ";
				mfile << player.stats.economystrength.at(i).avg << " (" <<
					player.stats.economystrength.at(i).min << ", " <<
					player.stats.economystrength.at(i).max << "), ";
				mfile << player.stats.militarystrength.at(i).avg << " (" <<
					player.stats.militarystrength.at(i).min << ", " <<
					player.stats.militarystrength.at(i).max << "), ";
				mfile << player.stats.workers.at(i).avg << " (" <<
					player.stats.workers.at(i).min << ", " <<
					player.stats.workers.at(i).max << "), ";
				mfile << player.stats.wares.at(i).avg << " (" <<
					player.stats.wares.at(i).min << ", " <<
					player.stats.wares.at(i).max << "), ";
				mfile << player.stats.productivity.at(i).avg << " (" <<
					player.stats.productivity.at(i).min << ", " <<
					player.stats.productivity.at(i).max << "), ";
				mfile << player.stats.casualties.at(i).avg << " (" <<
					player.stats.casualties.at(i).min << ", " <<
					player.stats.casualties.at(i).max << "), ";
				mfile << player.stats.kills.at(i).avg << " (" <<
					player.stats.kills.at(i).min << ", " <<
					player.stats.kills.at(i).max << "))" << std::endl;
				} catch (std::exception e) {
					wllog(DL_ERROR, "catched std::exception: %s", e.what());
					mfile << "Error: catched exception" << std::endl;
				}
			}
			mfile << std::endl << std::endl;
		}
		wllog(DL_DEBUG, "Game Information written to file");
		mfile.close();
	}

	if (wlstat.map().gametype() == gametype_defeatall)
	{
		wllog(DL_INFO, "gametype defeat all");
/*
		std::map<std::string, WidelandsPlayer*>::iterator it = g_wls->m_players.begin();
		int number = 0;
		while (it != g_wls->m_players.end())
		{
			wllog
				(DL_DUMP, "Player (ggz: %i, wl: %i, \"%s\", %s) ",
				 it->second->ggz_player_number(), it->second->wl_player_number(),
				 it->first.c_str(), it->second->tribe().c_str());

			if (it->second->ggz_player_number() >= 0)
			{
				number++;
				if (it->second->ggz_player_number() >= players())
					wllog(DL_ERROR, "ERROR: ggz_player_number() >= players()");
				else {
					if (it->second->result == gamestatresult_winner) {
						results[it->second->ggz_player_number()] = GGZ_GAME_WIN;
						score[it->second->ggz_player_number()] = 7;
					}
					else if
						(it->second->result == gamestatresult_looser)
					{
						results[it->second->ggz_player_number()] = GGZ_GAME_LOSS;
						score[it->second->ggz_player_number()] = -3;
					} else
						results[it->second->ggz_player_number()] = GGZ_GAME_FORFEIT;
				}
			}
			it++;
		}
		if (number < 2)
			wllog(DL_INFO, "Less than two ggz players in game. Do not report");
		if (wlstat.game_end_time() < (30 * 60 * 1000))
			wllog(DL_INFO, "Game lasted less than 30 minutes. Do not report");
		// TODO feed teams to reportGame
		if (number > 1 and wlstat.game_end_time() > (30 * 60 * 1000))
			reportGame(NULL, results, score);
		*/
	}
	else if (wlstat.map().gametype() == gametype_tribes_together)
	{
		wllog(DL_WARN, "gametype tribes together. deprecated");
	}
	else if (wlstat.map().gametype() == gametype_collectors)
	{
		wllog(DL_INFO, "gametype collectors");
/*
		std::map<std::string,WidelandsPlayer*>::iterator it =
			g_wls->m_players.begin();
		int number = 0;
		while (it != g_wls->m_players.end())
		{
			wllog(DL_DUMP, "Player (ggz: %i, wl: %i, \"%s\", %s) ",
				it->second->ggz_player_number(), it->second->wl_player_number(),
				it->first.c_str(), it->second->tribe().c_str());

			if (it->second->ggz_player_number() >= 0)
			{
				number++;
				if (it->second->ggz_player_number() >= players())
					wllog(DL_ERROR, "ERROR: ggz_player_number() >= players()");
				else
				{
					if (it->second->result == gamestatresult_winner) {
						results[it->second->ggz_player_number()] = GGZ_GAME_WIN;
						score[it->second->ggz_player_number()] = 1;
					}
					else if
						(it->second->result == gamestatresult_looser)
					{
						results[it->second->ggz_player_number()] = GGZ_GAME_LOSS;
						score[it->second->ggz_player_number()] = -1;
					} else
						results[it->second->ggz_player_number()] = GGZ_GAME_FORFEIT;
				}
			}
			it++;
		}
		if (number < 2)
			wllog(DL_WARN, "Less than two player in game. Do not report");
		if (number > 1)
			reportGame(NULL, results, score);
		*/
	}
	else if (wlstat.map().gametype() == gametype_endless)
		wllog(DL_INFO, "gametype_endless");
	else if (wlstat.map().gametype() == gametype_territorial_lord)
		wllog(DL_WARN, "gametype: territorial lord - no reporting for this one");
	else if (wlstat.map().gametype() == gametype_wood_gnome)
		wllog(DL_WARN, "gametype: wood gnome - no reporting for this one");
	else
		wllog(DL_ERROR, "Error: Unknow gametype! cannot report game");
	m_reported = true;

	set_state_done();
}

void WidelandsServer::check_reports()
{
	bool all_reported = true;
	std::map<std::string, WidelandsClient *>::iterator it = m_clients.begin();
	while (it != m_clients.end())
		if (SUPPORT_B16_PROTOCOL(it->second) and not it->second->reported_game())
			all_reported = false;
	if (all_reported)
		game_done();
}


WidelandsClient * WidelandsServer::get_client_by_name
	(std::string name, bool create)
{
	WidelandsClient * p = m_clients[name];
	if (not p and create)
	{
		wllog(DL_DEBUG, "*****try to create new player structure for %s. This should not happen", name.c_str());
		//p = new WidelandsClient(name);
		//m_clients[name] = p;
/*
		for (int i=0; i < players(); i++)
		{
			Seat * s;
			if ((s = seat(i)) and s->client and s->client->name == name)
			{
				p->set_ggz_player_number(s->client->number);
			}
		}
		if (p->ggz_player_number() < 0)
			wllog(DL_DEBUG, "%s still has no ggz player number", name.c_str());
*/
	}
	return p;
}

/*
WidelandsClient * WidelandsServer::get_client_by_wlid(int id)
{
	if (id < 0)
		return NULL;

	WidelandsPlayer * player = NULL;

	std::map<std::string,WidelandsPlayer*>::iterator it = m_players.begin();

	while (it != m_players.end())
	{
	if (it->second->wl_player_number() == id)
		player = it->second;
	++it;
	}

	return player;

}

WidelandsPlayer * WidelandsServer::get_player_by_ggzid(int id)
{
	Seat * s = seat(id);
	if (not s or not s->client)
		return 0;
	WidelandsPlayer * p = m_players[seat(id)->client->name];
	if (p and p->ggz_player_number() < 0)
		p->set_ggz_player_number(id);
	return p;
}
*/


bool WidelandsServer::is_host(const WidelandsClient * client)
{
	if (not client)
		return false;
	return client->name() == m_host_username;
}

bool WidelandsServer::is_host(const Client * client)
{
	if (not client)
		return false;
	return client->name == m_host_username;
}

#include <cstdarg>
#include <cstdio>
#include <sstream>

/*
* class _wexception implementation
*/
#undef wexception
_wlggzexception::_wlggzexception
	(char const * const file, uint32_t const line, char const * const fmt, ...)
	throw ()
	{
		char buffer[512];
		{
			va_list va;
			va_start(va, fmt);
			vsnprintf(buffer, sizeof(buffer), fmt, va);
			va_end(va);
		}
			std::ostringstream ost;
			ost << '[' << file << ':' << line << "] " << buffer;
			m_what = ost.str();
	}

	_wlggzexception::~_wlggzexception() throw () {}

	char const * _wlggzexception::what() const throw ()
	{
		return m_what.c_str();
	}

