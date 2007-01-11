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

#include "error.h"
#include "filesystem.h"
#include "game.h"
#include "game_cmd_queue_data_packet.h"
#include "game_computer_player_data_packet.h"
#include "game_data_packet_ids.h"
#include "game_game_class_data_packet.h"
#include "game_loader.h"
#include "game_map_data_packet.h"
#include "game_preload_data_packet.h"
#include "game_interactive_player_data_packet.h"
#include "game_player_economies_data_packet.h"
#include "game_player_info_data_packet.h"
#include "widelands_map_map_object_loader.h"


/*
 * Constructor
 */
Game_Loader::Game_Loader(FileSystem & fs, Game* game) : m_fs(fs), m_game(game)
{}

/*
 * Destructor
 */
Game_Loader::~Game_Loader(void) {
}

/*
 * This function preloads a game
 */
int Game_Loader::preload_game(Game_Preload_Data_Packet* mp) {
   // Load elemental data block
   mp->Read(m_fs, m_game, 0);

   return 0;
}

/*
 * Load the complete file
 */
int Game_Loader::load_game(void) {

   log("Game: Reading Preload Data ... ");
	{Game_Preload_Data_Packet                     p; p.Read(m_fs, m_game, 0);}
   log(" done\n");

   log("Game: Reading Game Class Data ... ");
	{Game_Game_Class_Data_Packet                   p; p.Read(m_fs, m_game, 0);}
   log(" done\n");

   log("Game: Reading Player Info ... ");
	{Game_Player_Info_Data_Packet                  p; p.Read(m_fs, m_game, 0);}
   log(" done\n");

   log("Game: Reading Map Data!\n");
	Game_Map_Data_Packet                           M; M.Read(m_fs, m_game, 0);
   log("Game: Reading Map Data done!\n");

	Widelands_Map_Map_Object_Loader * const mol = M.get_map_object_loader();

   log("Game: Reading Player Economies Info ... ");
	{Game_Player_Economies_Data_Packet             p; p.Read(m_fs, m_game, mol);}
   log(" done\n");

   log("Game: Reading Command Queue Data ... ");
	{Game_Cmd_Queue_Data_Packet                    p; p.Read(m_fs, m_game, mol);}
   log(" done\n");

   log("Game: Reading Interactive Player Data ... ");
	{Game_Interactive_Player_Data_Packet           p; p.Read(m_fs, m_game, mol);}
   log(" done\n");

   log("Game: Reading Computer Player Data ... ");
	{Game_Computer_Player_Data_Packet              p; p.Read(m_fs, m_game, mol);}
   log(" done\n");

   return 0;
}
