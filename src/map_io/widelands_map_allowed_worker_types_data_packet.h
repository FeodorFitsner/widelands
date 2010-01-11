/*
 * Copyright (C) 2002-2004, 2006-2008, 2010 by the Widelands Development Team
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

#ifndef WIDELANDS_MAP_ALLOWED_WORKER_TYPES_DATA_PACKET_H
#define WIDELANDS_MAP_ALLOWED_WORKER_TYPES_DATA_PACKET_H

#include "widelands_map_data_packet.h"

namespace Widelands {

/// If present, this contains all worker types that are allowed to be build
/// by players. If absent, all worker types are allowed per default.
struct Map_Allowed_Worker_Types_Data_Packet : public Map_Data_Packet {
	virtual void Read
		(FileSystem &,
		 Editor_Game_Base      &,
		 const bool,
		 Map_Map_Object_Loader * = 0)
		throw (_wexception);
	void Write(FileSystem &, Editor_Game_Base &, Map_Map_Object_Saver * = 0)
		throw (_wexception);
};

}

#endif