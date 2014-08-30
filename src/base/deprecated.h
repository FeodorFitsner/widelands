/*
 * Copyright (C) 2006-2014 by the Widelands Development Team
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

#ifndef WL_BASE_DEPRECATED_H
#define WL_BASE_DEPRECATED_H

// Code that is still used all over the place, but should not be used anymore.

#include <cassert>

// DEPRECATED: leads to unsafe code. Instead use upcast() or is_a() to check at
// runtime for the type you are expecting.
template<typename Derived, typename Base> Derived & ref_cast(Base & base) {
	assert(dynamic_cast<Derived *>(&base) == static_cast<Derived *>(&base));
	return static_cast<Derived &>(base);
}


#endif  // end of include guard: WL_BASE_DEPRECATED_H