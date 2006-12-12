/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
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

#include <map>
#include <set>
#include "battle.h"
#include "fileread.h"
#include "filewrite.h"
#include "editor_game_base.h"
#include "immovable.h"
#include "militarysite.h"
#include "map.h"
#include "soldier.h"
#include "tribe.h"
#include "world.h"
#include "geometry.h"
#include "attack_controller.h"
#include "widelands_map_data_packet_ids.h"
#include "widelands_map_attack_controller_data_packet.h"
#include "widelands_map_map_object_loader.h"
#include "widelands_map_map_object_saver.h"
#include "error.h"

/* VERSION 1: initial release
*/

#define CURRENT_PACKET_VERSION 1

Widelands_Map_Attack_Controller_Data_Packet::~Widelands_Map_Attack_Controller_Data_Packet(void) {
}

void Widelands_Map_Attack_Controller_Data_Packet::Read
(FileSystem & fs,
 Editor_Game_Base* egbase,
 const bool skip,
 Widelands_Map_Map_Object_Loader * const ol)
throw (_wexception)
{
   if( skip )
      return;

   FileRead fr;
   try {
      fr.Open( fs, "binary/attackcontroller" );
   } catch ( ... ) {
      // not there, so skip
      return ;
   }
   // First packet version
   int packet_version=fr.Unsigned16();

   if(packet_version == CURRENT_PACKET_VERSION) {
      uint nrControllers = fr.Unsigned32();
      for (uint i=0;i<nrControllers;i++) {
         AttackController* ctrl = egbase->create_attack_controller();
         
         uint serial = fr.Unsigned32();
         ol->register_object(egbase, serial, ctrl);
         
         uint flagFilePos = fr.Unsigned32();
         log("Loggi: searching for a flag with serial %i\n",flagFilePos);
         
         Flag* flag = (Flag*) ol->get_object_by_file_index(flagFilePos);
         assert(flag);
         
         ctrl->flag = flag;
         ctrl->attackingPlayer = fr.Unsigned32();
         ctrl->defendingPlayer = fr.Unsigned32();
         ctrl->totallyLaunched = fr.Unsigned32();
         
         uint numBs = fr.Unsigned32();
         
         for (uint j=0;j<numBs;j++) {
            Soldier* soldier = (Soldier*) ol->get_object_by_file_index(fr.Unsigned32());
            assert(soldier);
            
            MilitarySite* origin = (MilitarySite*) ol->get_object_by_file_index(fr.Unsigned32());
            assert(origin);
            
            Coords* battleGround = new Coords(fr.Unsigned32(),fr.Unsigned32());
            
            AttackController::BattleSoldier bs = {
               soldier,
               origin,
               battleGround,
               fr.Unsigned8(),
               fr.Unsigned8(),
               fr.Unsigned8()
            };
            ctrl->involvedSoldiers.push_back(bs);
            soldier->set_attack_ctrl(ctrl);
         }
         
         uint numInMs = fr.Unsigned32();
         for (uint j=0;j<numInMs;j++) {
            MilitarySite* ms = (MilitarySite*) ol->get_object_by_file_index(fr.Unsigned32());
            assert(ms);
            ctrl->involvedMilitarySites.insert(ms);
            ms->set_in_battle(true);
         }
         
      }
      if (fr.Unsigned32() != 0xffffffff)
         throw wexception ("Error in Widelands_Attack_Controller_Data_Packet : Couldn't find 0xffffffff.");
      return; // End of packet, do not run into assert
   }
   else
      throw wexception ("Unkown version of Widelands_Map_Battle_Data_Packet : %d", packet_version);
   assert(0); // never here
}
/*
 * Write Function.
 * This writes ALL the information about battles !
 */
void Widelands_Map_Attack_Controller_Data_Packet::Write
(FileSystem & fs,
 Editor_Game_Base* egbase,
 Widelands_Map_Map_Object_Saver * const os)
throw (_wexception) {
   FileWrite fw;

   // now packet version
   fw.Unsigned16(CURRENT_PACKET_VERSION);
   
   std::vector<uint> serials = egbase->get_attack_controller_serials();
   
   fw.Unsigned32(serials.size());
   
   for (uint i=0;i<serials.size();i++) {
      AttackController* ctrl = (AttackController*)egbase->get_objects()->get_object(serials[i]);
      assert(not os->is_object_known(ctrl));
      
      fw.Unsigned32(os->register_object(ctrl));  // Something like serial ..
      
      //save the flag against which the attack is launched
      assert(os->is_object_known(ctrl->flag));
      uint flagFilePos = os->get_object_file_index(ctrl->flag);
      log("Loggi: WRITING a flag with serial %i for ATTACK CONTROLLER\n",flagFilePos);
      fw.Unsigned32(flagFilePos);
      
      fw.Unsigned32(ctrl->attackingPlayer);
      fw.Unsigned32(ctrl->defendingPlayer);
      fw.Unsigned32(ctrl->totallyLaunched); 
      
      //write battle soldier structure of involved soldiers
      fw.Unsigned32(ctrl->involvedSoldiers.size());

      for (uint j=0;j<ctrl->involvedSoldiers.size();j++) {
         AttackController::BattleSoldier bs = ctrl->involvedSoldiers[j];
         
         assert(os->is_object_known(bs.soldier));
			fw.Unsigned32(os->get_object_file_index(bs.soldier));
			
			assert(os->is_object_known(bs.origin));
			fw.Unsigned32(os->get_object_file_index(bs.origin));
			
			fw.Unsigned32(bs.battleGround->x);
			fw.Unsigned32(bs.battleGround->y);
			
			fw.Unsigned8(bs.attacker);
			fw.Unsigned8(bs.arrived);
			fw.Unsigned8(bs.fighting);
      }
      
      //write involved military sites
      fw.Unsigned32(ctrl->involvedMilitarySites.size());
      for (std::set<Object_Ptr>::iterator it=ctrl->involvedMilitarySites.begin();it!=ctrl->involvedMilitarySites.end();it++) {
         MilitarySite* ms = static_cast<MilitarySite *const>(static_cast<Object_Ptr>(*it).get(egbase));
         assert(os->is_object_known(ms));
         fw.Unsigned32(os->get_object_file_index(ms));
      }
      os->mark_object_as_saved(ctrl);
   }
   
   fw.Unsigned32(0xffffffff);
   fw.Write( fs, "binary/attackcontroller" );
}
