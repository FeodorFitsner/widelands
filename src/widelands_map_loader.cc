/*
 * Copyright (C) 2002-4 by the Widelands Development Team
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

#include "widelands_map_loader.h"
#include "widelands_map_elemental_data_packet.h"
#include "widelands_map_player_names_and_tribes_data_packet.h"
#include "widelands_map_data_packet_ids.h"
#include "widelands_map_data_packet_factory.h"
#include "filesystem.h"
#include "map.h"
#include "world.h"
#include "error.h"

/*
 * Constructor
 */
Widelands_Map_Loader::Widelands_Map_Loader(const char* filename, Map* map) :
   Map_Loader(filename, map) {
      m_map=map;
      m_filename=filename;
}

/*
 * Destructor
 */
Widelands_Map_Loader::~Widelands_Map_Loader(void) {
}

/*
 * This function preloads map so that
 * the map class returns valid data for all 
 * the get_info() functions (_width, _nrplayers..)
 */
int Widelands_Map_Loader::preload_map(bool scenario) {
   assert(get_state()!=STATE_LOADED);
   
   m_map->cleanup();

   // Load elemental data block
   Widelands_Map_Elemental_Data_Packet mp;
   FileRead fr;

   fr.Open(g_fs, m_filename.c_str());
   mp.Pre_Read(&fr, m_map);
   log("Number of players: %i, scenario: %i\n", m_map->get_nrplayers(), scenario);
   
   if(!World::exists_world(m_map->get_world_name())) {
      throw wexception("%s: %s", m_map->get_world_name(), "World doesn't exist!");
   }
  
   log("Number of players1: %i\n", m_map->get_nrplayers());

   if(fr.Unsigned16()!=PACKET_PLAYER_NAM_TRIB) 
      throw wexception("Wrong packet order in map!\n");

   Widelands_Map_Player_Names_And_Tribes_Data_Packet* dp=new Widelands_Map_Player_Names_And_Tribes_Data_Packet();
   log("Number of players2: %i\n", m_map->get_nrplayers());
   if(!scenario) 
      dp->set_scenario_skip(true);
   else 
      dp->set_scenario_skip(false);
   log("Number of players3: %i\n", m_map->get_nrplayers());
   dp->Pre_Read(&fr, m_map);
   delete dp;

   fr.Close();
   set_state(STATE_PRELOADED);

   return 0;
}

/*
 * Load the complete map and make sure that it runs without problems
 */
int Widelands_Map_Loader::load_map_complete(Editor_Game_Base* egbase, bool scenario) {

   // now, load the world, load the rest infos from the map
   m_map->load_world();
   // Postload the world which provides all the immovables found on a map
   m_map->get_world()->postload(egbase);
   m_map->set_size(m_map->m_width, m_map->m_height);
    
   // Load elemental data block (again)
   Widelands_Map_Elemental_Data_Packet mp;
   FileRead fr;

   fr.Open(g_fs, m_filename.c_str());
   mp.Pre_Read(&fr, m_map);

   if(mp.get_version()<=WLMF_VERSION) {
      // ok, now go on and load the rest
      Widelands_Map_Data_Packet_Factory fac;

      ushort id;
      Widelands_Map_Data_Packet* pak; 
      while(!fr.IsEOF()) {
         id=fr.Unsigned16();
         pak=fac.create_correct_packet(id);
         if(!scenario) 
            pak->set_scenario_skip(true);
         pak->Read(&fr, egbase);
         delete pak;
      } 
   }

   m_map->recalc_whole_map();

   set_state(STATE_LOADED);
   
   return 0;
}
