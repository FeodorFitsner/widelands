/*
 * Copyright (C) 2006 by the Widelands Development Team
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


#include "editor_set_terrain_tool.h"
#include "editorinteractive.h"

int Editor_Set_Terrain_Tool::handle_click_impl
(Map & map, const Node_and_Triangle center, Editor_Interactive & parent)
{
	assert(center.triangle.t == TCoords::D or center.triangle.t == TCoords::R);
	const int radius = parent.get_sel_radius();
	if (get_nr_enabled()) {
		int max = 0;
		MapTriangleRegion mr(map, center.triangle, radius);
		TCoords c;
		while (mr.next(c))
			max = std::max(max, map.change_terrain(c, get_random_enabled()));
		return radius + max;
	} else return radius;
}
