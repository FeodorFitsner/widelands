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

#ifndef __S__MAP_LOADER_H
#define __S__MAP_LOADER_H

class Map;
class Editor_Game_Base;

/*
=============================

class Map_Loader

This class loads a map from a file. It firsts only loads
small junks of informations like size, nr of players for the
map select dialog. For this loading function the same class Map* can be reused.
Then, when the player has a map selected, the Map is completly filled with
objects and information. When now the player selects another map, this class Map*
must be deleted, a new one must be selected

=============================
*/
class Map_Loader {
   public:
      Map_Loader(const char*, Map*) { m_s=STATE_INIT; m_map=0; }
      virtual ~Map_Loader() { };

      virtual int preload_map(bool as_scenario)=0;
      virtual int load_map_complete(Editor_Game_Base*, bool as_scenario)=0;

      inline Map* get_map() { return m_map; }

   protected:
      enum State {
         STATE_INIT,
         STATE_PRELOADED,
         STATE_LOADED
      };
      void set_state(State s) { m_s=s; }
      State get_state(void) { return m_s; }
      Map* m_map;

   private:
      State m_s;
};

#endif
