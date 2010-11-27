/*
 * Copyright (C) 2002-2004, 2006, 2009-2010 by the Widelands Development Team
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

#include "io/fileread.h"
#include "io/filesystem/layered_filesystem.h"
#include "io/streamwrite.h"

#include "surface_sdl.h"
#include "surface_opengl.h"
#include "graphic/graphic.h"
#include "graphic/picture.h"

#include "log.h"
#include "upcast.h"

#include <SDL.h>

/*
==============================================================================

AnimationGfx -- contains graphics data for an animtion

==============================================================================
*/

/*
===============
AnimationGfx::AnimationGfx

Load the animation
===============
*/
static const uint32_t nextensions = 2;
static const char extensions[nextensions][5] = {".png", ".jpg"};
AnimationGfx::AnimationGfx(AnimationData const * const data) :
	m_hotspot(data->hotspot)
{
	m_hasplrclrs = data->hasplrclrs;

	//  In the filename template, the last sequence of '?' characters (if any)
	//  is replaced with a number, for example the template "idle_??" is
	//  replaced with "idle_00". Then the code looks if there is a file with
	//  that name + any extension in the list above. Assuming that it finds a
	//  file, it increments the number so that the filename is "idle_01" and
	//  looks for a file with that name + extension, and so on until it can not
	//  find any file. Then it is assumed that there are no more frames in the
	//  animation.

	//  Allocate a buffer for the filename. It must be large enough to hold the
	//  picture name template, the "_pc" (3 characters) part for playercolor
	//  masks, the extension (4 characters including the dot) and the null
	//  terminator. Copy the picture name template into the buffer.
	char filename[256];
	std::string::size_type const picnametempl_size = data->picnametempl.size();
	if (sizeof(filename) < picnametempl_size + 3 + 4 + 1)
		throw wexception
			("buffer too small (%u) for picture name temlplate of size %u\n",
			 sizeof(filename), picnametempl_size);
	strcpy(filename, data->picnametempl.c_str());

	//  Find out where in the picture name template the number is. Search
	//  backwards from the end.
	char * const after_basename = filename + picnametempl_size;
	char * last_digit = after_basename;
	while (filename <= last_digit and *last_digit != '?')
		--last_digit;
	char * before_first_digit = last_digit;
	while (filename <= before_first_digit and *before_first_digit == '?') {
		*before_first_digit = '0';
		--before_first_digit;
	}
	unsigned int width = 0, height = 0;
#ifndef NDEBUG
	//#define VALIDATE_ANIMATION_CROPPING
#endif
#ifdef VALIDATE_ANIMATION_CROPPING
	bool data_in_x_min = false, data_in_x_max = false;
	bool data_in_y_min = false, data_in_y_max = false;
#endif

	for (;;) {
		// Load the base image
		for (size_t extnr = 0;;) {
			strcpy(after_basename, extensions[extnr]);
			if (g_fs->FileExists(filename)) { //  Is the frame actually there?
				try {
					PictureID pic = g_gr->load_image(filename, true);
					if (width == 0) { //  This is the first frame.
						width  = pic->get_w();
						height = pic->get_h();
					} else if (width != pic->get_w() or height != pic->get_h())
						throw wexception
							("wrong size: (%u, %u), should be (%u, %u) like the "
							 "first frame",
							 pic->get_w(), pic->get_h(), width, height);
					//  Get a new AnimFrame.
					m_plrframes[0].push_back(pic);
#ifdef VALIDATE_ANIMATION_CROPPING
					if (not data_in_x_min)
						for (int y = 0; y < height; ++y) {
							uint8_t r, g, b, a;
							SDL_GetRGBA
								(frame.get_pixel(0,         y),
								 surface.format,
								 &r, &g, &b, &a);
							if (a) {
								data_in_x_min = true;
								break;
							}
						}
					if (not data_in_x_max)
						for (int y = 0; y < height; ++y) {
							uint8_t r, g, b, a;
							SDL_GetRGBA
								(frame.get_pixel(width - 1, y),
								 surface.format,
								 &r, &g, &b, &a);
							if (a) {
								data_in_x_max = true;
								break;
							}
						}
					if (not data_in_y_min)
						for (int x = 0; x < width; ++x) {
							uint8_t r, g, b, a;
							SDL_GetRGBA
								(frame.get_pixel(x,         0),
								 surface.format,
								 &r, &g, &b, &a);
							if (a) {
								data_in_y_min = true;
								break;
							}
						}
					if (not data_in_y_max)
						for (int x = 0; x < width; ++x) {
							uint8_t r, g, b, a;
							SDL_GetRGBA
								(frame.get_pixel(x,         height - 1),
								 surface.format,
								 &r, &g, &b, &a);
							if (a) {
								data_in_y_max = true;
								break;
							}
						}
#endif
				} catch (std::exception const & e) {
					throw wexception
						("could not load animation frame %s: %s\n",
						 filename, e.what());
				}
				//  Successfully loaded the frame.
				break;  //  No need to look for files with other extensions.
			} else if (++extnr == nextensions) //  Tried all extensions.
				goto end;  //  This frame does not exist. No more frames in anim.
		}

		if (m_hasplrclrs) {
			//TODO Do not load playercolor mask as opengl texture or use it as
			//     opengl texture.
			strcpy(after_basename, "_pc");
			for (size_t extnr = 0;;) {
				strcpy(after_basename + 3, extensions[extnr]);
				if (g_fs->FileExists(filename)) {
					try {
						PictureID picture = g_gr->load_image(filename, true);
						if (width != picture->get_w() or height != picture->get_h())
							throw wexception
								("playercolor mask has wrong size: (%u, %u), should "
								 "be (%u, %u) like the animation frame",
								 picture->get_w(), picture->get_h(), width, height);
						m_pcmasks.push_back(picture);
						break;
					} catch (std::exception const & e) {
						throw wexception
							("error while reading \"%s\": %s", filename, e.what());
					}
				} else if (++extnr == nextensions) {
					after_basename[3] = '\0'; //  cut off the extension
					throw wexception("\"%s\" is missing", filename);
				}
			}
		}

		//  Increment the number in the filename.
		for (char * digit_to_increment = last_digit;;) {
			if (digit_to_increment == before_first_digit)
				goto end; //  The number wrapped around to all zeros.
			assert('0' <= *digit_to_increment);
			assert        (*digit_to_increment <= '9');
			if (*digit_to_increment == '9') {
				*digit_to_increment = '0';
				--digit_to_increment;
			} else {
				++*digit_to_increment;
				break;
			}
		}
	}
end:
	if (m_plrframes[0].empty())
		throw wexception
			("animation %s has no frames", data->picnametempl.c_str());

	if (m_pcmasks.size() and m_pcmasks.size() < m_plrframes[0].size())
		throw wexception
			("animation has %zu frames but playercolor mask has only %zu frames",
			 m_plrframes[0].size(), m_pcmasks.size());
#ifdef VALIDATE_ANIMATION_CROPPING
	if
		(char const * const where =
		 	not data_in_x_min ? "left column"  :
		 	not data_in_x_max ? "right column" :
		 	not data_in_y_min ? "top row"      :
		 	not data_in_y_max ? "bottom row"   :
		 	0)
		log
			("The animation %s is not properly cropped (the %s has only fully "
			 "transparent pixels in each frame. Therefore the %s should be "
			 "removed (and the hotspot adjusted accordingly). Otherwise "
			 "rendering will be slowed down by needless painting of fully "
			 "transparent pixels.\n",
			 data->picnametempl.c_str(), where, where);
#endif
}


AnimationGfx::~AnimationGfx()
{
}


/*
===============
Encodes the given surface into a frame
===============
*/
void AnimationGfx::encode(uint8_t const plr, RGBColor const * const plrclrs)
{
	assert(m_plrframes[0].size() == m_pcmasks.size());
	std::vector<PictureID> & frames = m_plrframes[plr];

	for (uint32_t i = 0; i < m_plrframes[0].size(); ++i) {
		//  Copy the old surface.
		PictureID origpic = m_plrframes[0][i];
		uint32_t w = origpic->get_w();
		uint32_t h = origpic->get_h();
		IPixelAccess & origpix = m_plrframes[0][i]->pixelaccess();
		IPixelAccess & pcmask = m_pcmasks[i]->pixelaccess();

		PictureID newpicture = g_gr->create_picture(w, h, true);
		IPixelAccess & newpix = newpicture->pixelaccess();

		const SDL_PixelFormat & fmt = origpix.format();
		const SDL_PixelFormat & fmt_pc = pcmask.format();

		origpix.lock();
		pcmask.lock();
		newpix.lock();
		// This could be done significantly faster, but since we
		// cache the result, let's keep it simple for now.
		for (uint32_t y = 0; y < h; ++y) {
			for (uint32_t x = 0; x < w; ++x) {
				RGBAColor source;
				RGBAColor mask;

				source.set(fmt, origpix.get_pixel(x, y));
				mask.set(fmt_pc, pcmask.get_pixel(x, y));

				if
					(uint32_t const influence =
					 	static_cast<uint32_t>(mask.r) * mask.a)
				{
					uint32_t const intensity =
						(luminance_table_r[source.r] +
						 luminance_table_g[source.g] +
						 luminance_table_b[source.b] +
						 8388608U) //  compensate for truncation:  .5 * 2^24
						>> 24;
					RGBAColor plrclr;

					plrclr.r = (plrclrs[3].r() * intensity) >> 8;
					plrclr.g = (plrclrs[3].g() * intensity) >> 8;
					plrclr.b = (plrclrs[3].b() * intensity) >> 8;

					RGBAColor dest(source);
					dest.r =
						(plrclr.r * influence + dest.r * (65536 - influence)) >> 16;
					dest.g =
						(plrclr.g * influence + dest.g * (65536 - influence)) >> 16;
					dest.b =
						(plrclr.b * influence + dest.b * (65536 - influence)) >> 16;

					newpix.set_pixel(x, y, dest.map(fmt));
				} else {
					newpix.set_pixel(x, y, source.map(fmt));
				}
			}
		}
		origpix.unlock();
		pcmask.unlock();
		newpix.unlock();

		frames.push_back(newpicture);
	}
}

