/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
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

#ifndef __S__FONT_LOADER_H
#define __S__FONT_LOADER_H

#include "rgbcolor.h"

#include <SDL_ttf.h>

#include <map>
#include <string>
#include <vector>

class FileRead;

/*
 * Font
 *
 * this represents a loaded font used by the FontHandler
 */
class Font_Loader {
	public:
		Font_Loader() {};
		~Font_Loader();
		TTF_Font* open_font(const std::string& name, int size);
		TTF_Font* get_font(std::string name, int size);
		void clear_fonts();
	private:
		std::map<std::string,TTF_Font*> m_font_table;
      std::vector<FileRead*> m_freads;
};

#endif // __S__FONT_LOADER_H
