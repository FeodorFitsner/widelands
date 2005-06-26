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

#include "game.h"
#include "game_player_economies_data_packet.h"
#include "player.h"
#include "transport.h"

#define CURRENT_PACKET_VERSION 1

/*
 * Destructor
 */
Game_Player_Economies_Data_Packet::~Game_Player_Economies_Data_Packet(void) {
}

/*
 * Read Function
 */
void Game_Player_Economies_Data_Packet::Read(FileSystem* fs, Game* game, Widelands_Map_Map_Object_Loader*) throw(wexception) {
   FileRead fr;

   fr.Open( fs, "binary/player_economies" );

   // read packet version
   int packet_version=fr.Unsigned16();

   if(packet_version==CURRENT_PACKET_VERSION) {
      // DONE
      Map* map=game->get_map();
      for(uint i=1; i<=game->get_map()->get_nrplayers(); i++) {
         Player* plr=game->get_safe_player(i);
         if(!plr) continue;

         uint nr_economies=fr.Unsigned16();
         assert(nr_economies == plr->m_economies.size());

         std::vector<Economy*> ecos;
         ecos.resize(nr_economies);

         for(uint j=0; j<plr->m_economies.size(); j++) {
            int x=fr.Unsigned16();
            int y=fr.Unsigned16();
            Flag* flag=static_cast<Flag*>(map->get_field(Coords(x,y))->get_immovable());
            assert(flag);
            ecos[j]=flag->get_economy();
         }
         for(uint i=0; i<ecos.size(); i++) { 
            plr->m_economies[i]=ecos[i];
            ecos[i]->balance_requestsupply(); // Issue first balance
         }
      }

      return;
   } else
      throw wexception("Unknown version in Game_Player_Economies_Data_Packet: %i\n", packet_version);
   
   assert(0); // never here
}

/*
 * Write Function
 */
void Game_Player_Economies_Data_Packet::Write(FileSystem* fs, Game* game, Widelands_Map_Map_Object_Saver*) throw(wexception) {
   FileWrite fw;
   
   // Now packet version
   fw.Unsigned16(CURRENT_PACKET_VERSION);

   bool done=false;
   for(uint i=1; i<=game->get_map()->get_nrplayers(); i++) {
      Player* plr=game->get_player(i);
      if(!plr) continue; 
      fw.Unsigned16(plr->m_economies.size());
      for(uint j=0; j<plr->m_economies.size(); j++) {
         done=false;
         // Walk the map so that we find a representant
         Map* map=game->get_map();
         for(ushort y=0; y<map->get_height(); y++) {
            for(ushort x=0; x<map->get_width(); x++) {
               BaseImmovable* imm=map->get_field(Coords(x,y))->get_immovable();
               if(!imm) continue;

               if(imm->get_type()==Map_Object::FLAG) {
                  Flag* flag=static_cast<Flag*>(imm);
                  if(flag->get_economy() == plr->m_economies[j]) {
                     fw.Unsigned16(x);
                     fw.Unsigned16(y);
                     done=true;
                  }
               }
               if(done) break;
            }
            if(done) break;
         }
         if(done) continue;
      }
   }

   fw.Write( fs, "binary/player_economies" );
}
