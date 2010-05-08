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

#ifndef COROUTINE_IMPL_H
#define COROUTINE_IMPL_H

#include "scripting.h"

namespace Widelands {
	struct Player;
}

/*
============================================
       Lua Coroutine
============================================
*/
class LuaCoroutine_Impl : public LuaCoroutine {
public:
	LuaCoroutine_Impl(lua_State* L);
	virtual ~LuaCoroutine_Impl();

	virtual int get_status() {return lua_status(m_L);}
	virtual int resume(uint32_t * sleeptime = 0);

	virtual uint32_t write
		(lua_State*, Widelands::FileWrite &,
		 Widelands::Map_Map_Object_Saver & );
	virtual void read
		(lua_State*, Widelands::FileRead &,
		 Widelands::Map_Map_Object_Loader &, uint32_t);

	virtual void push_arg(const Widelands::Player *);

private:
	void m_reference();
	void m_unreference();

private:
	lua_State * m_L;
	uint32_t m_idx;
	uint32_t m_nargs;
};


#endif /* end of include guard: COROUTINE_IMPL_H */

