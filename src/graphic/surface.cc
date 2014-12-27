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

#include "graphic/surface.h"

#include <cassert>
#include <cmath>
#include <cstdlib>

#include <SDL.h>

#include "base/macros.h"
#include "graphic/gl/blit_program.h"
#include "graphic/gl/draw_line_program.h"
#include "graphic/gl/fill_rect_program.h"
#include "graphic/gl/utils.h"
#include "graphic/graphic.h"
#include "graphic/render_queue.h"
#include "graphic/screen.h"
#include "graphic/texture.h"

// NOCOM(#sirver): remove is_a from in here

namespace  {

// Convert the 'rect' in pixel space into opengl space.
enum class ConversionMode {
	// Convert the rect as given.
	kExact,

	// Convert the rect so that the borders are in the center
	// of the pixels.
	kMidPoint,
};

FloatRect to_opengl(const Surface& surface, const Rect& rect, ConversionMode mode) {
	const float delta = mode == ConversionMode::kExact ? 0. : 0.5;
	float x1 = rect.x + delta;
	float y1 = rect.y + delta;
	surface.pixel_to_gl(&x1, &y1);
	float x2 = rect.x + rect.w - delta;
	float y2 = rect.y + rect.h - delta;
	surface.pixel_to_gl(&x2, &y2);
	return FloatRect(x1, y1, x2 - x1, y2 - y1);
}

// Converts the pixel (x, y) in a texture to a gl coordinate in [0, 1].
inline void pixel_to_gl_texture(const int width, const int height, float* x, float* y) {
	*x = (*x / width);
	*y = (*y / height);
}

// Convert 'dst' and 'srcrc' from pixel space into opengl space, taking into
// account that we might be a subtexture in a bigger texture.
void src_and_dst_rect_to_gl(const Surface& surface,
                            const Image& image,
                            const Rect& dst_rect,
                            const Rect& src_rect,
                            FloatRect* gl_dst_rect,
                            FloatRect* gl_src_rect) {
	// Source Rectangle. We have to take into account that the texture might be
	// a subtexture in another bigger texture. So we first figure out the pixel
	// coordinates given it is a full texture (values between 0 and 1) and then
	// adjust these for the texture coordinates in the parent texture.
	const FloatRect& texture_coordinates = image.texture_coordinates();

	float x1 = src_rect.x;
	float y1 = src_rect.y;
	pixel_to_gl_texture(image.width(), image.height(), &x1, &y1);
	x1 = texture_coordinates.x + x1 * texture_coordinates.w;
	y1 = texture_coordinates.y + y1 * texture_coordinates.h;

	float x2 = src_rect.x + src_rect.w;
	float y2 = src_rect.y + src_rect.h;
	pixel_to_gl_texture(image.width(), image.height(), &x2, &y2);
	x2 = texture_coordinates.x + x2 * texture_coordinates.w;
	y2 = texture_coordinates.y + y2 * texture_coordinates.h;

	gl_src_rect->x = x1;
	gl_src_rect->y = y1;
	gl_src_rect->w = x2 - x1;
	gl_src_rect->h = y2 - y1;

	*gl_dst_rect = to_opengl(surface, dst_rect, ConversionMode::kExact);
}

}  // namespace

void draw_rect(const Rect& rc, const RGBColor& clr, Surface* surface) {
	draw_line(rc.x, rc.y, rc.x + rc.w, rc.y, clr, 1, surface);
	draw_line(rc.x + rc.w, rc.y, rc.x + rc.w, rc.y + rc.h, clr, 1, surface);
	draw_line(rc.x + rc.w, rc.y + rc.h, rc.x, rc.y + rc.h, clr, 1, surface);
	draw_line(rc.x, rc.y + rc.h, rc.x, rc.y, clr, 1, surface);
}

void fill_rect(const Rect& rc, const RGBAColor& clr, Surface* surface) {
	const FloatRect rect = to_opengl(*surface, rc, ConversionMode::kExact);
	if (is_a(Screen, surface)) {
		RenderQueue::Item i;
		i.program = RenderQueue::Program::RECT;
		i.blend_mode = BlendMode::Copy;
		i.destination_rect = rect;
		i.rect_arguments.color = clr;
		RenderQueue::instance().enqueue(i);
		return;
	}

	surface->setup_gl();
	glViewport(0, 0, surface->width(), surface->height());
	FillRectProgram::instance().draw(rect, 0.f, clr, BlendMode::Copy);
}

void brighten_rect(const Rect& rc, const int32_t factor, Surface * surface)
{
	if (!factor)
		return;

	const BlendMode blend_mode = factor < 0 ? BlendMode::Subtract : BlendMode::UseAlpha;
	const int abs_factor = std::abs(factor);
	const RGBAColor color(abs_factor, abs_factor, abs_factor, 0);

	const FloatRect rect = to_opengl(*surface, rc, ConversionMode::kExact);
	if (is_a(Screen, surface)) {
		RenderQueue::Item i;
		i.program = RenderQueue::Program::RECT;
		i.blend_mode = blend_mode;
		i.destination_rect = rect;
		i.rect_arguments.color = color;
		RenderQueue::instance().enqueue(i);
		return;
	}

	surface->setup_gl();
	glViewport(0, 0, surface->width(), surface->height());
	FillRectProgram::instance().draw(rect, 0.f, color, blend_mode);
}

void draw_line
	(int x1, int y1, int x2, int y2, const RGBColor& color, int gwidth, Surface * surface)
{
	float gl_x1 = x1 + 0.5;
	float gl_y1 = y1 + 0.5;
	surface->pixel_to_gl(&gl_x1, &gl_y1);

	float gl_x2 = x2 + 0.5;
	float gl_y2 = y2 + 0.5;
	surface->pixel_to_gl(&gl_x2, &gl_y2);

	if (is_a(Screen, surface)) {
		RenderQueue::Item i;
		i.program = RenderQueue::Program::LINE;
		i.blend_mode = BlendMode::Copy;
		i.destination_rect = FloatRect(gl_x1, gl_y1, gl_x2 - gl_x1, gl_y2 - gl_y1);
		i.line_arguments.color = color;
		i.line_arguments.line_width = gwidth;
		RenderQueue::instance().enqueue(i);
		return;
	}

	surface->setup_gl();
	glViewport(0, 0, surface->width(), surface->height());
	DrawLineProgram::instance().draw(gl_x1, gl_y1, gl_x2, gl_y2, 0.f, color, gwidth);
}

void blit_monochrome(const Rect& dst_rect,
                     const Image& image,
                     const Rect& src_rect,
                     const RGBAColor& blend,
                     Surface* surface) {
	FloatRect gl_dst_rect, gl_src_rect;
	src_and_dst_rect_to_gl(*surface, image, dst_rect, src_rect, &gl_dst_rect, &gl_src_rect);

	if (is_a(Screen, surface)) {
		RenderQueue::Item i;
		i.program = RenderQueue::Program::BLIT_MONOCHROME;
		i.blend_mode = BlendMode::UseAlpha;
		i.destination_rect = gl_dst_rect;
		i.monochrome_blit_arguments.source_rect = gl_src_rect;
		i.monochrome_blit_arguments.texture = image.get_gl_texture();
		i.monochrome_blit_arguments.blend = blend;
		RenderQueue::instance().enqueue(i);
		return;
	}

	surface->setup_gl();
	glViewport(0, 0, surface->width(), surface->height());
	MonochromeBlitProgram::instance().draw(gl_dst_rect, gl_src_rect, 0.f, image.get_gl_texture(), blend);
}

void blit_blended(const Rect& dst_rect,
                  const Image& image,
                  const Image& mask,
                  const Rect& src_rect,
                  const RGBColor& blend,
                  Surface* surface) {
	FloatRect gl_dst_rect, gl_src_rect;
	src_and_dst_rect_to_gl(*surface, image, dst_rect, src_rect, &gl_dst_rect, &gl_src_rect);

	if (is_a(Screen, surface)) {
		RenderQueue::Item i;
		i.program = RenderQueue::Program::BLIT_BLENDED;
		i.blend_mode = BlendMode::UseAlpha;
		i.destination_rect = gl_dst_rect;
		i.blended_blit_arguments.source_rect = gl_src_rect;
		i.blended_blit_arguments.texture = image.get_gl_texture();
		// NOCOM(#sirver): this must actually take a separate source rectangle.
		i.blended_blit_arguments.texture_mask = mask.get_gl_texture();
		i.blended_blit_arguments.blend = blend;
		RenderQueue::instance().enqueue(i);
		return;
	}

	surface->setup_gl();
	glViewport(0, 0, surface->width(), surface->height());
	BlendedBlitProgram::instance().draw(
	   gl_dst_rect, gl_src_rect, 0.f, image.get_gl_texture(), mask.get_gl_texture(), blend);
}

void blit(const Rect& dst_rect,
          const Image& image,
          const Rect& src_rect,
          float opacity,
          BlendMode blend_mode,
          Surface* surface) {
	FloatRect gl_dst_rect, gl_src_rect;
	src_and_dst_rect_to_gl(*surface, image, dst_rect, src_rect, &gl_dst_rect, &gl_src_rect);

	if (is_a(Screen, surface)) {
		RenderQueue::Item i;
		i.program = RenderQueue::Program::BLIT;
		i.blend_mode = blend_mode;
		i.destination_rect = gl_dst_rect;
		i.vanilla_blit_arguments.source_rect = gl_src_rect;
		i.vanilla_blit_arguments.texture = image.get_gl_texture();
		i.vanilla_blit_arguments.opacity = opacity;
		RenderQueue::instance().enqueue(i);
		return;
	}

	glViewport(0, 0, surface->width(), surface->height());
	surface->setup_gl();
	VanillaBlitProgram::instance().draw(
	   gl_dst_rect, gl_src_rect, 0.f, image.get_gl_texture(), opacity, blend_mode);
}
