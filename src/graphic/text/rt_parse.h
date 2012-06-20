/*
 * Copyright (C) 2006-2012 by the Widelands Development Team
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

#ifndef RT_PARSER_H
#define RT_PARSER_H

#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include "rt_errors.h"

struct SDL_Color;

namespace RT {

struct Child;
class IAttr {
public:
	virtual ~IAttr() {};

	virtual const std::string & name() const = 0;
	virtual long get_int() const = 0;
	virtual bool get_bool() const = 0;
	virtual std::string get_string() const = 0;
	virtual void get_color(SDL_Color *) const = 0;
};

class IAttrMap {
public:
	virtual ~IAttrMap() {};

	virtual const IAttr & operator[] (const std::string&) const throw (AttributeNotFound) = 0;
	virtual bool has(const std::string &) const = 0;
};

class ITag {
public:
	typedef std::vector<Child*> ChildList;

	virtual ~ITag() {};
	virtual const std::string & name() const = 0;
	virtual const IAttrMap & attrs() const = 0;
	virtual const ChildList & childs() const = 0;
};

struct Child {
	Child() : tag(0), text() {}
	Child(ITag * t) : tag(t) {}
	Child(std::string t) : tag(0), text(t) {}
	~Child() {
		if (tag) delete tag;
	}
	ITag * tag;
	std::string text;
};


class IParser {
public:
	virtual ~IParser() {};

	virtual ITag * parse(std::string text) = 0;
	virtual std::string remaining_text() = 0;
};

// This function is mainly for testing
IParser * setup_parser();
}

#endif /* end of include guard: RT_PARSER_H */

