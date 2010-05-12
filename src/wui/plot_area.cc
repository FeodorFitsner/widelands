/*
 * Copyright (C) 2002-2004, 2008-2009 by the Widelands Development Team
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

#include "plot_area.h"

#include "constants.h"
#include "graphic/font_handler.h"
#include "graphic/graphic.h"
#include "graphic/rendertarget.h"

#include "ui_basic/panel.h"

#include <cstdio>


/*
 * Where to draw tics
 */
static const int32_t how_many_ticks[] = {
	5,  // 15 Mins
	3,  // 30 Mins
	6,  // 1  H
	4,  // 2  H
	4,  // 4  H
	4,  // 8  H
	4,  // 16 H
};

static const int32_t max_x[] = {
	15,
	30,
	60,
	120,
	4,
	8,
	16
};

static const uint32_t time_in_ms[] = {
	15      * 60 * 1000,
	30      * 60 * 1000,
	1  * 60 * 60 * 1000,
	2  * 60 * 60 * 1000,
	4  * 60 * 60 * 1000,
	8  * 60 * 60 * 1000,
	16 * 60 * 60 * 1000,
};

#define NR_SAMPLES 30   // How many samples per diagramm when relative plotting

#define BG_PIC "pics/plot_area_bg.png"
#define LINE_COLOR RGBColor(0, 0, 0)


WUIPlot_Area::WUIPlot_Area
	(UI::Panel * const parent,
	 int32_t const x, int32_t const y, int32_t const w, int32_t const h)
:
UI::Panel (parent, x, y, w, h),
m_time    (TIME_ONE_HOUR),
m_plotmode(PLOTMODE_ABSOLUTE)
{}


/*
 * Draw this. This is the main function
 */
void WUIPlot_Area::draw(RenderTarget & dst) {

	// first, tile the background
	dst.tile
		(Rect(Point(0, 0), get_inner_w(), get_inner_h()),
		 g_gr->get_picture(PicMod_Game, BG_PIC), Point(0, 0));

	int32_t const spacing         =  5;
	int32_t const space_at_bottom = 15;
	int32_t const space_at_right  =  5;

	float const xline_length = get_inner_w() - space_at_right  - spacing;
	float const yline_length = get_inner_h() - space_at_bottom - spacing;

	// Draw coordinate system
	// X Axis
	dst.draw_line
		(spacing,                        get_inner_h() - space_at_bottom,
		 get_inner_w() - space_at_right, get_inner_h() - space_at_bottom,
		 LINE_COLOR);
	// Arrow
	dst.draw_line
		(spacing,     get_inner_h() - space_at_bottom,
		 spacing + 5, get_inner_h() - space_at_bottom - 3,
		 LINE_COLOR);
	dst.draw_line
		(spacing,     get_inner_h() - space_at_bottom,
		 spacing + 5, get_inner_h() - space_at_bottom + 3,
		 LINE_COLOR);
	//  Y Axis
	dst.draw_line
		(get_inner_w() - space_at_right, spacing,
		 get_inner_w() - space_at_right,
		 get_inner_h() - space_at_bottom,
		 LINE_COLOR);
	//  No Arrow here, since this doesn't continue.

	//  draw xticks
	float sub = xline_length / how_many_ticks[m_time];
	float posx = get_inner_w() - space_at_right;
	char buffer[200];
	for (int32_t i = 0; i <= how_many_ticks[m_time]; ++i) {
		dst.draw_line
			(static_cast<int32_t>(posx), get_inner_h() - space_at_bottom,
			 static_cast<int32_t>(posx), get_inner_h() - space_at_bottom + 3,
			 LINE_COLOR);

		snprintf
			(buffer, sizeof(buffer),
			 "%u", max_x[m_time] / how_many_ticks[m_time] * i);

		UI::g_fh->draw_string
			(dst,
			 UI_FONT_SMALL,
			 RGBColor(255, 0, 0), RGBColor(255, 255, 255),
			 Point
			 	(static_cast<int32_t>(posx),
			 	 get_inner_h() - space_at_bottom + 4),
			 buffer,
			 UI::Align_Center,
			 std::numeric_limits<uint32_t>::max(),
			 UI::Widget_Cache_None,
			 g_gr->get_no_picture(),
			 std::numeric_limits<uint32_t>::max(),
			 false);
		posx -= sub;
	}

	//  draw yticks, one at full, one at half
	dst.draw_line
		(get_inner_w() - space_at_right,    spacing,
		 get_inner_w() - space_at_right -3, spacing,
		 LINE_COLOR);
	dst.draw_line
		(get_inner_w() - space_at_right,
		 spacing + ((get_inner_h() - space_at_bottom) - spacing) / 2,
		 get_inner_w() - space_at_right - 3,
		 spacing + ((get_inner_h() - space_at_bottom) - spacing) / 2,
		 LINE_COLOR);

	uint32_t max = 0;
	//  Find the maximum value.
	if (m_plotmode == PLOTMODE_ABSOLUTE)  {
		for (uint32_t i = 0; i < m_plotdata.size(); ++i)
			if (m_plotdata[i].showplot) {
				for (uint32_t l = 0; l < m_plotdata[i].dataset->size(); ++l)
					if (max < (*m_plotdata[i].dataset)[l])
						max = (*m_plotdata[i].dataset)[l];
			}
	} else {
		for (uint32_t plot = 0; plot < m_plotdata.size(); ++plot)
			if (m_plotdata[plot].showplot) {

				std::vector<uint32_t> const & dataset = *m_plotdata[plot].dataset;

				// How many do we take together
				int32_t const how_many =
					static_cast<int32_t>
					((static_cast<float>(time_in_ms[m_time])
					  /
					  static_cast<float>(NR_SAMPLES))
					 /
					 static_cast<float>(m_sample_rate));

				uint32_t add = 0;
				//  Relative data, first entry is always zero.
				for (uint32_t i = 0; i < dataset.size(); ++i) {
					add += dataset[i];
					if (0 == ((i + 1) % how_many)) {
						if (max < add)
							max = add;
						add = 0;
					}
				}
			}
	}

	//  print the maximal value
	sprintf(buffer, "%u", max);
	UI::g_fh->draw_string
		(dst,
		 UI_FONT_SMALL,
		 RGBColor(120, 255, 0), RGBColor(255, 255, 255),
		 Point(get_inner_w() - space_at_right - 2, spacing),
		 buffer,
		 UI::Align_CenterRight,
		 std::numeric_limits<uint32_t>::max(),
		 UI::Widget_Cache_None,
		 g_gr->get_no_picture(),
		 std::numeric_limits<uint32_t>::max(),
		 false);

	//  plot the pixels
	sub =
		xline_length
		/
		(static_cast<float>(time_in_ms[m_time])
		 /
		 static_cast<float>(m_sample_rate));
	for (uint32_t plot = 0; plot < m_plotdata.size(); ++plot)
		if (m_plotdata[plot].showplot) {

			RGBColor color = m_plotdata[plot].plotcolor;
			std::vector<uint32_t> const * dataset = m_plotdata[plot].dataset;

			std::vector<uint32_t> m_data;
			if (m_plotmode == PLOTMODE_RELATIVE) {
				//  How many do we take together.
				const int32_t how_many = static_cast<int32_t>
				((static_cast<float>(time_in_ms[m_time])
				  /
				  static_cast<float>(NR_SAMPLES))
				 /
				 static_cast<float>(m_sample_rate));

				uint32_t add = 0;
				// Relative data, first entry is always zero
				m_data.push_back(0);
				for (uint32_t i = 0; i < dataset->size(); ++i) {
					add += (*dataset)[i];
					if (0 == ((i + 1) % how_many)) {
						m_data.push_back(add);
						add = 0;
					}
				}

				dataset = &m_data;
				sub = xline_length / static_cast<float>(NR_SAMPLES);
			}

			posx = get_inner_w() - space_at_right;

			int32_t lx = get_inner_w() - space_at_right;
			int32_t ly = get_inner_h() - space_at_bottom;
			for (int32_t i = dataset->size() - 1; i > 0 and posx > spacing; --i) {
				int32_t const curx = static_cast<int32_t>(posx);
				int32_t       cury = get_inner_h() - space_at_bottom;
				if (int32_t value = (*dataset)[i]) {
					const float length_y =
						yline_length
						/
						(static_cast<float>(max) / static_cast<float>(value));
					cury -= static_cast<int32_t>(length_y);
				}
				dst.draw_line(lx, ly, curx, cury, color);

				posx -= sub;

				lx = curx;
				ly = cury;
			}
		}
}

/*
 * Register a new plot data stream
 */
void WUIPlot_Area::register_plot_data
	(uint32_t const id,
	 std::vector<uint32_t> const * const data,
	 RGBColor const color)
{
	if (id >= m_plotdata.size())
		m_plotdata.resize(id + 1);

	m_plotdata[id].dataset   = data;
	m_plotdata[id].showplot  = false;
	m_plotdata[id].plotcolor = color;
}

/*
 * Show this plot data?
 */
void WUIPlot_Area::show_plot(uint32_t const id, bool const t) {
	assert(id < m_plotdata.size());
	m_plotdata[id].showplot = t;
};

/*
 * set time
 */
void WUIPlot_Area::set_time(TIME const id) {m_time = id;}

/*
 * Set sample rate the data uses
 */
void WUIPlot_Area::set_sample_rate(uint32_t const id) {
	m_sample_rate = id;
}
