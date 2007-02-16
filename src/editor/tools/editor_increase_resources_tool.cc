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

#include "editor_increase_resources_tool.h"
#include "graphic.h"
#include "map.h"
#include "field.h"
#include "editorinteractive.h"
#include "world.h"
#include "map.h"
#include "error.h"
#include "overlay_manager.h"
#include "worlddata.h"

/*
=============================

class Editor_Increase_Resources_Tool

=============================
*/
int Editor_Change_Resource_Tool_Callback(const TCoords c, void * data, int curres) {
	Map & map = *static_cast<Map * const>(data);
	FCoords f = FCoords(c, map.get_field(c));

   FCoords f1;
   int count=0;

   // This field
	count += f.field->get_terr().resource_value(curres);
	count += f.field->get_terd().resource_value(curres);


   // If one of the neighbours is unpassable, count its resource stronger
   // top left neigbour
   map.get_neighbour(f, Map_Object::WALK_NW, &f1);
	count += f1.field->get_terr().resource_value(curres);
	count += f1.field->get_terd().resource_value(curres);

   // top right neigbour
   map.get_neighbour(f, Map_Object::WALK_NE, &f1);
	count += f1.field->get_terd().resource_value(curres);

   // left neighbour
   map.get_neighbour(f, Map_Object::WALK_W, &f1);
	count += f1.field->get_terr().resource_value(curres);

   if(count<=3)
      return 0;
   else
      return f.field->get_caps();
}

/*
===========
Editor_Increase_Resources_Tool::handle_click_impl()

decrease the resources of the current field by one if
there is not already another resource there.
===========
*/
int Editor_Increase_Resources_Tool::handle_click_impl
(Map & map, const Node_and_Triangle center, Editor_Interactive & parent)
{
	const World & world = map.world();
	Overlay_Manager & overlay_manager = map.overlay_manager();
	MapRegion mr(map, Area(center.node, parent.get_sel_radius()));
	do {
		int res        = mr.location().field->get_resources();
		int amount     = mr.location().field->get_resources_amount();
      int max_amount = map.get_world()->get_resource(m_cur_res)->get_max_amount();

		amount += m_change_by;
      if(amount>max_amount) amount=max_amount;


		if
			((res == m_cur_res or not res)
			 and
			 Editor_Change_Resource_Tool_Callback(mr.location(), &map, m_cur_res))
		{
         // Ok, we're doing something. First remove the current overlays
         uint picid = g_gr->get_picture
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
	         map.recalc_for_field_area(Area(mr.location(), 0));
         }
      }
	} while (mr.advance(map));
	return mr.radius();
}
