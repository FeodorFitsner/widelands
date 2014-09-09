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

#include "map_io/widelands_map_flag_data_packet.h"

#include <map>

#include <boost/format.hpp>

#include "base/macros.h"
#include "economy/flag.h"
#include "io/fileread.h"
#include "io/filewrite.h"
#include "logic/game.h"
#include "logic/map.h"
#include "logic/player.h"
#include "map_io/widelands_map_map_object_loader.h"
#include "map_io/widelands_map_map_object_saver.h"

namespace Widelands {

#define CURRENT_PACKET_VERSION 1


void Map_Flag_Data_Packet::Read
	(FileSystem            &       fs,
	 Editor_Game_Base      &       egbase,
	 bool                    const skip,
	 MapMapObjectLoader &       mol)
{
	if (skip)
		return;

	FileRead fr;
	try {fr.Open(fs, "binary/flag");} catch (...) {return;}

	try {
		uint16_t const packet_version = fr.Unsigned16();
		if (packet_version == CURRENT_PACKET_VERSION) {
			const Map & map = egbase.map();
			Player_Number const nr_players = map.get_nrplayers();
			Widelands::Extent const extent = map.extent();
			iterate_Map_FCoords(map, extent, fc)
				if (fr.Unsigned8()) {
					Player_Number const owner  = fr.Unsigned8();
					if (!(0 < owner && owner <= nr_players)) {
						throw GameDataError("Invalid player number: %i.", owner);
					}

					Serial        const serial = fr.Unsigned32();

					try {
						if (fc.field->get_owned_by() != owner)
							throw GameDataError
								("the node is owned by player %u",
								 fc.field->get_owned_by());

						for (Direction dir = 6; dir; --dir) {
							FCoords n;
							map.get_neighbour(fc, dir, &n);
							try {

							//  Check that the flag owner owns all neighbour nodes (so
							//  that the flag is not on his border). We can not check
							//  the border bit of the node because it has not been set
							//  yet.
							if (n.field->get_owned_by() != owner)
								throw GameDataError
									("is owned by player %u",
									 n.field->get_owned_by());

							//  Check that there is not already a flag on a neighbour
							//  node. We read the map in order and only need to check
							//  the nodes that we have already read.
							if (n.field < fc.field)
								if (upcast(Flag const, nf, n.field->get_immovable()))
									throw GameDataError
										("has a flag (%u)", nf->serial());
							} catch (const WException & e) {
								throw GameDataError
									("neighbour node (%i, %i): %s",
									 n.x, n.y, e.what());
							}
						}

						//  No flag lives on more than one place.

						//  Now, create this Flag. Directly create it, do not call
						//  the player class since we recreate the data in another
						//  packet. We always create this, no matter what skip is
						//  since we have to read the data packets. We delete this
						//  object later again, if it is not wanted.
						mol.register_object<Flag>
							(serial,
							 *new Flag
							 	(ref_cast<Game, Editor_Game_Base>(egbase),
							 	 egbase.player(owner),
							 	 fc));
					} catch (const WException & e) {
						throw GameDataError
							("%u (at (%i, %i), owned by player %u): %s",
							 serial, fc.x, fc.y, owner, e.what());
					}
				}
		} else
			throw GameDataError
				("unknown/unhandled version %u", packet_version);
	} catch (const WException & e) {
		throw GameDataError("flags: %s", e.what());
	}
}


void Map_Flag_Data_Packet::Write
	(FileSystem & fs, Editor_Game_Base & egbase, MapMapObjectSaver & mos)
{
	FileWrite fw;

	fw.Unsigned16(CURRENT_PACKET_VERSION);

	//  Write flags and owner, register this with the map_object_saver so that
	//  it's data can be saved later.
	const Map   & map        = egbase.map();
	const Field & fields_end = map[map.max_index()];
	for (Field const * field = &map[0]; field < &fields_end; ++field)
		//  we only write flags, so the upcast is safe
		if (upcast(Flag const, flag, field->get_immovable())) {
			//  Flags can't life on multiply positions, therefore this flag
			//  shouldn't be registered.
			assert(!mos.is_object_known(*flag));
			assert(field->get_owned_by() == flag->owner().player_number());

			fw.Unsigned8(1);
			fw.Unsigned8(flag->owner().player_number());
			fw.Unsigned32(mos.register_object(*flag));
		} else //  no existence, no owner
			fw.Unsigned8(0);

	fw.Write(fs, "binary/flag");
}

}
