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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "map_io/map_road_packet.h"

#include <map>

#include "base/macros.h"
#include "economy/road.h"
#include "io/fileread.h"
#include "io/filewrite.h"
#include "logic/editor_game_base.h"
#include "logic/map.h"
#include "logic/player.h"
#include "map_io/map_object_loader.h"
#include "map_io/map_object_saver.h"

namespace Widelands {

#define CURRENT_PACKET_VERSION 1


void MapRoadPacket::Read
	(FileSystem            &       fs,
	 Editor_Game_Base      &       egbase,
	 bool                    const skip,
	 MapObjectLoader &       mol)
{
	if (skip)
		return;

	FileRead fr;
	try {fr.Open(fs, "binary/road");} catch (...) {return;}

	try {
		uint16_t const packet_version = fr.Unsigned16();
		if (packet_version == CURRENT_PACKET_VERSION) {
			Serial serial;
			while ((serial = fr.Unsigned32()) != 0xffffffff) {
				try {
					//  If this is already known, get it.
					//  Road data is read somewhere else
					mol.register_object(serial, *new Road()).init(egbase);
				} catch (const WException & e) {
					throw GameDataError("%u: %s", serial, e.what());
				}
			}
		} else
			throw GameDataError
				("unknown/unhandled version %u", packet_version);
	} catch (const WException & e) {
		throw GameDataError("road: %s", e.what());
	}
}


void MapRoadPacket::Write
	(FileSystem & fs, Editor_Game_Base & egbase, MapObjectSaver & mos)
{
	FileWrite fw;

	fw.Unsigned16(CURRENT_PACKET_VERSION);

	//  Write roads. Register this with the map_object_saver so that its data
	//  can be saved later.
	const Map & map = egbase.map();
	Field * field = &map[0];
	Field const * const fields_end = field + map.max_index();
	for (; field < fields_end; ++field)
		if (upcast(Road const, road, field->get_immovable())) // only roads
			//  Roads can life on multiple positions.
			if (!mos.is_object_known(*road))
				fw.Unsigned32(mos.register_object(*road));
	fw.Unsigned32(0xffffffff);

	fw.Write(fs, "binary/road");
}

}
