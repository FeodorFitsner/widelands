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

#include <SDL_image.h>
#include "editor_game_base.h"
#include "error.h"
#include "fileread.h"
#include "filesystem.h"
#include "filewrite.h"
#include "graphic_impl.h" // Since we are laying about the path of the pictures
#include "map.h"
#include "profile.h"
#include "widelands_map_extradata_data_packet.h"
#include "widelands_map_data_packet_ids.h"

#define CURRENT_PACKET_VERSION 1

Widelands_Map_Extradata_Data_Packet::~Widelands_Map_Extradata_Data_Packet(void)
{}

/**
 * Read Function
 */
void Widelands_Map_Extradata_Data_Packet::Read(FileSystem* fs, Editor_Game_Base* egbase, bool skip, Widelands_Map_Map_Object_Loader*) throw(_wexception) {
   if( skip )
      return;

   Profile prof;
   try {
      prof.read( "extra_data", 0, fs );
   } catch( ... ) {
      // not found, skip it
      return;
   }
   Section* s = prof.get_section( "global" );

   // read packet version
   int packet_version=s->get_int("packet_version");

   if(packet_version==CURRENT_PACKET_VERSION) {
      // Nothing more. But read all pics
      if( fs->FileExists("pics") && fs->IsDirectory("pics")) {
         filenameset_t pictures;
         fs->FindFiles( "pics", "*", &pictures );
         for(filenameset_t::iterator pname = pictures.begin(); pname != pictures.end(); pname++) {
            if( fs->IsDirectory( (*pname).c_str())) // Might be some dir, maybe CVS
               continue;

            FileRead fr;

         	fr.Open(fs, *pname);
            SDL_Surface* surf = IMG_Load_RW(SDL_RWFromMem(fr.Data(0), fr.GetSize()), 1);
            if (!surf)
               continue; // Illegal pic. Skip it
            Surface* picsurf = new Surface( );
            picsurf->set_sdl_surface( surf );

	    std::string picname = FileSystem::FS_Filename( (*pname).c_str() );
            picname = "map:" + picname;

            uint data = g_gr->get_picture( PicMod_Game, picsurf, picname.c_str());

            // ok, the pic is now known to the game. But when the game is saved, this data has to be
            // regenerated.
            Map::Extradata_Info info;
            info.type = Map::Extradata_Info::PIC;
            info.filename = *pname;
            info.data = (void*)data;
            egbase->get_map()->m_extradatainfos.push_back( info );
         }
      }
      return;
   }
   assert(0); // never here
}


/**
 * Write Function
 */
void Widelands_Map_Extradata_Data_Packet::Write(FileSystem* fs, Editor_Game_Base* egbase, Widelands_Map_Map_Object_Saver*) throw(_wexception) {
   Profile prof;
   Section* s = prof.create_section("global");

   // packet version
   s->set_int("packet_version", CURRENT_PACKET_VERSION);

   // Nothing more. All pics in the dir pic are loaded as pictures
   for( uint i = 0; i < egbase->get_map()->m_extradatainfos.size(); i++) {
      Map::Extradata_Info& edi = egbase->get_map()->m_extradatainfos[i];
      assert( edi.type == Map::Extradata_Info::PIC );

      fs->EnsureDirectoryExists( "pics" );
      FileWrite fw;

      g_gr->save_png( (ulong)edi.data, &fw );

      fw.Write( fs, edi.filename.c_str() );
   }

   // Write out
   prof.write("extra_data", false, fs );
}
