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

#include <lua.hpp>

#include "logic/game.h"
#include "i18n.h"

#include "scripting.h"
#include "c_utils.h"

#include "lua_globals.h"


/* RST
Global functions
======================

The following functions are imported into the global namespace
of all scripts that are running inside widelands. They provide convenient
access to other scripts in other locations, localisation features and more.

*/

/*
 * ========================================================================
 *                         MODULE CLASSES
 * ========================================================================
 */


/*
 * ========================================================================
 *                         MODULE FUNCTIONS
 * ========================================================================
 */
/* RST
	.. function:: set_textdomain(domain)

		Sets the textdomain for all further calls to :func:`_`.

		:arg domain: The textdomain
		:type domain: :class:`string`
		:returns: :const:`nil`
*/
static int L_set_textdomain(lua_State * L) {
	luaL_checkstring(L, -1);
	lua_setglobal(L, "__TEXTDOMAIN");
	return 0;
}

/* RST
	.. function:: _(str)

		This peculiar function is used to translate texts in your scenario into
		another language. The function takes a single string, grabs the
		textdomain of your map (which is used the maps name) and returns the
		translated string. Make sure that you part translatable and untranslatable
		stuff:

		.. code-block:: lua

			s = "<p><br>" .. _ "Only this should be translated" .. "<br></p>"

		:arg str: text to translate.
		:type str: :class:`string`
		:returns: :const:`nil`
*/
static int L__(lua_State * L) {
	lua_getglobal(L, "__TEXTDOMAIN");

	if (not lua_isnil(L, -1)) {
		i18n::Textdomain dom(luaL_checkstring(L, -1));
		lua_pushstring(L, i18n::translate(luaL_checkstring(L, 1)));
	} else {
		lua_pushstring(L, i18n::translate(luaL_checkstring(L, 1)));
	}
	return 1;
}

/* RST
	.. function:: use(ns, script)

		Includes the script referenced at the caller location. Use this
		to factor your scripts into smaller parts.

		:arg ns:
			The namespace were the imported script resides. Can be any of
				:const:`maps`
					The script is in the ``scripting/`` directory of the current map.

		:type ns: :class:`string`
		:arg script: The filename of the string without the extension ``.lua``.
		:type script: :class:`string`
		:returns: :const:`nil`
*/
static int L_use(lua_State * L) {
	const char * ns = luaL_checkstring(L, -2);
	const char * script = luaL_checkstring(L, -1);

	// remove our argument so that the executed script gets a clear stack
	lua_pop(L, 2);

	try {
		lua_getfield(L, LUA_REGISTRYINDEX, "lua_interface");
		LuaInterface * lua = static_cast<LuaInterface *>(lua_touserdata(L, -1));
		lua_pop(L, 1); // pop this userdata

		lua->run_script(ns, script);
	} catch (LuaError & e) {
		report_error(L, "%s", e.what());
	}
	return 0;
}

const static struct luaL_reg globals [] = {
	{"set_textdomain", &L_set_textdomain},
	{"use", &L_use},
	{"_", &L__},
	{0, 0}
};

void luaopen_globals(lua_State * L) {
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, 0, globals);
	lua_pop(L, 1);

}

