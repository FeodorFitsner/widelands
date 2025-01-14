/*
//  * Copyright (C) 2002-2004, 2006-2010 by the Widelands Development Team
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

#ifndef WL_MAP_IO_MAP_OBJECT_SAVER_H
#define WL_MAP_IO_MAP_OBJECT_SAVER_H

#include <map>
#include <string>

#include "logic/constants.h"
#include "logic/widelands.h"
#include "map_io/map_message_saver.h"

namespace Widelands {

class MapObject;

/*
 * This class helps to
 *   - keep track of map objects on the map
 *   - translate MapObject* Pointer into the index used in the saved file
 */
struct MapObjectSaver {
	MapObjectSaver();

	bool is_object_known(const MapObject &) const;
	Serial register_object(const MapObject &);

	uint32_t get_object_file_index(const MapObject &);
	uint32_t get_object_file_index_or_zero(MapObject const *);

	void mark_object_as_saved(const MapObject &);

	// Information functions
#ifndef NDEBUG
	void     detect_unsaved_objects() const;
#endif
	uint32_t get_nr_roads          () const {return m_nr_roads;}
	uint32_t get_nr_flags          () const {return m_nr_flags;}
	uint32_t get_nr_buildings      () const {return m_nr_buildings;}
	uint32_t get_nr_wares          () const {return m_nr_wares;}
	uint32_t get_nr_bobs           () const {return m_nr_bobs;}
	uint32_t get_nr_immovables     () const {return m_nr_immovables;}
	uint32_t get_nr_battles        () const {return m_nr_battles;}

	bool is_object_saved(const MapObject &);

	/// \note Indexed by player number - 1.
	MapMessageSaver message_savers[MAX_PLAYERS];

private:
	struct MapObjectRec {
#ifndef NDEBUG
		std::string description;
#endif
		uint32_t fileserial;
		bool registered;
		bool saved;
	};
	using MapObjectRecordMap = std::map<const MapObject *, MapObjectRec>;

	MapObjectRec & get_object_record(const MapObject &);

	MapObjectRecordMap m_objects;
	uint32_t m_nr_roads;
	uint32_t m_nr_flags;
	uint32_t m_nr_buildings;
	uint32_t m_nr_bobs;
	uint32_t m_nr_wares;
	uint32_t m_nr_immovables;
	uint32_t m_nr_battles;
	uint32_t m_nr_fleets;
	uint32_t m_nr_portdocks;
	uint32_t m_lastserial;
};

}

#endif  // end of include guard: WL_MAP_IO_MAP_OBJECT_SAVER_H
