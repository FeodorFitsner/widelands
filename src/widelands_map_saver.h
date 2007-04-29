/*
 * Copyright (C) 2002-2004, 2006-2007 by the Widelands Development Team
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

#ifndef __S__WIDELANDS_MAP_SAVER_H
#define __S__WIDELANDS_MAP_SAVER_H

#include "wexception.h"

class Editor_Game_Base;
class FileSystem;
class Widelands_Map_Map_Object_Saver;

/*
===========================

This class saves a widelands map into a file system

throws wexecption on failure

NOTE: The widelands map format is mostly binary, but some stuff is
ascii. The binary is done for speeds sake, and the ASCII for easier
debugability

===========================
*/
struct Widelands_Map_Saver {
	Widelands_Map_Saver(FileSystem &, Editor_Game_Base*);
      ~Widelands_Map_Saver(void);

      void save() throw(_wexception) ;
      inline Widelands_Map_Map_Object_Saver* get_map_object_saver(void) { return m_mos; }

   private:
      Editor_Game_Base* m_egbase;
	FileSystem & m_fs;
      Widelands_Map_Map_Object_Saver* m_mos;
};

#endif
