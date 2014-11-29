/*
 * Copyright (C) 2002-2004, 2006-2010 by the Widelands Development Team
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

#ifndef WL_GRAPHIC_IMAGE_H
#define WL_GRAPHIC_IMAGE_H

#include <string>

#include <stdint.h>

#include "base/macros.h"

class Texture;

/**
 * Interface to a bitmap that can act as the source of a rendering
 * operation.
 */
class Image {
public:
	Image() = default;
	virtual ~Image() {}

	virtual uint16_t width() const = 0;
	virtual uint16_t height() const = 0;

	// Internal functions needed for caching.
	virtual Texture* texture() const = 0;
	virtual const std::string& hash() const = 0;

	DISALLOW_COPY_AND_ASSIGN(Image);
};


#endif  // end of include guard: WL_GRAPHIC_IMAGE_H
