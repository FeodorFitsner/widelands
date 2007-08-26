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

#include "queue_cmd_factory.h"

#include "cmd_check_eventchain.h"
#include "cmd_incorporate.h"
#include "events/event_chain.h"
#include "queue_cmd_ids.h"
#include "playercommand.h"
#include "instances.h"
#include "transport.h"
#include "wexception.h"


BaseCommand* Queue_Cmd_Factory::create_correct_queue_command(uint id) {
   switch(id) {
      case QUEUE_CMD_BUILD: return new Cmd_Build(); break;
      case QUEUE_CMD_FLAG: return new Cmd_BuildFlag(); break;
      case QUEUE_CMD_BUILDROAD: return new Cmd_BuildRoad(); break;
      case QUEUE_CMD_FLAGACTION: return new Cmd_FlagAction(); break;
      case QUEUE_CMD_STOPBUILDING: return new Cmd_StartStopBuilding(); break;
      case QUEUE_CMD_ENHANCEBUILDING: return new Cmd_EnhanceBuilding(); break;
      case QUEUE_CMD_BULLDOZE: return new Cmd_Bulldoze(); break;
      case QUEUE_CMD_DESTROY_MAPOBJECT: return new Cmd_Destroy_Map_Object(); break;
      case QUEUE_CMD_ACT: return new Cmd_Act(); break;
      case QUEUE_CMD_CHECK_EVENTCHAIN: return new Cmd_CheckEventChain(); break;
      case QUEUE_CMD_INCORPORATE: return new Cmd_Incorporate(); break;
      case QUEUE_CMD_CALL_ECONOMY_BALANCE: return new Cmd_Call_Economy_Balance();
      default: throw wexception("Unknown Queue_Cmd_Id in file: %i\n", id);
	}
   return 0; // Never here
}
