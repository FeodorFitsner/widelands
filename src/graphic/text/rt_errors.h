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

#ifndef WL_GRAPHIC_TEXT_RT_ERRORS_H
#define WL_GRAPHIC_TEXT_RT_ERRORS_H

#include "base/wexception.h"

#include <exception>

namespace RT {

class Exception : public std::exception {
public:
	Exception(std::string msg) : std::exception(), m_msg(msg) {
	}
	const char* what() const noexcept override {return m_msg.c_str();}

private:
	std::string m_msg;
};

#define DEF_ERR(Name) class Name : public Exception { \
public: \
		  Name(std::string msg) : Exception(msg) {} \
};

DEF_ERR(AttributeNotFound)
DEF_ERR(BadFont)
DEF_ERR(EndOfText)
DEF_ERR(InvalidColor)
DEF_ERR(RenderError)
DEF_ERR(SyntaxError)
DEF_ERR(WidthTooSmall)

#undef DEF_ERR


}

#endif  // end of include guard: WL_GRAPHIC_TEXT_RT_ERRORS_H
