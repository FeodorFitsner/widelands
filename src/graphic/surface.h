/*
 * Copyright (C) 2002-2004, 2006-2008 by the Widelands Development Team
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

#ifndef SURFACE_H
#define SURFACE_H

#include "rgbcolor.h"
#include "texture.h"

#include "rect.h"

#include <SDL_opengl.h>


namespace Widelands {
struct Editor_Game_Base;
struct Field;
struct Player;
}

namespace UI {
struct Font_Handler;
}

struct Vertex;

/**
 * This was formerly called struct Bitmap. But now it manages an
 * SDL_Surface as it's core.
 *
 * Represents a simple bitmap without managing its memory. The rendering
 * functions do NOT perform any clipping; this is up to the caller.
*/
class Surface {
	friend class AnimationGfx;
	friend struct UI::Font_Handler; //  needs m_surface for SDL_Blitting

	SDL_Surface * m_surface;
	int32_t m_offsx;
	int32_t m_offsy;
	uint32_t m_w, m_h;

public:
	Surface() : 
		m_surface(0), m_offsx(0), m_offsy(0) 
#ifdef HAS_OPENGL
		, m_surf_type(SURF_SDL),
		m_texture(0)
#endif 
	{}
	
	Surface(Surface const &);
	~Surface();

	/// Set surface, only call once
	void set_sdl_surface(SDL_Surface & surface);
	SDL_Surface * get_sdl_surface() {return m_surface;}

	/// Get width and height
	uint32_t get_w() const {return m_w;}
	uint32_t get_h() const {return m_h;}
	void update();

	/// Save a bitmap of this to a file
	void save_bmp(const char & fname) const;

	// For the bravest: Direct Pixel access. Use carefully
	/// Needed if you want to blit directly to the screen by memcpy
	void force_disable_alpha();
	const SDL_PixelFormat * get_format() const;
	const SDL_PixelFormat & format() const;
	uint16_t get_pitch() const {return m_surface->pitch;}
	void * get_pixels() const throw ();

	/// Lock
	void lock();
	void unlock();

	/// For the slowest: Indirect pixel access
	uint32_t get_pixel(uint32_t x, uint32_t y) __attribute__ ((hot));
	void set_pixel(uint32_t x, uint32_t y, Uint32 clr) __attribute__ ((hot));

	void clear();
	void draw_rect(Rect, RGBColor);
	void fill_rect(Rect, RGBAColor);
	void brighten_rect(Rect, int32_t factor);

	void blit(Point, Surface *, Rect srcrc, bool enable_alpha = true);
	void fast_blit(Surface *);

	void draw_minimap
		(Widelands::Editor_Game_Base const &,
		 Widelands::Player           const *,
		 Rect, Point, uint32_t flags);


	/// sw16_terrain.cc
	void draw_field
		(Rect &,
		 Vertex const & f_vert,
		 Vertex const & r_vert,
		 Vertex const & bl_vert,
		 Vertex const & br_vert,
		 uint8_t roads,
		 const Texture & tr_d_texture,
		 const Texture & l_r_texture,
		 const Texture & f_d_texture,
		 const Texture & f_r_texture);

#ifdef HAS_OPENGL
	enum SurfaceType_t
	{
		SURF_SDL,
		SURF_OGLHW
	} m_surf_type;
	void set_isGLsf(){m_surf_type=SURF_OGLHW;}
	bool isGLsf() const {return m_surf_type==SURF_OGLHW;}
	
	class oglTexture
	{
	  public:
		oglTexture() {}
		oglTexture(GLuint id): m_textureID(id) {}
		~oglTexture()
			{glDeleteTextures( 1, &m_textureID);}
		GLuint id() {return m_textureID;}
		GLuint m_textureID;
	};
	
	oglTexture * m_texture;
	bool m_glTexUpdate;
	
	GLuint getTexture();
#endif

private:
	void set_subwin(Rect r);
	void unset_subwin();
};

#endif
