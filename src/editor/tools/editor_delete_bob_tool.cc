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

#include "editor_delete_bob_tool.h"
#include "field.h"
#include "map.h"
#include "editorinteractive.h"
#include "editor.h"
#include "bob.h"

/*
=================================================

class Editor_Delete_Bob_Tool

=================================================
*/

/*
===========
Editor_Delete_Bob_Tool::handle_click_impl()

deletes the bob at the given location
===========
*/
int Editor_Delete_Bob_Tool::handle_click_impl
(Map & map, const Node_and_Triangle center, Editor_Interactive & parent)
{
	const int radius = parent.get_sel_radius();
	MapRegion mr(map, center.node, radius);
   FCoords fc;
	while (mr.next(fc)) if (Bob * const bob=fc.field->get_first_bob())
         bob->remove(parent.get_editor());
   return radius + 2;
}
