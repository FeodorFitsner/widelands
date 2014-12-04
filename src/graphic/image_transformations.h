/*
 * Copyright (C) 2006-2013 by the Widelands Development Team
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

#ifndef WL_GRAPHIC_IMAGE_TRANSFORMATIONS_H
#define WL_GRAPHIC_IMAGE_TRANSFORMATIONS_H

#include <stdint.h>

struct RGBColor;
class Image;

// A set of image transformations used in Widelands.
namespace ImageTransformations {

// This must be called once before any of the functions below are called. This
// is done when the graphics system is initialized.
void initialize();

// All of the functions below take an original image, transform it, cache the
// newly created image in the global ImageCache and return it. It is therefore
// safe to call the methods with the same arguments multiple times without
// construction cost.

// Encodes the given Image into the corresponding image with a player color.
// Takes the image and the player color mask and the new color the image should
// be tainted in.
const Image* player_colored(const RGBColor& clr, const Image* original, const Image* mask);
}


#endif  // end of include guard: WL_GRAPHIC_IMAGE_TRANSFORMATIONS_H
