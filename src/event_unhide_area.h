/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
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

#ifndef __S__EVENT_UNHIDE_AREA_H
#define __S__EVENT_UNHIDE_AREA_H

#include "event.h"
#include "geometry.h"

class Editor_Game_Base;

/*
 * This event shows a message box
 */
class Event_Unhide_Area : public Event {
   public:
     Event_Unhide_Area();
      ~Event_Unhide_Area();

      // one liner functions
	const char * get_id() const {return "unhide_area";}

      State run(Game*);
      virtual void reinitialize(Game*);

      // File Functions
	void Write(Section &, const Editor_Game_Base &) const;
      void Read(Section*, Editor_Game_Base*);

      inline void set_coords(Coords pt) { m_pt=pt; }
	Coords get_coords() const {return m_pt;}
	int get_player() const {return m_player;}
      inline void set_player(int i) { m_player=i; }
	int get_area() const {return m_area;}
      inline void set_area(int i) { m_area=i; }

   private:
      Coords m_pt;
      int m_area;
      int m_player;
};



#endif
