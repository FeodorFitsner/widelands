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

#include "graphic/text/sdl_ttf_font.h"

#include <memory>

#include <SDL.h>
#include <SDL_ttf.h>
#include <boost/format.hpp>

#include "graphic/sdl_utils.h"
#include "graphic/text/rt_errors.h"
#include "graphic/texture.h"
#include "graphic/texture_cache.h"

using namespace std;
using namespace boost;

static const int SHADOW_OFFSET = 1;
static const SDL_Color SHADOW_CLR = {0, 0, 0, SDL_ALPHA_OPAQUE};

namespace RT {

SdlTtfFont::SdlTtfFont(TTF_Font * font, const string& face, int ptsize, string* ttf_memory_block) :
	font_(font), style_(TTF_STYLE_NORMAL), font_name_(face), ptsize_(ptsize),
	ttf_file_memory_block_(ttf_memory_block) {
}

SdlTtfFont::~SdlTtfFont() {
	TTF_CloseFont(font_);
	font_ = nullptr;
}

void SdlTtfFont::dimensions(const string& txt, int style, uint16_t * gw, uint16_t * gh) {
	m_set_style(style);

	int w, h;
	TTF_SizeUTF8(font_, txt.c_str(), &w, &h);

	if (style & SHADOW) {
		w += SHADOW_OFFSET; h += SHADOW_OFFSET;
	}
	*gw = w; *gh = h;
}

const Texture& SdlTtfFont::render
	(const string& txt, const RGBColor& clr, int style, TextureCache* texture_cache) {
	const string hash =
		(boost::format("%s:%s:%i:%02x%02x%02x:%i") % font_name_ % ptsize_ % txt %
		 static_cast<int>(clr.r) % static_cast<int>(clr.g) % static_cast<int>(clr.b) % style)
			.str();
	const Texture* rv = texture_cache->get(hash);
	if (rv) return *rv;

	m_set_style(style);

	SDL_Surface * text_surface = nullptr;

	SDL_Color sdlclr = {clr.r, clr.g, clr.b, SDL_ALPHA_OPAQUE};
	if (style & SHADOW) {
		SDL_Surface * tsurf = TTF_RenderUTF8_Blended(font_, txt.c_str(), sdlclr);
		SDL_Surface * shadow = TTF_RenderUTF8_Blended(font_, txt.c_str(), SHADOW_CLR);
		text_surface = empty_sdl_surface(shadow->w + SHADOW_OFFSET, shadow->h + SHADOW_OFFSET);
		SDL_FillRect(text_surface,
		             NULL,
						 SDL_MapRGBA(text_surface->format, 255, 255, 255, SDL_ALPHA_TRANSPARENT));

		if (text_surface->format->BitsPerPixel != 32)
			throw RenderError("SDL_TTF did not return a 32 bit surface for shadow text. Giving up!");

		SDL_Rect dstrct1 = {0, 0, 0, 0};
		SDL_SetSurfaceAlphaMod(shadow, SDL_ALPHA_OPAQUE);
		SDL_SetSurfaceBlendMode(shadow, SDL_BLENDMODE_NONE);
		SDL_BlitSurface(shadow, nullptr, text_surface, &dstrct1);

		uint32_t* spix = static_cast<uint32_t*>(tsurf->pixels);
		uint32_t* dpix = static_cast<uint32_t*>(text_surface->pixels);

		// Alpha Blend the Text onto the Shadow. This is really slow, but it is
		// the only compatible way to do it using SDL 1.2. SDL 2.0 offers more
		// functionality but is not yet released.
		uint8_t sr, sg, sb, sa, dr, dg, db, da, outa, outr = 0, outg = 0, outb = 0;
		for (int y = 0; y < tsurf->h; ++y) {
			for (int x = 0; x < tsurf->w; ++x) {
				size_t sidx = (y * tsurf->pitch + 4 * x) / 4;
				size_t didx = ((y + SHADOW_OFFSET) * text_surface->pitch + (x + SHADOW_OFFSET) * 4) / 4;

				SDL_GetRGBA(spix[sidx], tsurf->format, &sr, &sg, &sb, &sa);
				SDL_GetRGBA(dpix[didx], text_surface->format, &dr, &dg, &db, &da);

				outa = (255 * sa + da * (255 - sa)) / 255;
				if (outa) {
					outr = (255 * sa * sr + da * dr * (255 - sa)) / outa / 255;
					outg = (255 * sa * sg + da * dg * (255 - sa)) / outa / 255;
					outb = (255 * sa * sb + da * db * (255 - sa)) / outa / 255;
				}
				dpix[didx] = SDL_MapRGBA(text_surface->format, outr, outg, outb, outa);
			}
		}
		SDL_FreeSurface(tsurf);
		SDL_FreeSurface(shadow);
	} else
		text_surface = TTF_RenderUTF8_Blended(font_, txt.c_str(), sdlclr);

	if (!text_surface)
		throw RenderError((format("Rendering '%s' gave the error: %s") % txt % TTF_GetError()).str());

	return *texture_cache->insert(hash, std::unique_ptr<Texture>(new Texture(text_surface)));
}

uint16_t SdlTtfFont::ascent(int style) const {
	uint16_t rv = TTF_FontAscent(font_);
	if (style & SHADOW)
		rv += SHADOW_OFFSET;
	return rv;
}

void SdlTtfFont::m_set_style(int style) {
	// Those must have been handled by loading the correct font.
	assert(!(style & BOLD));
	assert(!(style & ITALIC));

	int sdl_style = TTF_STYLE_NORMAL;
	if (style & UNDERLINE) sdl_style |= TTF_STYLE_UNDERLINE;

	// Remember the last style. This should avoid that SDL_TTF flushes its
	// glyphcache all too often
	if (sdl_style == style_)
		return;
	style_ = sdl_style;
	TTF_SetFontStyle(font_, sdl_style);
}

}  // namespace RT
