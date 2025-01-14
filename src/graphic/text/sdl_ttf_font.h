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

#ifndef WL_GRAPHIC_TEXT_SDL_TTF_FONT_H
#define WL_GRAPHIC_TEXT_SDL_TTF_FONT_H

#include <memory>
#include <string>

#include <SDL_ttf.h>

#include "graphic/text/rt_render.h"

namespace RT {

// Implementation of a Font object using SDL_ttf.
class SdlTtfFont : public IFont {
public:
	SdlTtfFont
		(TTF_Font* ttf, const std::string& face, int ptsize, std::string* ttf_memory_block);
	virtual ~SdlTtfFont();

	void dimensions(const std::string&, int, uint16_t * w, uint16_t * h) override;
	const Texture& render(const std::string&, const RGBColor& clr, int, TextureCache*) override;
	uint16_t ascent(int) const override;

private:
	void m_set_style(int);

	TTF_Font * font_;
	int style_;
	const std::string font_name_;
	const int ptsize_;
	// Old version of SDLTtf seem to need to keep this around.
	std::unique_ptr<std::string> ttf_file_memory_block_;
};

}  // namespace RT

#endif  // end of include guard:
