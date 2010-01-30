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

#include <boost/lexical_cast.hpp>

#include "coroutine_impl.h"

#include "c_utils.h"
#include "pluto/pluto.h"

uint32_t LuaCoroutine_Impl::g_idx = 0;

LuaCoroutine_Impl::LuaCoroutine_Impl(lua_State* ms)
	: m_L(ms)
{
	// Cache a lua reference to this object so that it does not
	// get garbage collected
	m_idx = g_idx ++;
	if(ms) {
		// TODO: factor this out. This is essentially the same as code in read.
		lua_pushthread(ms);
		std::string key = "co_" + boost::lexical_cast<std::string>(m_idx);
		log("construct: key.c_str(: %s\n", key.c_str());
		lua_setfield(ms, LUA_REGISTRYINDEX, key.c_str());
	}
}

LuaCoroutine_Impl::~LuaCoroutine_Impl()
{
	// Release the reference cached.
	// TODO: factor this out
	lua_pushnil(m_L);
	std::string key = "co_" + boost::lexical_cast<std::string>(m_idx);
	log("destruct: key.c_str(: %s\n", key.c_str());
	lua_setfield(m_L, LUA_REGISTRYINDEX, key.c_str());
}
int LuaCoroutine_Impl::resume(uint32_t * sleeptime)
{
	int rv = lua_resume(m_L, 0);
	int n = lua_gettop(m_L);

	uint32_t sleep_for = 0;
	if (n == 1) {
		sleep_for = luaL_checkint32(m_L, -1);
		lua_pop(m_L, 1);
	}

	if (sleeptime)
		*sleeptime = sleep_for;

	if (rv != 0 && rv != YIELDED)
		return lua_error(m_L);

	return rv;
}


struct DataWriter {
	uint32_t written;
	Widelands::FileWrite & fw;

	DataWriter(Widelands::FileWrite & gfw) : written(0), fw(gfw) {}
};

int write_func(lua_State *, const void * p, size_t data, void * ud) {
	DataWriter * dw = static_cast<DataWriter *>(ud);

	dw->fw.Data(p, data, Widelands::FileWrite::Pos::Null());
	dw->written += data;

	return data;
}

uint32_t LuaCoroutine_Impl::write
	(lua_State * parent, Widelands::FileWrite & fw,
	 Widelands::Map_Map_Object_Saver & mos)
{
	int n = lua_gettop(parent);
	log("\n\nWriting Coroutine: %i\n", n);

	// Save a reference to the object saver
	lua_pushlightuserdata(parent, &mos);
	lua_setfield(parent, LUA_REGISTRYINDEX, "mos");

	// Push all the stuff that should be be regenerated
	lua_newtable(parent);
	lua_getglobal(parent, "coroutine");
	lua_getfield(parent, -1, "yield");
	lua_pushint32(parent, 1); // stack: newtable coroutine yield integer
	lua_settable(parent, -4); //  newtable[yield] = integer
	lua_pop(parent, 1); // pop coroutine

	log("Pushed all globals\n");

	// Dirty hack: we have to push m_L on the childs stack, then get it from
	// there to push it on the parents stack. I didn't find another way to push
	// m_L as a thread type onto the parents stack.
	lua_pushthread(m_L);
	lua_xmove (m_L, parent, 1);

	log("after pushing object: %i\n", lua_gettop(parent));

	// fw.Unsigned32(0xff);
	DataWriter dw(fw);

	pluto_persist(parent, &write_func, &dw);
	lua_pop(parent, 2); // pop the two tables

	log("After pluto_persist!\n");
	log("Pickled %i bytes. Stack size: %i\n", dw.written, lua_gettop(parent));

	// Delete the entry in the registry
	lua_pushnil(parent);
	lua_setfield(parent, LUA_REGISTRYINDEX, "mos");

	return dw.written;
}

struct DataReader {
	uint32_t size;
	Widelands::FileRead & fr;

	DataReader(uint32_t gsize, Widelands::FileRead & gfr) :
		size(gsize), fr(gfr) {}
};

const char * read_func(lua_State *, void * ud, size_t * sz) {
	DataReader * dr = static_cast<DataReader *>(ud);

	*sz = dr->size;
	return static_cast<const char *>(dr->fr.Data(dr->size));
}

void LuaCoroutine_Impl::read
	(lua_State * parent, Widelands::FileRead & fr,
	 Widelands::Map_Map_Object_Loader & mol, uint32_t size)
{
	int n = lua_gettop(parent);
	log("\n\nReading Coroutine: %i\n", n);
		
	
	// Save the mol in the registry
	lua_pushlightuserdata(parent, &mol);
	lua_setfield(parent, LUA_REGISTRYINDEX, "mol");

	// Push all the stuff that should not be regenerated
	lua_newtable(parent);
	lua_getglobal(parent, "coroutine");
	lua_pushint32(parent, 1);
	lua_getfield(parent, -2, "yield");// stack: table coroutine integer yield
	lua_settable(parent, -4); //  newtable[integer] = yield
	lua_pop(parent, 1); // pop coroutine

	log("Pushed all globals\n");

	DataReader rd(size, fr);

	pluto_unpersist(parent, &read_func, &rd);
	m_L = luaL_checkthread(parent, -1);
	lua_pop(parent, 2); // pop the thread & the table
	
	// TODO: this persistence code is the same as above
	lua_pushthread(m_L);
	std::string key = "co_" + boost::lexical_cast<std::string>(m_idx);
	log("construct: key.c_str(: %s\n", key.c_str());
	lua_setfield(m_L, LUA_REGISTRYINDEX, key.c_str());

	log("Done with pickling!\n");

	// Delete the entry in the registry
	lua_pushnil(parent);
	lua_setfield(parent, LUA_REGISTRYINDEX, "mol");

	log("Unpickled %i bytes. Stack size: %i\n", size, lua_gettop(parent));
}

