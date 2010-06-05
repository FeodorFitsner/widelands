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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef WIDELANDS_MAP_MAP_OBJECT_SAVER_H
#define WIDELANDS_MAP_MAP_OBJECT_SAVER_H

#include "logic/widelands.h"
#include "widelands_map_message_saver.h"

#include <map>

#ifndef NDEBUG
#include <string>
#endif

namespace Widelands {

class Map_Object;

/*
 * This class helps to
 *   - keep track of map objects on the map
 *   - translate Map_Object* Pointer into the index used in the saved file
 */
struct Map_Map_Object_Saver {
	Map_Map_Object_Saver();

	bool is_object_known(Map_Object const &) const;
	Serial register_object(Map_Object const &);

	uint32_t get_object_file_index(Map_Object const &);
	uint32_t get_object_file_index_or_zero(Map_Object const *);

	void mark_object_as_saved(Map_Object const &);

	// Information functions
#ifndef NDEBUG
	void     detect_unsaved_objects() const;
#endif
	uint32_t get_nr_roads          () const throw () {return m_nr_roads;}
	uint32_t get_nr_flags          () const throw () {return m_nr_flags;}
	uint32_t get_nr_buildings      () const throw () {return m_nr_buildings;}
	uint32_t get_nr_wares          () const throw () {return m_nr_wares;}
	uint32_t get_nr_bobs           () const throw () {return m_nr_bobs;}
	uint32_t get_nr_immovables     () const throw () {return m_nr_immovables;}
	uint32_t get_nr_battles        () const throw () {return m_nr_battles;}

	bool is_object_saved(Map_Object const &) throw ();

	/// \note Indexed by player number - 1.
	Map_Message_Saver message_savers[MAX_PLAYERS];

private:
	struct MapObjectRec {
#ifndef NDEBUG
		std::string description;
#endif
		uint32_t fileserial;
		bool registered;
		bool saved;
	};
	typedef std::map<const Map_Object *, MapObjectRec> Map_Object_Map;

	MapObjectRec & get_object_record(Map_Object const &);

	Map_Object_Map m_objects;
	uint32_t m_nr_roads;
	uint32_t m_nr_flags;
	uint32_t m_nr_buildings;
	uint32_t m_nr_bobs;
	uint32_t m_nr_wares;
	uint32_t m_nr_immovables;
	uint32_t m_nr_battles;
	uint32_t m_lastserial;
};

}

#endif
