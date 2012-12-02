/*
 * Copyright (C) 2010-2011 by the Widelands Development Team
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

#ifndef SURFACE_SDL_H
#define SURFACE_SDL_H

#include "rgbcolor.h"
#include "rect.h"

#include "graphic/pixelaccess.h"
#include "graphic/surface.h"

/**
* This implements SDL rendering. Do not use this class directly. The right
* way is to use the base struct Surface wherever possible. Everything which
* needs to know about the underlying renderer should go to the graphics
* subdirectory.
* Surfaces are created through Graphic::create_surface() functions.
*/
class SurfaceSDL : virtual public Surface, virtual public IPixelAccess {
public:
	SurfaceSDL(SDL_Surface & surface) :
		m_surface(&surface),
		m_offsx(0), m_offsy(0),
		m_w(surface.w), m_h(surface.h),
		m_isscreen(false)
	{}
	~SurfaceSDL();

	// Implements IBlitableSurface
	virtual uint32_t get_w() const {return m_w;}
	virtual uint32_t get_h() const {return m_h;}
	void blit(const Point&, const IPicture*, const Rect& srcrc, Composite cm);
	void fill_rect(const Rect&, RGBAColor);
	virtual IPixelAccess & pixelaccess() {return *this;}


	/// Set surface, only call once
	void set_sdl_surface(SDL_Surface & surface);
	SDL_Surface * get_sdl_surface() const {return m_surface;}

	/// Get width and height
	void update();

	/// Save a bitmap of this to a file
	void save_bmp(const char & fname) const;

	SDL_PixelFormat const & format() const;
	uint8_t * get_pixels() const;
	uint16_t get_pitch() const {return m_surface->pitch;}

	/// Lock
	void lock(LockMode);
	void unlock(UnlockMode);

	/// For the slowest: Indirect pixel access
	uint32_t get_pixel(uint32_t x, uint32_t y);
	void set_pixel(uint32_t x, uint32_t y, Uint32 clr);

	void clear();
	void draw_rect(const Rect&, RGBColor);
	void brighten_rect(const Rect&, int32_t factor);

	void draw_line
		(int32_t x1, int32_t y1,
		 int32_t x2, int32_t y2,
		 RGBColor, uint8_t width);

	// TODO(sirver): what is that? same as rendertarget?
	void set_subwin(const Rect& r);
	void unset_subwin();

	void set_isscreen(bool screen);

private:
	SDL_Surface * m_surface;
	int32_t m_offsx;
	int32_t m_offsy;
	uint32_t m_w, m_h;
	bool m_isscreen;
};

#endif
