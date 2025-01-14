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

#ifndef WL_LOGIC_MAP_OBJECTS_WORLD_RESOURCE_DESCRIPTION_H
#define WL_LOGIC_MAP_OBJECTS_WORLD_RESOURCE_DESCRIPTION_H

#include <string>
#include <vector>

#include "base/macros.h"
#include "logic/widelands.h"

class LuaTable;

namespace Widelands {

class ResourceDescription {
public:
	struct EditorPicture {
		std::string picname;
		int upper_limit;
	};

	ResourceDescription(const LuaTable& table);

	/// Returns the in engine name of this resource.
	const std::string& name() const;

	/// Returns the name of this resource for users. Usually translated.
	const std::string& descname() const;

	/// Returns if this resource is detectable by a geologist.
	bool detectable() const;

	/// Returns the maximum amount that can be in a field for this resource.
	int32_t max_amount() const;

	/// Returns the path to the image that should be used in the editor to
	/// represent an 'amount' of this resource.
	const std::string& get_editor_pic(uint32_t amount) const;

private:
	const std::string name_;
	const std::string descname_;
	const bool detectable_;
	const int32_t max_amount_;
	std::vector<EditorPicture> editor_pictures_;

	DISALLOW_COPY_AND_ASSIGN(ResourceDescription);
};

}  // namespace Widelands

#endif  // end of include guard: WL_LOGIC_MAP_OBJECTS_WORLD_RESOURCE_DESCRIPTION_H
