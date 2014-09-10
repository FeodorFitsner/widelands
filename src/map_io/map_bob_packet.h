/*
 * Copyright (C) 2002-2004, 2006, 2008, 2010 by the Widelands Development Team
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

#ifndef WL_MAP_IO_WIDELANDS_MAP_BOB_DATA_PACKET_H
#define WL_MAP_IO_WIDELANDS_MAP_BOB_DATA_PACKET_H

#include "logic/widelands_geometry.h"

class FileRead;
class FileSystem;
class OneWorldLegacyLookupTable;

namespace Widelands {

class Editor_Game_Base;
class MapObjectLoader;

// This data packet contains critters on old maps. These days, the bobs are saved in the map_objects
// packet.
struct MapBobPacket {
	void Read(FileSystem&,
	          Editor_Game_Base&,
				 MapObjectLoader&,
	          const OneWorldLegacyLookupTable& lookup_table);

private:
	void ReadBob(FileRead&,
	             Editor_Game_Base&,
					 MapObjectLoader&,
	             Coords,
	             const OneWorldLegacyLookupTable& lookup_table);
};
}

#endif  // end of include guard: WL_MAP_IO_WIDELANDS_MAP_BOB_DATA_PACKET_H
