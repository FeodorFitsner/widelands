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

#include "editor_set_resources_tool.h"

#include "editor_increase_resources_tool.h"
#include "editor_decrease_resources_tool.h"
#include "graphic.h"
#include "map.h"
#include "field.h"
#include "editorinteractive.h"
#include "world.h"
#include "overlay_manager.h"

/*
===========
Editor_Set_Resources_Tool::handle_click_impl()

sets the resources of the current to a fixed value
===========
*/
int Editor_Set_Resources_Tool::handle_click_impl
(Map & map, const Node_and_Triangle<> center, Editor_Interactive & parent)
{
	const World & world = map.world();
	Overlay_Manager & overlay_manager = map.overlay_manager();
	MapRegion<Area<FCoords> > mr
		(map,
		 Area<FCoords>(map.get_fcoords(center.node), parent.get_sel_radius()));
	do {
		int res        = mr.location().field->get_resources();
		int amount     = mr.location().field->get_resources_amount();
		int max_amount = world.get_resource(m_cur_res)->get_max_amount();

      amount=m_set_to;
      if(amount<0) amount=0;
      if(amount>max_amount) amount=max_amount;

		if (Editor_Change_Resource_Tool_Callback(mr.location(), &map, m_cur_res))
		{
         // Ok, we're doing something. First remove the current overlays
			int picid = g_gr->get_picture
				(PicMod_Menu,
				 world.get_resource(res)->get_editor_pic
				 (mr.location().field->get_resources_amount()).c_str());
			overlay_manager.remove_overlay(mr.location(), picid);

         if(!amount) {
				mr.location().field->set_resources(0, 0);
				mr.location().field->set_starting_res_amount(0);
			} else {
				mr.location().field->set_resources(m_cur_res,amount);
				mr.location().field->set_starting_res_amount(amount);
            // set new overlay
				picid = g_gr->get_picture
					(PicMod_Menu,
					 world.get_resource(m_cur_res)->get_editor_pic(amount).c_str());
				overlay_manager.register_overlay(mr.location(), picid, 4);
				map.recalc_for_field_area(Area<FCoords>(mr.location(), 0));
			}
		}
	} while (mr.advance(map));
	return mr.radius();
}
