/*
 * Copyright (C) 2002-2004, 2007-2008, 2010-2011 by the Widelands Development Team
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

#ifndef WL_MAP_IO_MAP_OBJECT_LOADER_H
#define WL_MAP_IO_MAP_OBJECT_LOADER_H

#include <map>
#include <typeinfo>

#include <stdint.h>

#include "base/macros.h"
#include "logic/game_data_error.h"
#include "logic/map_objects/map_object.h"
#include "logic/widelands.h"


namespace Widelands {
class Bob;
class MapObject;
class EditorGameBase;

/*
 * This class helps to
 *   - keep track of map objects on the map (to be loaded)
 *   - translate file index pointers into MapObjects
 */
class MapObjectLoader {
public:
	bool is_object_known(uint32_t);

	/// Registers the object as a new one.
	///
	/// \returns a reference to the object.
	/// \throws WException if there is already an object registered with the
	/// same serial. (not implemented: In that case, the object is deleted.)
	///
	// TODO(unknown): Currently the object must be passed as a parameter to this
	// function. This should be changed so that the object is allocated here.
	// The parameter object should then be removed and the function renamed to
	// create_object. Then there will no longer be necessary to delete the
	// object in case the serial number is alrealy known, since the object will
	// never even be allocated then. But this change can only be done when all
	// kinds of map objects have suitable default constructors.
	template<typename T> T & register_object(Serial const n, T & object) {
		ReverseMapObjectMap::const_iterator const existing =
			m_objects.find(n);
		if (existing != m_objects.end()) {
			//delete &object; can not do this
			throw GameDataError("already loaded (%s)", to_string(existing->second->descr().type()).c_str());
		}
		m_objects.insert(std::pair<Serial, MapObject *>(n, &object));
		m_loaded_obj[&object] = false;
		return object;
	}

	template<typename T> T & get(Serial const serial) {
		ReverseMapObjectMap::iterator const it = m_objects.find(serial);
		if (it == m_objects.end())
			throw GameDataError("not found");
		else if (upcast(T, result, it->second))
			return *result;
		else
			throw GameDataError("is a %s, expected a %s",
			                      to_string(it->second->descr().type()).c_str(),
			                      typeid(T).name());
	}

	int32_t get_nr_unloaded_objects();
	bool is_object_loaded(MapObject & obj) {return m_loaded_obj[&obj];}

	void mark_object_as_loaded(MapObject &);

	void schedule_destroy(MapObject &);
	void schedule_act(Bob &);

	void load_finish_game(Game & g);

private:
	using ReverseMapObjectMap = std::map<Serial, MapObject *>;

	std::map<MapObject *, bool> m_loaded_obj;
	ReverseMapObjectMap m_objects;

	std::vector<MapObject *> m_schedule_destroy;
	std::vector<Bob *> m_schedule_act;
};

}

#endif  // end of include guard: WL_MAP_IO_MAP_OBJECT_LOADER_H
