/*
 * Copyright (C) 2002-2004, 2006-2008 by the Widelands Development Team
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

#include "map_io/map_object_saver.h"

#include "base/wexception.h"
#include "economy/flag.h"
#include "economy/fleet.h"
#include "economy/portdock.h"
#include "economy/road.h"
#include "economy/ware_instance.h"
#include "logic/map_objects/bob.h"
#include "logic/map_objects/tribes/battle.h"
#include "logic/map_objects/tribes/building.h"
#include "logic/map_objects/tribes/ware_descr.h"

namespace Widelands {

MapObjectSaver::MapObjectSaver() :
m_nr_roads     (0),
m_nr_flags     (0),
m_nr_buildings (0),
m_nr_bobs      (0),
m_nr_wares     (0),
m_nr_immovables(0),
m_nr_battles   (0),
m_nr_fleets    (0),
m_nr_portdocks (0),
m_lastserial   (0)
{}


/**
 * Return a pointer to the record for the given object.
 * Create a record if that hasn't been done yet.
 */
MapObjectSaver::MapObjectRec &
MapObjectSaver::get_object_record(const MapObject & obj)
{
	MapObjectRecordMap::iterator it = m_objects.find(&obj);

	if (it != m_objects.end())
		return it->second;

	MapObjectRec rec;
#ifndef NDEBUG
	rec.description = to_string(obj.descr().type());
	rec.description += " (";
	rec.description += obj.serial();
	rec.description += ')';
#endif
	rec.fileserial = ++m_lastserial;
	rec.registered = false;
	rec.saved = false;
	return
		m_objects.insert(std::pair<MapObject const *, MapObjectRec>(&obj, rec))
		.first->second;
}


/**
 * Returns true if this object has already been registered.
 * \deprecated since get_object_file_index supports unregistered objects now
 */
bool MapObjectSaver::is_object_known(const MapObject & obj) const
{
	MapObjectRecordMap::const_iterator it = m_objects.find(&obj);

	if (it == m_objects.end())
		return false;

	return it->second.registered;
}

bool MapObjectSaver::is_object_saved(const MapObject & obj)
{
	return get_object_record(obj).saved;
}


/*
 * Registers this object as a new one
 */
Serial MapObjectSaver::register_object(const MapObject & obj) {
	MapObjectRec & rec = get_object_record(obj);

	assert(!rec.registered);

	if      (dynamic_cast<Flag         const *>(&obj)) ++m_nr_flags;
	else if (dynamic_cast<Road         const *>(&obj)) ++m_nr_roads;
	else if (dynamic_cast<Building     const *>(&obj)) ++m_nr_buildings;
	else if (dynamic_cast<Immovable    const *>(&obj)) ++m_nr_immovables;
	else if (dynamic_cast<WareInstance const *>(&obj)) ++m_nr_wares;
	else if (dynamic_cast<Bob          const *>(&obj)) ++m_nr_bobs;
	else if (dynamic_cast<Battle       const *>(&obj)) ++m_nr_battles;
	else if (dynamic_cast<Fleet        const *>(&obj)) ++m_nr_fleets;
	else if (dynamic_cast<PortDock     const *>(&obj)) ++m_nr_portdocks;
	else
		throw wexception("MapObjectSaver: Unknown MapObject type");

	rec.registered = true;
	return rec.fileserial;
}

/**
 * Returns the file index for this map object. This is used on load
 * to regenerate the dependencies between the objects.
 */
uint32_t MapObjectSaver::get_object_file_index(const MapObject & obj)
{
	return get_object_record(obj).fileserial;
}

/**
 * Returns the file index of the given object, or zero for null pointers.
 */
uint32_t MapObjectSaver::get_object_file_index_or_zero
	(const MapObject * obj)
{
	if (obj)
		return get_object_file_index(*obj);
	return 0;
}

/*
 * mark this object as saved
 */
void MapObjectSaver::mark_object_as_saved(const MapObject & obj) {
	MapObjectRec & rec = get_object_record(obj);
	assert(rec.registered);
	rec.saved = true;
}

#ifndef NDEBUG
/*
 * Return the number of unsaved objects
 */
void MapObjectSaver::detect_unsaved_objects() const {
	for (const std::pair<const MapObject *, MapObjectRec>& temp_map : m_objects) {
		if (!temp_map.second.saved) {
			throw wexception
				("%s has not been saved", temp_map.second.description.c_str());
		}
	}
}
#endif

}
