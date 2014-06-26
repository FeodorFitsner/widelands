/*
 * Copyright (C) 2006-2014 by the Widelands Development Team
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

#include "graphic/color.h"

RGBColor::RGBColor() {
}

RGBColor::RGBColor(uint8_t const R, uint8_t const G, uint8_t const B) {
	SDL_Color::r = R, SDL_Color::g = G, SDL_Color::b = B;
}

Uint32 RGBColor::map(const SDL_PixelFormat& fmt) const {
	return SDL_MapRGB(&const_cast<SDL_PixelFormat&>(fmt), r, g, b);
}

void RGBColor::set(SDL_PixelFormat* const fmt, Uint32 const clr) {
	SDL_GetRGB(clr, fmt, &r, &g, &b);
}

bool RGBColor::operator == (const RGBColor& other) const {
	return r == other.r and g == other.g and b == other.b;
}

RGBAColor::RGBAColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A) : r(R), g(G), b(B), a(A) {
}

// Initializes the color to black.
RGBAColor::RGBAColor() : RGBAColor(0, 0, 0, 0) {
}

RGBAColor::RGBAColor(const RGBColor& c) {
	r = c.r;
	g = c.g;
	b = c.b;
	a = 255;
}

uint32_t RGBAColor::map(const SDL_PixelFormat& fmt) const {
	return SDL_MapRGBA(&const_cast<SDL_PixelFormat&>(fmt), r, g, b, a);
}

void RGBAColor::set(const SDL_PixelFormat& fmt, const uint32_t clr) {
	SDL_GetRGBA(clr, const_cast<SDL_PixelFormat*>(&fmt), &r, &g, &b, &a);
}
