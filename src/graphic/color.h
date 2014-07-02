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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef RGBCOLOR_H
#define RGBCOLOR_H

#include <SDL.h>

// TODO(sirver): Do not inherit from SDL_Color if possible.
struct RGBColor : public SDL_Color {
	RGBColor(uint8_t R, uint8_t G, uint8_t B);

	// Initializes the color to black.
	RGBColor();

	// Map this color to the given 'fmt'
	uint32_t map(const SDL_PixelFormat& fmt) const;

	// Set it to the given 'clr' which is interpretes through 'fmt'.
	void set(SDL_PixelFormat * fmt, uint32_t clr);

	bool operator == (const RGBColor& other) const;
	bool operator != (const RGBColor& other) const;
};

struct RGBAColor {
	RGBAColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A);

	// Initializes the color to black.
	RGBAColor();

	// Initializes to opaque color.
	RGBAColor(const RGBColor& c);

	// Map this color to the given 'fmt'
	uint32_t map(const SDL_PixelFormat& fmt) const;

	// Set it to the given 'clr' which is interpretes through 'fmt'.
	void set(const SDL_PixelFormat & fmt, uint32_t clr);

	bool operator == (const RGBAColor& other) const;
	bool operator != (const RGBAColor& other) const;

	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

#endif
