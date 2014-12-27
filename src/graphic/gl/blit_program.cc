/*
 * Copyright (C) 2006-2014 by the Widelands Development Team
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

#include "graphic/gl/blit_program.h"

#include <vector>

#include "base/log.h"
#include "graphic/gl/utils.h"

namespace  {

const char kBlitVertexShader[] = R"(
#version 120

// Attributes.
attribute vec3 attr_position;
attribute vec2 attr_texture_position;
attribute vec4 attr_blend;

varying vec2 out_texture_coordinate;
varying vec4 out_blend;

void main() {
	out_texture_coordinate = attr_texture_position;
	out_blend = attr_blend;
	gl_Position = vec4(attr_position, 1.);
}
)";

const char kVanillaBlitFragmentShader[] = R"(
#version 120

uniform sampler2D u_texture;

varying vec2 out_texture_coordinate;
varying vec4 out_blend;

void main() {
	vec4 color = texture2D(u_texture, out_texture_coordinate);
	gl_FragColor = color * out_blend;
}
)";

const char kMonochromeBlitFragmentShader[] = R"(
#version 120

uniform sampler2D u_texture;

varying vec2 out_texture_coordinate;
varying vec4 out_blend;

void main() {
	vec4 texture_color = texture2D(u_texture, out_texture_coordinate);

	// See http://en.wikipedia.org/wiki/YUV.
	float luminance = dot(vec3(0.299, 0.587, 0.114), texture_color.rgb);

	gl_FragColor = vec4(vec3(luminance) * out_blend.rgb, out_blend.a * texture_color.a);
}
)";

const char kBlendedBlitFragmentShader[] = R"(
#version 120

uniform sampler2D u_texture;
uniform sampler2D u_mask;

varying vec2 out_texture_coordinate;
varying vec4 out_blend;

void main() {
	vec4 texture_color = texture2D(u_texture, out_texture_coordinate);
	vec4 mask_color = texture2D(u_mask, out_texture_coordinate);

	// See http://en.wikipedia.org/wiki/YUV.
	float luminance = dot(vec3(0.299, 0.587, 0.114), texture_color.rgb);
	float blend_influence = mask_color.r * mask_color.a;
	gl_FragColor = vec4(
	   mix(texture_color.rgb, out_blend.rgb * luminance, blend_influence), out_blend.a * texture_color.a);
}
)";

}  // namespace

class BlitProgram {
public:
	// NOCOM(#sirver): document these.
	struct Arguments {
		FloatRect destination_rect;
		FloatRect source_rect;
		float z_value;
		int texture;
		int texture_mask;
		RGBAColor blend;
		BlendMode blend_mode;
	};
	BlitProgram(const std::string& fragment_shader);

	void activate();

	void draw_and_deactivate(const std::vector<Arguments>& arguments);

	int program_object() const {
		return gl_program_.object();
	}

private:
	struct PerVertexData {
		PerVertexData(float init_gl_x,
		              float init_gl_y,
		              float init_gl_z,
		              float init_texture_x,
		              float init_texture_y,
		              float init_blend_r,
		              float init_blend_g,
		              float init_blend_b,
		              float init_blend_a)
		   : gl_x(init_gl_x),
		     gl_y(init_gl_y),
		     gl_z(init_gl_z),
		     texture_x(init_texture_x),
		     texture_y(init_texture_y),
		     blend_r(init_blend_r),
		     blend_g(init_blend_g),
		     blend_b(init_blend_b),
		     blend_a(init_blend_a) {
		}

		float gl_x, gl_y, gl_z;
		float texture_x, texture_y;
		float blend_r, blend_g, blend_b, blend_a;
	};
	static_assert(sizeof(PerVertexData) == 36, "Wrong padding.");

	// The buffer that will contain the quad for rendering.
	Gl::Buffer gl_array_buffer_;

	// The program.
	Gl::Program gl_program_;

	// Attributes.
	GLint attr_blend_;
	GLint attr_position_;
	GLint attr_texture_position_;

	// Uniforms.
	GLint u_texture_;
	GLint u_mask_;

	DISALLOW_COPY_AND_ASSIGN(BlitProgram);
};

BlitProgram::BlitProgram(const std::string& fragment_shader) {
	gl_program_.build(kBlitVertexShader, fragment_shader.c_str());

	attr_blend_ = glGetAttribLocation(gl_program_.object(), "attr_blend");
	attr_position_ = glGetAttribLocation(gl_program_.object(), "attr_position");
	attr_texture_position_ = glGetAttribLocation(gl_program_.object(), "attr_texture_position");

	u_texture_ = glGetUniformLocation(gl_program_.object(), "u_texture");
	u_mask_ = glGetUniformLocation(gl_program_.object(), "u_mask");
}

void BlitProgram::activate() {
	glUseProgram(gl_program_.object());

	glEnableVertexAttribArray(attr_blend_);
	glEnableVertexAttribArray(attr_position_);
	glEnableVertexAttribArray(attr_texture_position_);
}

void BlitProgram::draw_and_deactivate(const std::vector<Arguments>& arguments) {
	size_t i = 0;

	// NOCOM(#sirver): make member.
	std::vector<PerVertexData> vertices;

	glBindBuffer(GL_ARRAY_BUFFER, gl_array_buffer_.object());

	while (i < arguments.size()) {
		const Arguments& template_args = arguments[i];
		vertices.clear();

		// Batch common blit operations up.
		while (i < arguments.size()) {
			const Arguments& current_args = arguments[i];
			if (current_args.blend_mode != template_args.blend_mode ||
			    current_args.texture != template_args.texture ||
				 current_args.texture_mask != template_args.texture_mask) {
				break;
			}

			vertices.emplace_back(current_args.destination_rect.x,
			                      current_args.destination_rect.y,
			                      current_args.z_value,
			                      current_args.source_rect.x,
			                      current_args.source_rect.y,
			                      current_args.blend.r / 255.,
			                      current_args.blend.g / 255.,
			                      current_args.blend.b / 255.,
			                      current_args.blend.a / 255.);

			vertices.emplace_back(current_args.destination_rect.x + current_args.destination_rect.w,
			                      current_args.destination_rect.y,
			                      current_args.z_value,
			                      current_args.source_rect.x + current_args.source_rect.w,
			                      current_args.source_rect.y,
			                      current_args.blend.r / 255.,
			                      current_args.blend.g / 255.,
			                      current_args.blend.b / 255.,
			                      current_args.blend.a / 255.);

			vertices.emplace_back(current_args.destination_rect.x,
			                      current_args.destination_rect.y + current_args.destination_rect.h,
			                      current_args.z_value,
			                      current_args.source_rect.x,
			                      current_args.source_rect.y + current_args.source_rect.h,
			                      current_args.blend.r / 255.,
			                      current_args.blend.g / 255.,
			                      current_args.blend.b / 255.,
			                      current_args.blend.a / 255.);

			vertices.emplace_back(vertices.at(vertices.size() - 2));
			vertices.emplace_back(vertices.at(vertices.size() - 2));

			vertices.emplace_back(current_args.destination_rect.x + current_args.destination_rect.w,
			                      current_args.destination_rect.y + current_args.destination_rect.h,
			                      current_args.z_value,
			                      current_args.source_rect.x + current_args.source_rect.w,
			                      current_args.source_rect.y + current_args.source_rect.h,
			                      current_args.blend.r / 255.,
			                      current_args.blend.g / 255.,
			                      current_args.blend.b / 255.,
			                      current_args.blend.a / 255.);
			++i;
		}

		glBufferData(GL_ARRAY_BUFFER,
		             sizeof(PerVertexData) * vertices.size(),
		             vertices.data(),
		             GL_DYNAMIC_DRAW);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, template_args.texture);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, template_args.texture_mask);

		glUniform1i(u_texture_, 0);
		glUniform1i(u_mask_, 1);

		const auto set_attrib_pointer = [](const int vertex_index, int num_items, int offset) {
			glVertexAttribPointer(vertex_index,
			                      num_items,
			                      GL_FLOAT,
			                      GL_FALSE,
			                      sizeof(PerVertexData),
			                      reinterpret_cast<void*>(offset));
		};
		set_attrib_pointer(attr_position_, 3, offsetof(PerVertexData, gl_x));
		set_attrib_pointer(attr_texture_position_, 2, offsetof(PerVertexData, texture_x));
		set_attrib_pointer(attr_blend_, 4, offsetof(PerVertexData, blend_r));


		if (template_args.blend_mode == BlendMode::Copy) {
			glBlendFunc(GL_ONE, GL_ZERO);
		}

		log("#sirver       BlitProgram: vertices.size(): %ld\n", vertices.size());
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		if (template_args.blend_mode == BlendMode::Copy) {
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
	}

	glDisableVertexAttribArray(attr_blend_);
	glDisableVertexAttribArray(attr_position_);
	glDisableVertexAttribArray(attr_texture_position_);

	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// static
VanillaBlitProgram& VanillaBlitProgram::instance() {
	static VanillaBlitProgram blit_program;
	return blit_program;
}

VanillaBlitProgram::~VanillaBlitProgram() {
}

VanillaBlitProgram::VanillaBlitProgram() {
	blit_program_.reset(new BlitProgram(kVanillaBlitFragmentShader));
}

void VanillaBlitProgram::draw(const FloatRect& gl_dest_rect,
                              const FloatRect& gl_src_rect,
                              const float z_value,
                              const int texture,
                              const float opacity,
                              const BlendMode blend_mode) {
	draw({Arguments{gl_dest_rect, gl_src_rect, z_value, texture, opacity, blend_mode}});
}

void VanillaBlitProgram::draw(const std::vector<Arguments>& arguments) {
	std::vector<BlitProgram::Arguments> blit_arguments;
	for (const Arguments arg : arguments) {
		blit_arguments.emplace_back(BlitProgram::Arguments{
		   arg.destination_rect,
		   arg.source_rect,
		   arg.z_value,
		   arg.texture,
			0,
		   RGBAColor(255, 255, 255, arg.opacity * 255),
		   arg.blend_mode,
		});
	}

	blit_program_->activate();
	blit_program_->draw_and_deactivate(blit_arguments);
}


// static
MonochromeBlitProgram& MonochromeBlitProgram::instance() {
	static MonochromeBlitProgram blit_program;
	return blit_program;
}

MonochromeBlitProgram::~MonochromeBlitProgram() {
}

MonochromeBlitProgram::MonochromeBlitProgram() {
	blit_program_.reset(new BlitProgram(kMonochromeBlitFragmentShader));
}

void MonochromeBlitProgram::draw(const FloatRect& gl_dest_rect,
                                 const FloatRect& gl_src_rect,
                                 const float z_value,
                                 const int texture,
                                 const RGBAColor& blend) {
	draw({Arguments{gl_dest_rect, gl_src_rect, z_value, texture, blend, BlendMode::UseAlpha}});
}

void MonochromeBlitProgram::draw(const std::vector<Arguments>& arguments) {
	std::vector<BlitProgram::Arguments> blit_arguments;
	for (const Arguments arg : arguments) {
		blit_arguments.emplace_back(BlitProgram::Arguments{
		   arg.destination_rect,
		   arg.source_rect,
		   arg.z_value,
		   arg.texture,
			0,
		   arg.blend,
		   arg.blend_mode,
		});
	}

	blit_program_->activate();
	blit_program_->draw_and_deactivate(blit_arguments);
}

// static
BlendedBlitProgram& BlendedBlitProgram::instance() {
	static BlendedBlitProgram blit_program;
	return blit_program;
}

BlendedBlitProgram::~BlendedBlitProgram() {
}

BlendedBlitProgram::BlendedBlitProgram() {
	blit_program_.reset(new BlitProgram(kBlendedBlitFragmentShader));
}

void BlendedBlitProgram::draw(const FloatRect& gl_dest_rect,
                              const FloatRect& gl_src_rect,
                              const float z_value,
                              const int texture_image,
                              const int texture_mask,
                              const RGBAColor& blend) {
	draw({Arguments{gl_dest_rect,
	                gl_src_rect,
	                z_value,
	                texture_image,
	                texture_mask,
	                blend,
	                BlendMode::UseAlpha}});
}

void BlendedBlitProgram::draw(const std::vector<Arguments>& arguments) {
	std::vector<BlitProgram::Arguments> blit_arguments;
	for (const Arguments arg : arguments) {
		blit_arguments.emplace_back(BlitProgram::Arguments{
		   arg.destination_rect,
		   arg.source_rect,
		   arg.z_value,
		   arg.texture,
			arg.texture_mask,
		   arg.blend,
		   arg.blend_mode,
		});
	}

	blit_program_->activate();
	blit_program_->draw_and_deactivate(blit_arguments);
}
