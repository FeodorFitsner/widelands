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

#include "editor_delete_immovable_tool.h"

#include "logic/field.h"
#include "editor/editorinteractive.h"
#include "logic/immovable.h"
#include "logic/mapregion.h"

#include "upcast.h"

/**
 * Deletes the immovable at the given location
*/
int32_t Editor_Delete_Immovable_Tool::handle_click_impl
	(Widelands::Map               &       map,
	 Widelands::Node_and_Triangle<> const center,
	 Editor_Interactive           &       parent)
{
	Widelands::Editor_Game_Base & egbase = parent.egbase();
	Widelands::MapRegion<Widelands::Area<Widelands::FCoords> > mr
		(map,
		 Widelands::Area<Widelands::FCoords>
		 	(map.get_fcoords(center.node), parent.get_sel_radius()));
	do if
		(upcast
		 	(Widelands::Immovable,
		 	 immovable,
		 	 mr.location().field->get_immovable()))
		immovable->remove(egbase); //  Delete no buildings or stuff.
	while (mr.advance(map));
	return mr.radius() + 2;
}
