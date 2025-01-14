/*
 * Copyright (C) 2002-2004, 2006-2011 by the Widelands Development Team
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

#include "game_io/game_loader.h"

#include <boost/bind.hpp>
#include <boost/signals2.hpp>

#include "base/log.h"
#include "base/scoped_timer.h"
#include "game_io/game_class_packet.h"
#include "game_io/game_cmd_queue_packet.h"
#include "game_io/game_interactive_player_packet.h"
#include "game_io/game_map_packet.h"
#include "game_io/game_player_ai_persistent_packet.h"
#include "game_io/game_player_economies_packet.h"
#include "game_io/game_player_info_packet.h"
#include "game_io/game_preload_packet.h"
#include "io/filesystem/layered_filesystem.h"
#include "logic/game.h"
#include "logic/player.h"
#include "map_io/map_object_loader.h"

namespace Widelands {

GameLoader::GameLoader(const std::string & path, Game & game) :
	m_fs(*g_fs->make_sub_file_system(path)), m_game(game)
{}


GameLoader::~GameLoader() {
	delete &m_fs;
}

/*
 * This function preloads a game
 */
int32_t GameLoader::preload_game(GamePreloadPacket & mp) {
	// Load elemental data block
	mp.read(m_fs, m_game, nullptr);

	return 0;
}

/*
 * Load the complete file
 */
int32_t GameLoader::load_game(bool const multiplayer) {
	ScopedTimer timer("GameLoader::load() took %ums");

	log("Game: Reading Preload Data ... ");
	{GamePreloadPacket                     p; p.read(m_fs, m_game);}
	log("took %ums\n", timer.ms_since_last_query());

	log("Game: Reading Game Class Data ... ");
	{GameClassPacket                  p; p.read(m_fs, m_game);}
	log("took %ums\n", timer.ms_since_last_query());

	log("Game: Reading Map Data ... ");
	GameMapPacket M;                          M.read(m_fs, m_game);
	log("Game: Reading Map Data took %ums\n", timer.ms_since_last_query());

	log("Game: Reading Player Info ... ");
	{GamePlayerInfoPacket                 p; p.read(m_fs, m_game);}
	log("Game: Reading Player Info took %ums\n", timer.ms_since_last_query());

	log("Game: Calling read_complete()\n");
	M.read_complete(m_game);
	log("Game: read_complete took: %ums\n", timer.ms_since_last_query());

	MapObjectLoader * const mol = M.get_map_object_loader();

	log("Game: Reading Player Economies Info ... ");
	{GamePlayerEconomiesPacket            p; p.read(m_fs, m_game, mol);}
	log("took %ums\n", timer.ms_since_last_query());

	log("Game: Reading ai persistent data ... ");
	{GamePlayerAiPersistentPacket           p; p.read(m_fs, m_game, mol);}
	log("took %ums\n", timer.ms_since_last_query());

	log("Game: Reading Command Queue Data ... ");
	{GameCmdQueuePacket                   p; p.read(m_fs, m_game, mol);}
	log("took %ums\n", timer.ms_since_last_query());

	//  This must be after the command queue has been read.
	log("Game: Parsing messages ... ");
	PlayerNumber const nr_players = m_game.map().get_nrplayers();
	iterate_players_existing_const(p, nr_players, m_game, player) {
		const MessageQueue & messages = player->messages();
		for (std::pair<MessageId, Message *> temp_message : messages) {
			Message* m = temp_message.second;
			MessageId m_id = temp_message.first;

			// Renew MapObject connections
			if (m->serial() > 0) {
				MapObject* mo = m_game.objects().get_object(m->serial());
				mo->removed.connect
					(boost::bind(&Player::message_object_removed, player, m_id));
			}
		}
	}
	log("took %ums\n", timer.ms_since_last_query());

	// For compatibility hacks only
	mol->load_finish_game(m_game);

	// Only read and use interactive player data, if we load a singleplayer game.
	// In multiplayer games every client needs to create a new interactive
	// player.
	if (!multiplayer) {
		log("Game: Reading Interactive Player Data ... ");
		{GameInteractivePlayerPacket       p; p.read(m_fs, m_game, mol);}
		log("took %ums\n", timer.ms_since_last_query());
	}

	return 0;
}

}
