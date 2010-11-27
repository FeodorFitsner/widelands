/*
 * Copyright 2010 by the Widelands Development Team
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
 */

#if !defined(SURFACE_OPENGL_H) and defined(USE_OPENGL)
#define SURFACE_OPENGL_H

#include "rgbcolor.h"
#include "rect.h"
#include "graphic/picture.h"
#include "graphic/pixelaccess.h"
#include "graphic/surface.h"

#include <SDL_opengl.h>

#include <cassert>

#define WL_GLINTERNALFORMAT GL_RGBA

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
 * This implements OpenGL rendering. Do not use this class directly. The right
 * way is to use the base class Surface wherever possible. Everything which
 * needs to know about the underlying renderer should go to the graphics
 * subdirectory.
 * Surfaces are created through Graphic::create_surface() functions.
 *
 * TODO: the ancestry is fake; this should be split in one class for rendering
 * and another for textures
 */
struct SurfaceOpenGL : Surface, IPicture, IPixelAccess {
public:
	/**
	 * Manages a single OpenGL texture. This makes sure the texture is
	 * freed when the object is destroyed.
	*/
	struct oglTexture {
		oglTexture() {}
		oglTexture(GLuint id): m_textureID(id) {}
		~oglTexture() {glDeleteTextures(1, &m_textureID);}
		GLuint id() const {return m_textureID;}
	private:
		GLuint m_textureID;
	};

	SurfaceOpenGL(SDL_Surface & surface);
	explicit SurfaceOpenGL(int w = 0, int h = 0);
	~SurfaceOpenGL();

	virtual bool valid();

	//@{
	/// Get width and height
	uint32_t get_w() {return m_w;}
	uint32_t get_h() {return m_h;}
	uint32_t get_tex_w() const {return m_tex_w;}
	uint32_t get_tex_h() const {return m_tex_h;}
	//@}

	void update();

	GLuint get_texture() const
	{
#if defined(DEBUG)
		if (m_isscreen)
			throw wexception
				("Try to get opengl texture id but not a source surface");
		if (not m_texture)
			throw wexception("no texture");
#endif
		return m_texture->id();
	}

	const SDL_PixelFormat * get_format() const;
	SDL_PixelFormat const & format() const {return *get_format();}

	/// Directly access the pixels. This is only valid if the surface is locked
	inline uint16_t get_pitch() const {return m_tex_w * 4;}
	uint8_t * get_pixels() const
	{
		assert(m_locked);
		assert(m_pixels);
		return m_pixels;
	}

	/// Lock: The Surface need to be locked before get or set pixel
	void lock();
	void unlock();

	/// For the slowest: Indirect pixel access
	inline uint32_t get_pixel(uint32_t x, uint32_t y) {
		assert(x < get_w());
		assert(y < get_h());
		assert(m_locked);

		return *reinterpret_cast<uint32_t *>(m_pixels + y * get_pitch() + x * 4);
	}
	inline void set_pixel(uint32_t x, uint32_t y, Uint32 clr) {
		assert(x < get_w());
		assert(y < get_h());
		assert(m_locked);
		m_glTexUpdate = true;

		*reinterpret_cast<uint32_t *>(m_pixels + y * get_pitch() + x * 4) = clr;
	}

	void clear();
	void draw_rect(Rect, RGBColor);
	void fill_rect(Rect, RGBAColor);
	void brighten_rect(Rect, int32_t factor);

	void draw_line
		(int32_t x1, int32_t y1,
		 int32_t x2, int32_t y2,
		 RGBColor, Rect const * clip = 0);

	void blit(Point, PictureID, Rect srcrc, Composite cm);
	void fast_blit(PictureID);

	oglTexture & getTexture() {return *m_texture;}

	virtual IPixelAccess & pixelaccess() {return *this;}
	virtual Surface & surface() {return *this;} //TODO get rid of this

private:
	SurfaceOpenGL & operator= (SurfaceOpenGL const &);
	explicit SurfaceOpenGL    (SurfaceOpenGL const &);

	/// stores the opengl texture id and frees if destroyed
	oglTexture * m_texture;
	/// Indicates that the texture needs to be updated after pixel access
	bool m_glTexUpdate;

	//@{
	/// set/unset a clipping rectangle
	void set_subwin(Rect r);
	void unset_subwin();
	//@}

	uint8_t * m_pixels;
	bool m_locked;

	/// Logical width and height of the surface
	uint32_t m_w, m_h;

	/// Keep the size of the opengl texture. This is necessary because some
	/// systems support only a power of two for texture sizes.
	uint32_t m_tex_w, m_tex_h;

	//TODO: should be superseded by class hierarchy
public:
	bool m_isscreen;
};

#endif
