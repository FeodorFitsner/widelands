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

#ifndef __S__EDITOR_INCREASE_HEIGHT_TOOL_H
#define __S__EDITOR_INCREASE_HEIGHT_TOOL_H

#include "editor_decrease_height_tool.h"
#include "editor_set_height_tool.h"

///  Increases the height of a field by a value.
struct Editor_Increase_Height_Tool : public Editor_Tool {
	Editor_Increase_Height_Tool
		(Editor_Decrease_Height_Tool & the_decrease_tool,
		 Editor_Set_Height_Tool      & the_set_tool)
		:
		Editor_Tool(the_decrease_tool, the_set_tool),
		m_decrease_tool(the_decrease_tool), m_set_tool(the_set_tool),
		m_change_by(1)
	{}

	int handle_click_impl(Map &, const Node_and_Triangle<>, Editor_Interactive &);
	const char * get_sel_impl() const throw ()
	{return "pics/fsel_editor_increase_height.png";}

	int get_change_by() const throw () {return m_change_by;}
	void set_change_by(const int n) throw () {m_change_by = n;}

	Editor_Decrease_Height_Tool & decrease_tool() const throw ()
	{return m_decrease_tool;}
	Editor_Set_Height_Tool & set_tool() throw () {return m_set_tool;}

private:
	Editor_Decrease_Height_Tool & m_decrease_tool;
	Editor_Set_Height_Tool      & m_set_tool;
	int                           m_change_by;
};

#endif
