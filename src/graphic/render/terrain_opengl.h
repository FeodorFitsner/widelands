/*
 * Copyright (C) 2010-2012 by the Widelands Development Team
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

#ifdef USE_OPENGL
#ifndef TERRAIN_OPENGL_H
#define TERRAIN_OPENGL_H

#include "gl_picture_texture.h"
#include "vertex.h"


void draw_field_opengl
	(Rect const & subwin,
	 Vertex const & p1,
	 Vertex const & p2,
	 Vertex const & p3,
	 Texture const & texture,
	 Texture const & left_texture,
	 Texture const & top_texture)
{
	if (p1.b <= -128 and p2.b <= -128 and p3.b <= -128)
		return;

	Vertex t1(p1), t2(p2), t3(p3);

	t1.x = p1.x + subwin.x;
	t1.y = p1.y + subwin.y;
	t2.x = p2.x + subwin.x;
	t2.y = p2.y + subwin.y;
	t3.x = p3.x + subwin.x;
	t3.y = p3.y + subwin.y;

	if (t1.x < subwin.x and t2.x < subwin.x and t3.x < subwin.x)
		return;
	if (t1.y < subwin.y and t2.y < subwin.y and t3.y < subwin.y)
		return;

	int const subxr = subwin.x + subwin.w;
	if (t1.x > subxr and t2.x > subxr and t3.x > subxr)
		return;

	int const subyd = subwin.y + subwin.h;
	if (t1.y > subyd and t2.y > subyd and t3.y > subyd)
		return;

	glEnable(GL_BLEND);

	// load current texture
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture.getTexture());
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	if ((&top_texture != &texture) and not (p1.b == -128 and p3.b == -128)) {
		// load top texture
		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, top_texture.getTexture());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		GLuint edge = dynamic_cast<GLPictureTexture const &>
			(*g_gr->get_edge_texture()).get_gl_texture();

		// combine current and top texture
		glActiveTexture(GL_TEXTURE2);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, edge);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR);
	}

	if ((&left_texture != &texture) and not (p1.b == -128 and p2.b == -128)) {
		// load left texture
		glActiveTexture(GL_TEXTURE3);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, left_texture.getTexture());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		GLuint edge = dynamic_cast<GLPictureTexture const &>
			(*g_gr->get_edge_texture()).get_gl_texture();

		// combine current and left texture
		glActiveTexture(GL_TEXTURE4);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, edge);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE3);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_COLOR);
	}

	// Fade effect for fog of war
	glActiveTexture(GL_TEXTURE5);
	glEnable(GL_TEXTURE_2D);
	// texture does not matter but one has to be bound
	glBindTexture(GL_TEXTURE_2D, texture.getTexture());
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

	glBegin(GL_TRIANGLES); {
		{
			GLfloat const brightness = (150.0 + p1.b) / 150.0;
			glColor3f(brightness, brightness, brightness);
		}
		glMultiTexCoord2i(GL_TEXTURE0, t1.tx, t1.ty);
		glMultiTexCoord2f(GL_TEXTURE1, t1.tx / TEXTURE_WIDTH, t1.ty / TEXTURE_WIDTH);
		glMultiTexCoord2f(GL_TEXTURE2, 0, 0);
		glMultiTexCoord2f(GL_TEXTURE3, t1.tx / TEXTURE_WIDTH, t1.ty / TEXTURE_WIDTH);
		glMultiTexCoord2f(GL_TEXTURE4, 1, 0);
		glMultiTexCoord2f(GL_TEXTURE5, t1.tx / TEXTURE_WIDTH, t1.ty / TEXTURE_WIDTH);
		glVertex2f(t1.x, t1.y);

		{
			GLfloat const brightness = (150.0 + p2.b) / 150.0;
			glColor3f(brightness, brightness, brightness);
		}
		glMultiTexCoord2i(GL_TEXTURE0, t2.tx, t2.ty);
		glMultiTexCoord2f(GL_TEXTURE1, t2.tx / TEXTURE_WIDTH, t2.ty / TEXTURE_WIDTH);
		glMultiTexCoord2f(GL_TEXTURE2, 0.5, 1);
		glMultiTexCoord2f(GL_TEXTURE3, t2.tx / TEXTURE_WIDTH, t2.ty / TEXTURE_WIDTH);
		glMultiTexCoord2f(GL_TEXTURE4, 0, 0);
		glMultiTexCoord2f(GL_TEXTURE5, t2.tx / TEXTURE_WIDTH, t2.ty / TEXTURE_WIDTH);
		glVertex2f(t2.x, t2.y);

		{
			GLfloat const brightness = (150.0 + p3.b) / 150.0;
			glColor3f(brightness, brightness, brightness);
		}
		glMultiTexCoord2i(GL_TEXTURE0, t3.tx, t3.ty);
		glMultiTexCoord2f(GL_TEXTURE1, t3.tx / TEXTURE_WIDTH, t3.ty / TEXTURE_WIDTH);
		glMultiTexCoord2f(GL_TEXTURE2, 1, 0);
		glMultiTexCoord2f(GL_TEXTURE3, t3.tx / TEXTURE_WIDTH, t3.ty / TEXTURE_WIDTH);
		glMultiTexCoord2f(GL_TEXTURE4, 0.5, 1);
		glMultiTexCoord2f(GL_TEXTURE5, t3.tx / TEXTURE_WIDTH, t3.ty / TEXTURE_WIDTH);
		glVertex2f(t3.x, t3.y);
	} glEnd();

	// Cleanup OpenGL
	glActiveTexture(GL_TEXTURE5);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glActiveTexture(GL_TEXTURE4);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glActiveTexture(GL_TEXTURE3);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glActiveTexture(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

#define horiz_road_opengl(tex, start, end)                                    \
   glBindTexture( GL_TEXTURE_2D, tex);                                        \
   glBegin(GL_QUADS); {                                                       \
      glTexCoord2i(0, 0);                                                     \
      glVertex2f(subwin.x + start.x, subwin.y + start.y - 2);                 \
      glTexCoord2i(TEXTURE_WIDTH, 0);                                         \
      glVertex2f(subwin.x + end.x,   subwin.y + end.y - 2);                   \
      glTexCoord2i(TEXTURE_WIDTH, 4);                                         \
      glVertex2f(subwin.x + end.x,   subwin.y + end.y + 2);                   \
      glTexCoord2i(0, 4);                                                     \
      glVertex2f(subwin.x + start.x, subwin.y + start.y + 2);                 \
   } glEnd();                                                                 \

#define vert_road_opengl(tex, start, end)                                     \
   glBindTexture( GL_TEXTURE_2D, tex);                                        \
   glBegin(GL_QUADS); {                                                       \
      glTexCoord2i(0, 0);                                                     \
      glVertex2f(subwin.x + start.x - 3, subwin.y + start.y);                 \
      glTexCoord2i(6, 0);                                                     \
      glVertex2f(subwin.x + start.x + 3, subwin.y + start.y);                 \
      glTexCoord2i(TEXTURE_WIDTH, TEXTURE_HEIGHT);                            \
      glVertex2f(subwin.x + end.x + 3,   subwin.y + end.y);                   \
      glTexCoord2i(TEXTURE_WIDTH - 6, TEXTURE_HEIGHT);                        \
      glVertex2f(subwin.x + end.x - 3,   subwin.y + end.y);                   \
   } glEnd();                                                                 \

void draw_roads_opengl
	(Rect & subwin,
	 uint8_t roads,
	 Vertex const & f_vert,
	 Vertex const & r_vert,
	 Vertex const & bl_vert,
	 Vertex const & br_vert)
{
	uint8_t road;

	GLuint rt_normal =
		dynamic_cast<GLPictureTexture const &>
		(*g_gr->get_road_texture(Widelands::Road_Normal)).get_gl_texture();
	GLuint rt_busy   =
		dynamic_cast<GLPictureTexture const &>
		(*g_gr->get_road_texture(Widelands::Road_Busy)).get_gl_texture();

	glDisable(GL_BLEND);
	glColor4f(1.0f, 1.0f, 1.0f, 0.6f);

	road = (roads >> Widelands::Road_East) & Widelands::Road_Mask;
	if ((-128 < f_vert.b or -128 < r_vert.b) and road) {
		switch (road) {
		case Widelands::Road_Normal:
			horiz_road_opengl(rt_normal, f_vert, r_vert)
			break;
		case Widelands::Road_Busy:
			horiz_road_opengl(rt_busy, f_vert, r_vert)
			break;
		default:
			assert(false);
		}
	}

	road = (roads >> Widelands::Road_SouthEast) & Widelands::Road_Mask;
	if ((-128 < f_vert.b or -128 < br_vert.b) and road) {
		switch (road) {
		case Widelands::Road_Normal:
			vert_road_opengl(rt_normal, f_vert, br_vert);
			break;
		case Widelands::Road_Busy:
			vert_road_opengl(rt_busy, f_vert, br_vert);
			break;
		default:
			assert(false);
		}
	}

	road = (roads >> Widelands::Road_SouthWest) & Widelands::Road_Mask;
	if ((-128 < f_vert.b or -128 < bl_vert.b) and road) {
		switch (road) {
		case Widelands::Road_Normal:
			vert_road_opengl(rt_normal, f_vert, bl_vert);
			break;
		case Widelands::Road_Busy:
			vert_road_opengl(rt_busy, f_vert, bl_vert);
			break;
		default:
			assert(false);
		}
	}
}

#endif
#endif
