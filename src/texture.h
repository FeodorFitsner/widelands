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

#ifndef TEXTURE_H
#define TEXTURE_H

#include "colormap.h"
#include "types.h"
#include <string>

/**
 * This contains all the road textures needed to render roads
 */
struct Road_Textures {
   uint pic_road_normal;
   uint pic_road_busy;
};

/** class Texture
*
* Texture represents are terrain texture, which is strictly
* TEXTURE_WIDTH by TEXTURE_HEIGHT pixels in size. It uses 8 bit color, and a pointer
* to the corresponding palette and color lookup table is provided.
*
* Currently, this is initialized from a 16 bit bitmap. This should be
* changed to load 8 bit bitmaps directly.
*/
class Texture {
private:
	Colormap      * m_colormap;
	uint            m_nrframes;
	unsigned char * m_pixels;
	uint            m_frametime;
	unsigned char * m_curframe;
	std::string     m_texture_picture;
	bool            is_32bit;
	bool            m_was_animated;

public:
	Texture(const char & fnametempl, const uint frametime, const SDL_PixelFormat &);
	~Texture();

	const char* get_texture_picture(void) { return m_texture_picture.c_str(); }

	unsigned char *get_pixels () const { return m_pixels; }
	unsigned char* get_curpixels() const { return m_curframe; }
	void* get_colormap () const { return m_colormap->get_colormap(); }

	Uint32 get_minimap_color(const char shade);

	void animate(uint time);
	void reset_was_animated( void ) { m_was_animated = false; }
	bool was_animated() const throw () {return m_was_animated;}
};

#endif
