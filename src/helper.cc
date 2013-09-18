/*
 * Copyright (C) 2002, 2006-2009 by the Widelands Development Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "helper.h"

#include <cstdarg>
#include <memory>
#include <string>

#include <boost/random.hpp>

using namespace std;

/// Split a string by separators.
/// \note This ignores empty elements, so do not use this for example to split
/// a string with newline characters into lines, because it would ignore empty
/// lines.
std::vector<std::string> split_string
	(const std::string & s, char const * const separators)
{
	std::vector<std::string> result;
	for
		(std::string::size_type pos = 0, endpos;
		 (pos = s.find_first_not_of(separators, pos)) != std::string::npos;
		 pos = endpos)
	{
		endpos = s.find_first_of(separators, pos);
		result.push_back(s.substr(pos, endpos - pos));
	}
	return result;
}

/**
 * remove spaces at the beginning or the end of a string
 */
void remove_spaces(std::string & s) {
	while (s[0] == ' ' or s[0] == '\t' or s[0] == '\n') s.erase(0, 1);

	while (*s.rbegin() == ' '  or *s.rbegin() == '\t' or *s.rbegin() == '\n')
		s.erase(s.size() - 1, 1);
}

/**
 * Tell whether a SDL_Keysym is printable.
 *
 * \param k SDL_Keysym to be checked for printability
 *
 * \return True if k is a printable character
 *
 * \todo This is _by_far_ not complete enough
 * \todo Should be based on k.unicode (already enabled by
 * WLApplication::init_hardware()) instead of k.sym. Doing so needs to take the
 * current locale into account; perhaps there already is a fitting gettext
 * function?
 */
bool is_printable(SDL_keysym k)
{
	return
		(k.sym == SDLK_TAB)                                   ||
		((k.sym >= SDLK_SPACE)   && (k.sym <= SDLK_z))        ||
		((k.sym >= SDLK_WORLD_0) && (k.sym <= SDLK_WORLD_95)) ||
		((k.sym >= SDLK_KP0)     && (k.sym <= SDLK_KP_EQUALS));
}

static boost::mt19937 random_generator;
string random_string(const string& chars, int nlen) {
	boost::uniform_int<> index_dist(0, chars.size() - 1);
	std::unique_ptr<char[]> buffer(new char[nlen]);
	for (int i = 0; i < nlen; ++i) {
		buffer[i] = chars[index_dist(random_generator)];
	}
	return string(buffer.get(), nlen);
}

