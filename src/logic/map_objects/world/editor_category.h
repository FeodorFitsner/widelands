/*
 * Copyright (C) 2006-2015 by the Widelands Development Team
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

#ifndef WL_LOGIC_MAP_OBJECTS_WORLD_EDITOR_CATEGORY_H
#define WL_LOGIC_MAP_OBJECTS_WORLD_EDITOR_CATEGORY_H

#include <string>

#include "base/macros.h"

class Image;
class LuaTable;

namespace Widelands {

/// Represents a category for grouping items in the Editor, so purely a UI
/// distinction and not a logical one.
class EditorCategory {
public:
	EditorCategory(const LuaTable& table);

	/// Internal name.
	const std::string& name() const;

	/// User facing (translated) name.
	const std::string& descname() const;

	/// The menu image for the category.
	const Image* picture() const;

private:
	const std::string name_;
	const std::string descname_;
	const std::string image_file_;
	DISALLOW_COPY_AND_ASSIGN(EditorCategory);
};

}  // namespace Widelands

#endif  // end of include guard: WL_LOGIC_MAP_OBJECTS_WORLD_EDITOR_CATEGORY_H
