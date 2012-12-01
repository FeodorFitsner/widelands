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


#ifndef SDL_TTF_FONT_IMPL_H
#define SDL_TTF_FONT_IMPL_H

#include <string>

#include <SDL_ttf.h>

#include "rt_render.h"

namespace RT {

// Implementation of a Font object using SDL_ttf.
class SDLTTF_Font : public IFont {
public:
	SDLTTF_Font(TTF_Font* ttf);
	virtual ~SDLTTF_Font();

	void dimensions(std::string, int, uint32_t * w, uint32_t * h);
	virtual IPicture* render(IGraphic &, std::string, RGBColor clr, int);
	uint32_t ascent(int) const;

private:
	void m_set_style(int);

	TTF_Font * m_font;
	int m_style;
};

}  // namespace RT



#endif /* end of include guard: SDL_TTF_FONT_IMPL_H */

