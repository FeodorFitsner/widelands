/*
 * Copyright (C) 2006-2015 by the Widelands Development Team
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

#ifndef WL_GRAPHIC_GL_DRAW_LINE_PROGRAM_H
#define WL_GRAPHIC_GL_DRAW_LINE_PROGRAM_H

#include "graphic/color.h"
#include "graphic/gl/utils.h"

class DrawLineProgram {
public:
	// Returns the (singleton) instance of this class.
	static DrawLineProgram& instance();

	// Draws a line from (x1, y1) to (x2, y2) which are in gl
	// coordinates in 'color' with a 'line_width' in pixels.
	void draw(float x1, float y1, float x2, float y2, const RGBColor& color, int line_width);

private:
	DrawLineProgram();

	struct PerVertexData {
		float gl_x, gl_y;
	};
	static_assert(sizeof(PerVertexData) == 8, "Wrong padding.");

	// The buffer that contains the vertices for rendering.
	Gl::Buffer gl_array_buffer_;

	// The program.
	Gl::Program gl_program_;

	// Attributes.
	GLint attr_position_;

	// Uniforms.
	GLint u_color_;

	DISALLOW_COPY_AND_ASSIGN(DrawLineProgram);
};

#endif  // end of include guard: WL_GRAPHIC_GL_DRAW_LINE_PROGRAM_H
