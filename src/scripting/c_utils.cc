/*
 * Copyright (C) 2006-2010 by the Widelands Development Team
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

#include "c_utils.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>

Widelands::Game & get_game(lua_State * const L) {
	lua_getfield(L, LUA_REGISTRYINDEX, "game");
	Widelands::Game * g = static_cast<Widelands::Game *>(lua_touserdata(L, -1));
	lua_pop(L, 1); // pop this userdata

	return *g;
}

Widelands::Editor_Game_Base & get_egbase(lua_State * const L) {
	lua_getfield(L, LUA_REGISTRYINDEX, "egbase");
	Widelands::Editor_Game_Base * g = static_cast<Widelands::Editor_Game_Base *>
		(lua_touserdata(L, -1));
	lua_pop(L, 1); // pop this userdata

	return *g;
}

Widelands::Map_Map_Object_Loader * get_mol(lua_State * const L) {
	lua_pushstring(L, "mol");
	lua_gettable(L, LUA_REGISTRYINDEX);

	Widelands::Map_Map_Object_Loader * mol =
		static_cast<Widelands::Map_Map_Object_Loader *>(lua_touserdata(L, -1));

	lua_pop(L, 1); // pop this userdata

	return mol;
}

Widelands::Map_Map_Object_Saver * get_mos(lua_State * const L) {
	lua_pushstring(L, "mos");
	lua_gettable(L, LUA_REGISTRYINDEX);

	Widelands::Map_Map_Object_Saver * mos =
		static_cast<Widelands::Map_Map_Object_Saver *>(lua_touserdata(L, -1));

	lua_pop(L, 1); // pop this userdata

	return mos;
}



/*
 * Returns an error to lua. Returns 0
 */
int report_error(lua_State * L, const char * const fmt, ...) {
	char buffer[2048];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);


	// Also create a traceback
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushstring(L, buffer); // error message
	lua_pushinteger(L, 2);  /* skip this function and traceback */
	lua_call(L, 2, 1);  /* call debug.traceback */

	lua_error(L);

	return 0;
}

