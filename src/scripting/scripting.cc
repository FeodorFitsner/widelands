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

#include <string>
#include <stdexcept>

#include "log.h"

#include "persistence.h"
#include "lua_debug.h"
#include "lua_game.h"
#include "lua_map.h"
#include "lua_globals.h"
#include "coroutine_impl.h"
#include "c_utils.h"

#include "scripting.h"

// TODO: document this whole scripting stuff: changes in pluto, pickling,
// TODO:   unpickling, Luna & Luna changes, mos & mol pickling
// TODO: make pickling independet of OS by using widelands Stream*

// TODO: get rid of LuaCmd. Only LuaFunction should be kept alife
// TODO: add support for gettext in lua
// TODO: add a lua-xgettext like procedure for lua

/*
============================================
       Lua Interface
============================================
*/
class LuaInterface_Impl : public LuaInterface {
	std::string m_last_error;
	std::map<std::string, ScriptContainer> m_scripts;
	lua_State * m_L;

	/*
	 * Private functions
	 */
	private:
		int m_check_for_errors(int);

	public:
		LuaInterface_Impl(Widelands::Editor_Game_Base *);
		virtual ~LuaInterface_Impl();

		virtual void interpret_string(std::string);
		virtual void interpret_file(std::string);
		virtual std::string const & get_last_error() const {return m_last_error;}

		virtual void register_script(std::string, std::string, std::string);
		virtual ScriptContainer & get_scripts_for(std::string ns) {
			return m_scripts[ns];
		}

		virtual void run_script(std::string, std::string);

		virtual LuaCoroutine * read_coroutine
			(Widelands::FileRead &, Widelands::Map_Map_Object_Loader &, uint32_t);
		virtual uint32_t write_coroutine
			(Widelands::FileWrite &, Widelands::Map_Map_Object_Saver &,
			 LuaCoroutine *);
		virtual void read_global_env
			(Widelands::FileRead &, Widelands::Map_Map_Object_Loader &, uint32_t);
		virtual uint32_t write_global_env
			(Widelands::FileWrite &, Widelands::Map_Map_Object_Saver &);
};

/*************************
 * Private functions
 *************************/
int LuaInterface_Impl::m_check_for_errors(int rv) {
	if (rv) {
		std::string err = luaL_checkstring(m_L, -1);
		lua_pop(m_L, 1);
		throw LuaError(err);
	}
	return rv;
}

/*************************
 * Public functions
 *************************/
LuaInterface_Impl::LuaInterface_Impl
	(Widelands::Editor_Game_Base * const egbase) : m_last_error("") {
	m_L = lua_open();

	// Open the lua libraries
#ifdef DEBUG
	static const luaL_Reg lualibs[] = {
		{"", luaopen_base},
		{LUA_LOADLIBNAME, luaopen_package},
		{LUA_TABLIBNAME, luaopen_table},
		{LUA_IOLIBNAME, luaopen_io},
		{LUA_OSLIBNAME, luaopen_os},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
		{LUA_DBLIBNAME, luaopen_debug},
		{0,               0}
	};
#else
	static const luaL_Reg lualibs[] = {
		{"", luaopen_base},
		{LUA_TABLIBNAME, luaopen_table},
		{LUA_STRLIBNAME, luaopen_string},
		{LUA_MATHLIBNAME, luaopen_math},
		{LUA_DBLIBNAME, luaopen_debug}, // needed for testsuite
		{0,               0}
	};
#endif
	const luaL_Reg * lib = lualibs;
	for (; lib->func; lib++) {
		lua_pushcfunction(m_L, lib->func);
		lua_pushstring(m_L, lib->name);
		lua_call(m_L, 1, 0);
	}

	// Now our own
	luaopen_globals(m_L);
	luaopen_wldebug(m_L);
	luaopen_wlmap(m_L);
	luaopen_wlgame(m_L);

	// Push the game onto the stack
	lua_pushstring(m_L, "egbase");
	lua_pushlightuserdata(m_L, static_cast<void *>(egbase));
	lua_settable(m_L, LUA_REGISTRYINDEX);
	// TODO: this should only be pushed if this is really a game!
	lua_pushstring(m_L, "game");
	lua_pushlightuserdata(m_L, static_cast<void *>(egbase));
	lua_settable(m_L, LUA_REGISTRYINDEX);
}

LuaInterface_Impl::~LuaInterface_Impl() {
	lua_close(m_L);
}

LuaCoroutine * LuaInterface_Impl::read_coroutine
	(Widelands::FileRead & fr, Widelands::Map_Map_Object_Loader & mol,
	 uint32_t size)
{
	LuaCoroutine_Impl * rv = new LuaCoroutine_Impl(0);

	rv->read(m_L, fr, mol, size);

	return rv;
}

uint32_t LuaInterface_Impl::write_coroutine
	(Widelands::FileWrite & fw, Widelands::Map_Map_Object_Saver & mos,
	 LuaCoroutine * cr)
{
	// we do not want to make the write function public by adding
	// it to the interface of LuaCoroutine. Therefore, we make a cast
	// to the Implementation function here.
	return dynamic_cast<LuaCoroutine_Impl *>(cr)->write(m_L, fw, mos);
}


static const char * m_persistent_globals[] = {
	"_VERSION", "assert", "collectgarbage", "coroutine", "debug",
	"dofile", "error", "gcinfo", "getfenv", "getmetatable", "io", "ipairs",
	"load", "loadfile", "loadstring", "math", "module", "newproxy", "next",
	"os", "package", "pairs", "pcall", "print", "rawequal",
	"rawget", "rawset", "require", "select", "setfenv", "setmetatable",
	"table", "tonumber", "tostring", "type", "unpack", "wl", "xpcall",
	"string", "use", "_", "set_textdomain", 0
};
void LuaInterface_Impl::read_global_env
	(Widelands::FileRead & fr, Widelands::Map_Map_Object_Loader & mol,
	 uint32_t size)
{
	// Empty table + object to persist on the stack Stack
	unpersist_object(m_L, m_persistent_globals, fr, mol, size);
	luaL_checktype(m_L, -1, LUA_TTABLE);

	// Now, we have to merge all keys from the loaded table
	// into the global table
	lua_pushnil(m_L);
	while (lua_next(m_L, -2) != 0) {
		// key value
		lua_pushvalue(m_L, -2); // key value key
		lua_gettable(m_L, LUA_GLOBALSINDEX); // key value global_value
		if (lua_equal(m_L, -1, -2)) {
			lua_pop(m_L, 2); // key
			continue;
		} else {
			// Make this a global value
			lua_pop(m_L, 1); // key value
			lua_pushvalue(m_L, -2); // key value key
			lua_pushvalue(m_L, -2); // key value key value
			lua_settable(m_L, LUA_GLOBALSINDEX); // key value
			lua_pop(m_L, 1); // key
		}
	}

	lua_pop(m_L, 1); // pop the table returned by unpersist_object
}

uint32_t LuaInterface_Impl::write_global_env
	(Widelands::FileWrite & fw, Widelands::Map_Map_Object_Saver & mos)
{
	// Empty table + object to persist on the stack Stack
	lua_newtable(m_L);
	lua_pushvalue(m_L, LUA_GLOBALSINDEX);

	return persist_object(m_L, m_persistent_globals, fw, mos);
}


void LuaInterface_Impl::register_script
	(std::string ns, std::string name, std::string content)
{
	m_scripts[ns][name] = content;
}

void LuaInterface_Impl::interpret_string(std::string cmd) {
	int rv = luaL_dostring(m_L, cmd.c_str());
	m_check_for_errors(rv);
}

void LuaInterface_Impl::interpret_file(std::string filename) {
	int rv = luaL_dofile(m_L, filename.c_str());
	m_check_for_errors(rv);
}

void LuaInterface_Impl::run_script(std::string ns, std::string name) {
	if
		((m_scripts.find(ns) == m_scripts.end()) ||
		 (m_scripts[ns].find(name) == m_scripts[ns].end()))
		throw LuaScriptNotExistingError(name);

	return interpret_string(m_scripts[ns][name]);
}

/*
============================================
       Global functions
============================================
*/
/*
 * Factory Function
 */
LuaInterface * create_lua_interface(Widelands::Editor_Game_Base * g) {
	return new LuaInterface_Impl(g);
}

