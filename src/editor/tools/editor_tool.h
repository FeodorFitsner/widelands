/*
 * Copyright (C) 2002-2004, 2006-2012 by the Widelands Development Team
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

#ifndef WL_EDITOR_TOOLS_EDITOR_TOOL_H
#define WL_EDITOR_TOOLS_EDITOR_TOOL_H

#define MAX_TOOL_AREA 9

#include "base/macros.h"
#include "editor/tools/editor_action_args.h"
#include "graphic/graphic.h"
#include "graphic/image.h"
#include "graphic/image_catalog.h"
#include "logic/widelands_geometry.h"

struct EditorInteractive;
namespace Widelands {
class Map;
class World;
}

/**
 * An editor tool is a tool that can be selected in the editor. Examples are:
 * modify height, place bob, place critter, place building. A Tool only makes
 * one function (like delete_building, place building, modify building are 3
 * tools).
 */
class EditorTool {
public:
	EditorTool(EditorTool & second, EditorTool & third, bool uda = true) :
		m_second(second), m_third(third), undoable(uda)
	{}
	virtual ~EditorTool() {}

	enum ToolIndex {First, Second, Third};
	int32_t handle_click
		(const ToolIndex i,
		Widelands::Map & map, const Widelands::World& world, Widelands::NodeAndTriangle<> const center,
		EditorInteractive & parent, EditorActionArgs & args)
	{
		return
		    (i == First ? *this : i == Second ? m_second : m_third)
		    .handle_click_impl(map, world, center, parent, args);
	}

	int32_t handle_undo
		(const ToolIndex i,
		Widelands::Map & map, const Widelands::World& world, Widelands::NodeAndTriangle<> const center,
		EditorInteractive & parent, EditorActionArgs & args)
	{
		return
		    (i == First ? *this : i == Second ? m_second : m_third)
		    .handle_undo_impl(map, world, center, parent, args);
	}

	ImageCatalog::Key get_sel(const ToolIndex i) {
		return
		    (i == First ? *this : i == Second ? m_second : m_third)
		    .get_sel_impl();
	}

	EditorActionArgs format_args(const ToolIndex i, EditorInteractive & parent) {
		return
		    (i == First ? *this : i == Second ? m_second : m_third)
		    .format_args_impl(parent);
	}

	bool is_unduable() {return undoable;}
	virtual bool has_size_one() const {return false;}
	virtual EditorActionArgs format_args_impl(EditorInteractive & parent) {
		return EditorActionArgs(parent);
	}
	virtual int32_t handle_click_impl(Widelands::Map&,
	                                  const Widelands::World& world,
	                                  Widelands::NodeAndTriangle<>,
	                                  EditorInteractive&,
	                                  EditorActionArgs&) = 0;
	virtual int32_t handle_undo_impl(Widelands::Map&,
	                                 const Widelands::World&,
	                                 Widelands::NodeAndTriangle<>,
	                                 EditorInteractive&,
	                                 EditorActionArgs&) {
		return 0;
	}  // non unduable tools don't need to implement this.
	virtual ImageCatalog::Key get_sel_impl() const = 0;
	virtual bool operates_on_triangles() const {return false;}

protected:
	EditorTool & m_second, & m_third;
	bool undoable;

private:
	DISALLOW_COPY_AND_ASSIGN(EditorTool);
};

#endif  // end of include guard: WL_EDITOR_TOOLS_EDITOR_TOOL_H
