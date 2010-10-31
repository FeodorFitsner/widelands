/*
 * Copyright (C) 2007-2008, 2010 by the Widelands Development Team
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

#include "widelands_map_object_packet.h"

#include "logic/battle.h"
#include "logic/critter_bob.h"
#include "logic/editor_game_base.h"
#include "logic/immovable.h"
#include "logic/legacy.h"
#include "logic/map.h"
#include "logic/ship.h"
#include "wexception.h"
#include "logic/widelands_fileread.h"
#include "logic/widelands_filewrite.h"
#include "logic/worker.h"
#include "widelands_map_map_object_loader.h"
#include "widelands_map_map_object_saver.h"

namespace Widelands {

#define CURRENT_PACKET_VERSION 1


Map_Object_Packet::~Map_Object_Packet() {
	while (loaders.size()) {
		delete *loaders.begin();
		loaders.erase(loaders.begin());
	}
}


void Map_Object_Packet::Read
	(FileSystem & fs, Editor_Game_Base & egbase, Map_Map_Object_Loader & mol)
{
	try {
		FileRead fr;
		fr.Open(fs, "binary/mapobjects");

		const uint8_t packet_version = fr.Unsigned8();
		if (packet_version != CURRENT_PACKET_VERSION)
			throw game_data_error
				(_("unknown/unhandled version %u"), packet_version);

		// Initial loading stage
		for (;;)
			switch (uint8_t const header = fr.Unsigned8()) {
			case 0:
				return;
			case Map_Object::header_Immovable:
				loaders.insert(Immovable::load(egbase, mol, fr));
				break;

			case Map_Object::header_Legacy_AttackController:
				loaders.insert(Legacy::loadAttackController(egbase, mol, fr));
				break;

			case Map_Object::header_Legacy_Battle:
				loaders.insert(Legacy::loadBattle(egbase, mol, fr));
				break;

			case Map_Object::header_Battle:
				loaders.insert(Battle::load(egbase, mol, fr));
				break;

			case Map_Object::header_Critter:
				loaders.insert(Critter_Bob::load(egbase, mol, fr));
				break;

			case Map_Object::header_Worker:
				loaders.insert(Worker::load(egbase, mol, fr));
				break;

			case Map_Object::header_WareInstance:
				loaders.insert(WareInstance::load(egbase, mol, fr));
				break;

			case Map_Object::header_Ship:
				loaders.insert(Ship::load(egbase, mol, fr));
				break;

			default:
				throw game_data_error(_("unknown object header %u"), header);
			}
	} catch (const std::exception & e) {
		throw game_data_error(_("map objects: %s"), e.what());
	}
}


void Map_Object_Packet::LoadFinish() {
	// load_pointer stage
	container_iterate_const(LoaderSet, loaders, i) {
		(*i.current)->load_pointers();
	}

	// load_finish stage
	container_iterate_const(LoaderSet, loaders, i) {
		(*i.current)->load_finish();
		(*i.current)->mol().mark_object_as_loaded(*(*i.current)->get_object());
	}
}


void Map_Object_Packet::Write
	(FileSystem & fs, Editor_Game_Base & egbase, Map_Map_Object_Saver & mos)
{
	FileWrite fw;

	fw.Unsigned8(CURRENT_PACKET_VERSION);

	Object_Manager::objmap_t const & objs = egbase.objects().get_objects();
	for
		(Object_Manager::objmap_t::const_iterator cit = objs.begin();
		 cit != objs.end();
		 ++cit)
	{
		Map_Object & obj = *cit->second;

		// These checks can be eliminated and the object saver simplified
		// once all Map_Objects are saved using the new system
		if (mos.is_object_known(obj))
			continue;

		if (!obj.has_new_save_support())
			throw game_data_error
				("MO(%u of type %s) without new style save support not saved "
				 "explicitly",
				 obj.serial(), obj.descr().descname().c_str());

		mos.register_object(obj);
		obj.save(egbase, mos, fw);
		mos.mark_object_as_saved(obj);
	}

	fw.Unsigned8(0);

	fw.Write(fs, "binary/mapobjects");
}

}
