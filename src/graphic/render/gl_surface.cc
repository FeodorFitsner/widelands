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

#include <cmath>

#include "gl_surface_texture.h"
#include "gl_utils.h"
#include "graphic/graphic.h"
#include "upcast.h"

#include "gl_surface.h"

uint32_t GLSurface::get_w() const {
	return m_w;
}

uint32_t GLSurface::get_h() const {
	return m_h;
}

const SDL_PixelFormat & GLSurface::format() const {
	return gl_rgba_format();
}


uint8_t * GLSurface::get_pixels() const
{
	return m_pixels.get();
}

uint32_t GLSurface::get_pixel(uint32_t x, uint32_t y) {
	assert(m_pixels);
	assert(x < m_w);
	assert(y < m_h);

	uint8_t * data = &m_pixels[y * get_pitch() + 4 * x];
	return *(reinterpret_cast<uint32_t *>(data));
}

void GLSurface::set_pixel(uint32_t x, uint32_t y, uint32_t clr) {
	assert(m_pixels);
	assert(x < m_w);
	assert(y < m_h);

	uint8_t * data = &m_pixels[y * get_pitch() + 4 * x];
	*(reinterpret_cast<uint32_t *>(data)) = clr;
}

/**
 * Draws the outline of a rectangle
 */
void GLSurface::draw_rect(const Rect& rc, const RGBColor clr)
{
	assert(g_opengl);
	setup_gl();

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(1);

	glBegin(GL_LINE_LOOP); {
		glColor3ub(clr.r(), clr.g(), clr.b());
		glVertex2f(rc.x + 0.5f,        rc.y + 0.5f);
		glVertex2f(rc.x + rc.w - 0.5f, rc.y + 0.5f);
		glVertex2f(rc.x + rc.w - 0.5f, rc.y + rc.h - 0.5f);
		glVertex2f(rc.x + 0.5f,        rc.y + rc.h - 0.5f);
	} glEnd();
	glEnable(GL_TEXTURE_2D);

	reset_gl();
}


/**
 * Draws a filled rectangle
 */
void GLSurface::fill_rect(const Rect& rc, const RGBAColor clr) {
	assert(rc.x >= 0);
	assert(rc.y >= 0);
	assert(rc.w >= 0);
	assert(rc.h >= 0);
	assert(g_opengl);
	setup_gl();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	glBegin(GL_QUADS); {
		glColor4ub(clr.r, clr.g, clr.b, clr.a);
		glVertex2f(rc.x,        rc.y);
		glVertex2f(rc.x + rc.w, rc.y);
		glVertex2f(rc.x + rc.w, rc.y + rc.h);
		glVertex2f(rc.x,        rc.y + rc.h);
	} glEnd();
	glEnable(GL_TEXTURE_2D);

	reset_gl();
}

/**
 * Change the brightness of the given rectangle
 */
void GLSurface::brighten_rect(const Rect& rc, const int32_t factor)
{
	if (!factor)
		return;

	assert(rc.x >= 0);
	assert(rc.y >= 0);
	assert(rc.w >= 1);
	assert(rc.h >= 1);
	assert(g_opengl);

	if (factor < 0) {
		if (!g_gr->caps().gl.blendequation)
			return;

		glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
	}

	setup_gl();

	/* glBlendFunc is a very nice feature of opengl. You can specify how the
	* color is calculated.
	*
	* glBlendFunc(GL_ONE, GL_ONE) means the following:
	* Rnew = Rdest + Rsrc
	* Gnew = Gdest + Gsrc
	* Bnew = Bdest + Bsrc
	* Anew = Adest + Asrc
	* where Xnew is the new calculated color for destination, Xdest is the old
	* color of the destination and Xsrc is the color of the source.
	*/
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_ONE, GL_ONE);

	// And now simply draw a rect with factor as the color
	// (this is the source color) over the region
	glBegin(GL_QUADS); {
		glColor4f
			((fabsf(factor) / 256.0f),
			 (fabsf(factor) / 256.0f),
			 (fabsf(factor) / 256.0f),
			 0);
		glVertex2f(rc.x,        rc.y);
		glVertex2f(rc.x + rc.w, rc.y);
		glVertex2f(rc.x + rc.w, rc.y + rc.h);
		glVertex2f(rc.x,        rc.y + rc.h);
	} glEnd();

	if (factor < 0)
		glBlendEquation(GL_FUNC_ADD);

	reset_gl();
}

void GLSurface::draw_line (int32_t x1, int32_t y1, int32_t x2, int32_t
		y2, const RGBColor& color, uint8_t width)
{
	setup_gl();

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glLineWidth(width);
	glBegin(GL_LINES); {
		glColor3ub(color.r(), color.g(), color.b());
		glVertex2f(x1 + 0.5f, y1 + 0.5f);
		glVertex2f(x2 + 0.5f, y2 + 0.5f);
	} glEnd();

	reset_gl();
}

void GLSurface::blit
	(const Point& dst, const IPicture* src, const Rect& srcrc, Composite cm)
{
	upcast(const GLSurfaceTexture, const_oglsrc, src);
	assert(const_oglsrc);
	GLSurfaceTexture* oglsrc = const_cast<GLSurfaceTexture*>(const_oglsrc);
	assert(g_opengl);

	setup_gl();

	/* Set a texture scaling factor. Normaly texture coordiantes
	* (see glBegin()...glEnd() Block below) are given in the range 0-1
	* to avoid the calculation (and let opengl do it) the texture
	* space is modified. glMatrixMode select which matrixconst  to manipulate
	* (the texture transformation matrix in this case). glLoadIdentity()
	* resets the (selected) matrix to the identity matrix. And finally
	* glScalef() calculates the texture matrix.
	*/
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef
		(1.0f / static_cast<GLfloat>(oglsrc->get_tex_w()),
		 1.0f / static_cast<GLfloat>(oglsrc->get_tex_h()), 1);

	// Enable Alpha blending
	if (cm == CM_Normal) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		glDisable(GL_BLEND);
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, oglsrc->get_gl_texture());

	glBegin(GL_QUADS); {
		//  set color white, otherwise textures get mixed with color
		glColor3f(1.0, 1.0, 1.0);
		//  top-left
		glTexCoord2i(srcrc.x,           srcrc.y);
		glVertex2i  (dst.x,             dst.y);
		//  top-right
		glTexCoord2i(srcrc.x + srcrc.w, srcrc.y);
		glVertex2f  (dst.x + srcrc.w,   dst.y);
		//  bottom-right
		glTexCoord2i(srcrc.x + srcrc.w, srcrc.y + srcrc.h);
		glVertex2f  (dst.x + srcrc.w,   dst.y + srcrc.h);
		//  bottom-left
		glTexCoord2i(srcrc.x,           srcrc.y + srcrc.h);
		glVertex2f  (dst.x,             dst.y + srcrc.h);
	} glEnd();

	glLoadIdentity();

	reset_gl();
}


