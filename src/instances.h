/*
 * Copyright (C) 2002, 2003 by the Widelands Development Team
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

#ifndef __S__INSTANCE_H
#define __S__INSTANCE_H

#include "field.h"

class Editor_Game_Base;
class Game;
class Object_Manager;
class RenderTarget;
class Animation;
class Path;
class Player;

//
// Base class for descriptions of worker, files and so on. this must just
// link them together
//
class Map_Object;

class Map_Object_Descr {
public:
	Map_Object_Descr(void) { }
	virtual ~Map_Object_Descr(void) { }

	bool has_attribute(uint attr);

protected:
	void add_attribute(uint attr);

private:
	std::vector<uint>	m_attributes;

public:
	static uint get_attribute_id(std::string name);

private:
	typedef std::map<std::string, uint> AttribMap;

	static uint			s_dyn_attribhigh;		// highest attribute ID used
	static AttribMap	s_dyn_attribs;
};


/*
Notes on Map_Object
-------------------

Map_Object is the base class for everything that can be on the map: buildings, animals,
decorations, etc... most of the time, however, you'll deal with one of the derived
classes, BaseImmovable or Bob.

Every Map_Object has a unique serial number. This serial number is used as key in the
Object_Manager map, and in the safe Object_Ptr.
Unless you're perfectly sure about when an object can be destroyed you should use
an Object_Ptr. (there are some well-defined exceptions, such as Building<->Flag
relationships)

Map_Objects can also have attributes. They are mainly useful for finding objects of a
given type (e.g. trees) within a certain radius.

DO NOT allocate/free Map_Objects directly.
Use the appropriate type-dependent create() function for creation, and call die()
for removal.
Note that convenient creation functions are defined in class Game.
*/

// If you find a better way to do this that doesn't cost a virtual function or additional
// member variable, go ahead
#define MO_DESCR(type) \
protected: inline type* get_descr() const { return static_cast<type*>(m_descr); }

// would be necessary for virtual inheritance stuff
//#define MO_VIRTUAL_DESCR(type)
//protected: type* m_descr; inline type* get_descr() const { return m_descr; }

class Map_Object {
   friend class Object_Manager;
	friend class Object_Ptr;

	MO_DESCR(Map_Object_Descr)

public:
	enum {
		BOB,			// class Bob

		WARE,			// class WareInstance

		// everything below is at least a BaseImmovable
		IMMOVABLE,

		// everything below is at least a PlayerImmovable
		BUILDING,
		FLAG,
		ROAD
	};
	// Some default, globally valid, attributes.
	// Other attributes (such as "harvestable corn") could be allocated dynamically (?)
	enum Attribute {
		WAREHOUSE = 1,		// assume BUILDING
		WORKER,				// assume BOB

		HIGHEST_FIXED_ATTRIBUTE
	};

	// the enums tell us where we are going
	enum WalkingDir {
		IDLE = 0,
		WALK_NE = 1,
		WALK_E = 2,
		WALK_SE = 3,
		WALK_SW = 4,
		WALK_W = 5,
		WALK_NW = 6,
	};

protected:
	Map_Object(Map_Object_Descr *descr);
	virtual ~Map_Object(void);

public:
	virtual int get_type() = 0;

	inline uint get_serial(void) const { return m_serial; }
	inline bool has_attribute(uint attr) { return m_descr->has_attribute(attr); }

	void remove(Editor_Game_Base*);
	virtual void destroy(Editor_Game_Base*);

   // The next functions are really only needed in games.
   // Not in Editor
	void schedule_destroy(Game *g);
	uint schedule_act(Game* g, uint tdelta, uint data = 0);
	virtual void act(Game*, uint data);

protected:
   // init for editor and game
	virtual void init(Editor_Game_Base*);
	virtual void cleanup(Editor_Game_Base*);

	void molog(const char* fmt, ...);

protected:
	Map_Object_Descr *m_descr;
	uint m_serial;
};

inline int get_reverse_dir(int dir) { return 1 + ((dir-1)+3)%6; }


/** class Object_Manager
 *
 * Keeps the list of all objects currently in the game.
 */
class Object_Manager {
	typedef std::map<uint, Map_Object *> objmap_t;

public:
	Object_Manager() { m_lastserial = 0; }
	~Object_Manager(void);

	void cleanup(Editor_Game_Base *g);

	inline Map_Object* get_object(uint serial) {
		objmap_t::iterator it = m_objects.find(serial);
		if (it == m_objects.end())
			return 0;
		return it->second;
	}

	void insert(Map_Object *obj);
	void remove(Map_Object *obj);

private:
	uint m_lastserial;
	objmap_t m_objects;
};

/** class Object_Ptr
 *
 * Provides a safe pointer to a Map_Object
 */
class Object_Ptr {
public:
	inline Object_Ptr() { m_serial = 0; }
	inline Object_Ptr(Map_Object* obj) { m_serial = obj->m_serial; }
	// can use standard copy constructor and assignment operator

	inline void set(Map_Object* obj) { if (obj) m_serial = obj->m_serial; else m_serial = 0; }
	inline Object_Ptr& operator = (Map_Object* obj) { set(obj); return *this; }

   inline bool is_set(void) { return m_serial; }

	// dammit... without a Editor_Game_Base object, we can't implement a Map_Object* operator
	// (would be _really_ nice)
	Map_Object* get(Editor_Game_Base* g);

private:
	uint m_serial;
};

#endif // __S__INSTANCE_H
