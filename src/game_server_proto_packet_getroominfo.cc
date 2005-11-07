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

#include <string>
#include "error.h"
#include "game_server_connection.h"
#include "game_server_proto.h"
#include "game_server_proto_packet_getroominfo.h"
#include "unicode.h"
#include "util.h"
#include "wexception.h"

#include "ui/ui_basic/ui_object.h" //just for i18n

/*
 * Constructor
 */
Game_Server_Protocol_Packet_GetRoomInfo::Game_Server_Protocol_Packet_GetRoomInfo( std::string room ) {
   m_roomname = room;
}

/*
 * Destructor
 */
Game_Server_Protocol_Packet_GetRoomInfo::~Game_Server_Protocol_Packet_GetRoomInfo( void ) {
}

/*
 * Get this packets id
 */
ushort Game_Server_Protocol_Packet_GetRoomInfo::get_id(void) {
   return GGSPP_GETROOMINFO;
}

/*
 * Write To network
 */
void Game_Server_Protocol_Packet_GetRoomInfo::send(Network_Buffer* buffer) {
   buffer->put_string(m_roomname);
}

/*
 * Handle reply
 */
void Game_Server_Protocol_Packet_GetRoomInfo::handle_reply(Game_Server_Connection* gsc, Network_Buffer* buf) {
   uchar flags = buf->get_8();

   if(flags == RI_NONEXISTANT) {
      char buffer[1024];

      snprintf(buffer, 1024, "%s %s %s", _("The Room"), m_roomname.c_str(), _("is currently not logged in or unknown to the server.\n"));

      gsc->server_message( buffer );
      return;
   }
   
   assert(flags == RI_ACK) ;

   ushort nrusers = buf->get_16();

   std::vector< std::string > users;
   
   for(uint i = 0; i < nrusers; i++) {
      users.push_back( buf->get_string() );
   }

   gsc->get_room_info( users );
}

