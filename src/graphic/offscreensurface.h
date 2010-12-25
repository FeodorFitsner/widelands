/*
 * Copyright (C) 2010 by the Widelands Development Team
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

#ifndef OFFSCREENSURFACE_H
#define OFFSCREENSURFACE_H

#include "picture.h"
#include "pixelaccess.h"
#include "surface.h"

/**
 * Interface to surfaces that can act conveniently both as
 * source and as destination for rendering.
 */
struct IOffscreenSurface : Surface, IPicture, IPixelAccess {
};

#endif // OFFSCREENSURFACE_H
