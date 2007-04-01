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



#include "editor_game_base.h"
#include "error.h"
#include "instances.h"
#include "widelands_map_map_object_loader.h"
#include "wexception.h"

/*
 * constructor, destructor
 */
Widelands_Map_Map_Object_Loader::Widelands_Map_Map_Object_Loader(void) {
}

Widelands_Map_Map_Object_Loader::~Widelands_Map_Map_Object_Loader(void) {
}


/*
 * Returns true if this object has already been inserted
 */
bool Widelands_Map_Map_Object_Loader::is_object_known(uint n) {
   Reverse_Map_Object_Map::iterator i;
   i=m_objects.find(n);
   return (i!=m_objects.end());
}

/*
 * Registers this object as a new one
 */
void Widelands_Map_Map_Object_Loader::register_object(Editor_Game_Base* egbase, uint n, Map_Object* obj) {
   assert(!is_object_known(n));

   egbase->objects().overwrite_file_serial(obj, n);

   m_objects.insert(std::pair<uint, Map_Object*>(n, obj));
   m_loaded_obj[obj]=false;
}

/*
 * Returns the file index for this map object. This is used on load
 * to regenerate the depencies between the objects
 */
Map_Object* Widelands_Map_Map_Object_Loader::get_object_by_file_index(uint n) {
   // This check should rather be an assert(), but we get more information
   // from a throw and time's not soo much an issue here
   if(!is_object_known(n))
      throw wexception("Widelands_Map_Map_Object_Loader::get_object_by_file_index(): Map Object %i is not known!\n", n);

   return m_objects[n];
}

/*
 * mark this object as saved
 */
void Widelands_Map_Map_Object_Loader::mark_object_as_loaded(Map_Object* obj) {
   m_loaded_obj[obj]=true;
}

/*
 * Return the number of unsaved objects
 */
int Widelands_Map_Map_Object_Loader::get_nr_unloaded_objects(void) {
   std::map<Map_Object*,bool>::iterator i=m_loaded_obj.begin();
   int retval=0;
   while(i!=m_loaded_obj.end()) {
      if(!i->second) retval++;
      ++i;
   }
   return retval;
}
