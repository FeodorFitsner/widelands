/*
 * Copyright (C) 2012 by the Widelands Development Team
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

#ifndef WL_EDITOR_TOOLS_EDITOR_HISTORY_H
#define WL_EDITOR_TOOLS_EDITOR_HISTORY_H

#include <deque>

#include "editor/tools/editor_draw_tool.h"
#include "editor/tools/editor_tool.h"

class EditorInteractive;
namespace UI {struct Button;}

/**
 * The all actions done with an editor tool are saved on a stack to
 * provide undo / redo functionality.
 * Do all tool action you want to make "undoable" using this class.
 */
struct EditorHistory {
	EditorHistory(UI::Button & undo, UI::Button & redo):
		m_undo_button(undo), m_redo_button(redo) {}

	uint32_t do_action(EditorTool& tool,
							 EditorTool::ToolIndex ind,
	                   Widelands::Map& map,
	                   const Widelands::World& world,
	                   Widelands::NodeAndTriangle<> const center,
							 EditorInteractive& parent,
	                   bool draw = false);
	uint32_t undo_action(const Widelands::World& world);
	uint32_t redo_action(const Widelands::World& world);

private:
	UI::Button & m_undo_button;
	UI::Button & m_redo_button;

	EditorDrawTool m_draw_tool;

	std::deque<EditorToolAction> undo_stack;
	std::deque<EditorToolAction> redo_stack;
};



#endif  // end of include guard: WL_EDITOR_TOOLS_EDITOR_HISTORY_H
