/*
 * Copyright (C) 2002-2004, 2006-2009 by the Widelands Development Team
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

#include "editor_tool_noise_height_options_menu.h"

#include "editorinteractive.h"
#include "editor_increase_height_tool.h"
#include "editor_decrease_height_tool.h"
#include "editor_noise_height_tool.h"
#include "graphic.h"
#include "i18n.h"

#include <cstdio>

using Widelands::Field;

#define width  20
#define height 20
Editor_Tool_Noise_Height_Options_Menu::Editor_Tool_Noise_Height_Options_Menu
	(Editor_Interactive         & parent,
	 Editor_Noise_Height_Tool   & noise_tool,
	 UI::UniqueWindow::Registry & registry)
:
Editor_Tool_Options_Menu
	(parent, registry, 200, 115, _("Noise Height Options")),
m_noise_tool(noise_tool),
m_lower_label
	(this,
	 hmargin(),
	 vmargin(), (get_inner_w() - 2 * hmargin() - spacing()) / 2, height,
	 Align_BottomCenter),
m_upper_label
	(this,
	 m_lower_label.get_x() + m_lower_label.get_w() + spacing(),
	 m_lower_label.get_y(),
	 m_lower_label.get_w(), height,
	 Align_BottomCenter),
m_lower_increase
	(this,
	 hmargin() + (get_inner_w() - 2 * hmargin() - hspacing() - 4 * width) / 4,
	 m_lower_label.get_y() + m_lower_label.get_h() + vspacing(),
	 width, height,
	 0,
	 g_gr->get_picture(PicMod_UI, "pics/scrollbar_up.png"),
	 &Editor_Tool_Noise_Height_Options_Menu::clicked_lower_increase, this,
	 std::string(),
	 noise_tool.get_interval().min < MAX_FIELD_HEIGHT),
m_lower_decrease
	(this,
	 m_lower_increase.get_x() + m_lower_increase.get_w(),
	 m_lower_increase.get_y(),
	 width, height,
	 0,
	 g_gr->get_picture(PicMod_UI, "pics/scrollbar_down.png"),
	 &Editor_Tool_Noise_Height_Options_Menu::clicked_lower_decrease, this,
	 std::string(),
	 0 < noise_tool.get_interval().min),
m_upper_increase
	(this,
	 m_lower_decrease.get_x() + width
	 +
	 (get_inner_w() - 2 * hmargin() - hspacing() - 4 * width) / 2 + hspacing(),
	 m_lower_decrease.get_y(),
	 width, height,
	 0,
	 g_gr->get_picture(PicMod_UI, "pics/scrollbar_up.png"),
	 &Editor_Tool_Noise_Height_Options_Menu::clicked_upper_increase, this,
	 std::string(),
	 noise_tool.get_interval().max < MAX_FIELD_HEIGHT),
m_upper_decrease
	(this,
	 m_upper_increase.get_x() + m_upper_increase.get_w(),
	 m_upper_increase.get_y(),
	 width, height,
	 0,
	 g_gr->get_picture(PicMod_UI, "pics/scrollbar_down.png"),
	 &Editor_Tool_Noise_Height_Options_Menu::clicked_upper_decrease, this,
	 std::string(),
	 0 < noise_tool.get_interval().max),
m_set_label
	(this,
	 hspacing(),
	 m_upper_decrease.get_y() + m_upper_decrease.get_h() + vspacing(),
	 get_inner_w() - 2 * hspacing(), height,
	 Align_BottomCenter),
m_setto_increase
	(this,
	 get_inner_w() / 2 - width,
	 m_set_label.get_y() + m_set_label.get_h() + vspacing(),
	 width, height,
	 1,
	 g_gr->get_picture(PicMod_Game, "pics/scrollbar_up.png"),
	 &Editor_Tool_Noise_Height_Options_Menu::clicked_setto_increase, this,
	 std::string(),
	 noise_tool.set_tool().get_interval().max < MAX_FIELD_HEIGHT),
m_setto_decrease
	(this,
	 get_inner_w() / 2, m_setto_increase.get_y(), width, height,
	 1,
	 g_gr->get_picture(PicMod_Game, "pics/scrollbar_down.png"),
	 &Editor_Tool_Noise_Height_Options_Menu::clicked_setto_decrease, this,
	 std::string(),
	 0 < noise_tool.set_tool().get_interval().min)
{
	update();
}

/**
 * Update all textareas
*/
void Editor_Tool_Noise_Height_Options_Menu::update() {
	char buffer[200];
	const interval<Field::Height> height_interval = m_noise_tool.get_interval();
	snprintf(buffer, sizeof(buffer), _("Minimum: %u"), height_interval.min);
	m_lower_label.set_text(buffer);
	snprintf(buffer, sizeof(buffer), _("Maximum: %u"), height_interval.max);
	m_upper_label.set_text(buffer);

	snprintf
		(buffer, sizeof(buffer),
		 _("Set value: %u"), m_noise_tool.set_tool().get_interval().min);
	m_set_label.set_text(buffer);

	select_correct_tool();
}


void Editor_Tool_Noise_Height_Options_Menu::clicked_lower_decrease() {
	interval<Field::Height> height_interval = m_noise_tool.get_interval();

	assert(height_interval.valid());
	assert(0 < height_interval.min);

	--height_interval.min;

	assert(height_interval.valid());

	m_noise_tool.set_interval(height_interval);
	m_lower_decrease.set_enabled(0 < height_interval.min);
	m_lower_increase.set_enabled(true);
	update();
}


void Editor_Tool_Noise_Height_Options_Menu::clicked_lower_increase() {
	interval<Field::Height> height_interval = m_noise_tool.get_interval();

	assert(height_interval.valid());
	assert(height_interval.min < MAX_FIELD_HEIGHT);

	++height_interval.min;
	height_interval.max = std::max(height_interval.min, height_interval.max);

	assert(height_interval.valid());

	m_noise_tool.set_interval(height_interval);
	m_lower_decrease.set_enabled(true);
	m_lower_increase.set_enabled(height_interval.min < MAX_FIELD_HEIGHT);
	m_upper_decrease.set_enabled(true);
	m_upper_increase.set_enabled(height_interval.max < MAX_FIELD_HEIGHT);

	update();
}


void Editor_Tool_Noise_Height_Options_Menu::clicked_upper_decrease() {
	interval<Field::Height> height_interval = m_noise_tool.get_interval();

	assert(height_interval.valid());
	assert(0 < m_noise_tool.get_interval().max);

	--height_interval.max;
	height_interval.min = std::min(height_interval.min, height_interval.max);

	assert(height_interval.valid());

	m_noise_tool.set_interval(height_interval);
	m_lower_decrease.set_enabled(0 < height_interval.min);
	m_lower_increase.set_enabled(true);
	m_upper_decrease.set_enabled(0 < height_interval.max);
	m_upper_increase.set_enabled(true);
	update();
}


void Editor_Tool_Noise_Height_Options_Menu::clicked_upper_increase() {
	interval<Field::Height> height_interval = m_noise_tool.get_interval();

	assert(m_noise_tool.get_interval().valid());
	assert(m_noise_tool.get_interval().max < MAX_FIELD_HEIGHT);

	++height_interval.max;

	assert(m_noise_tool.get_interval().valid());

	m_noise_tool.set_interval(height_interval);
	m_upper_decrease.set_enabled(true);
	m_upper_increase.set_enabled(height_interval.max < MAX_FIELD_HEIGHT);
	update();
}


void Editor_Tool_Noise_Height_Options_Menu::clicked_setto_decrease() {
	Editor_Set_Height_Tool & set_tool = m_noise_tool.set_tool();
	Field::Height h = set_tool.get_interval().min;

	assert(h == set_tool.get_interval().max);
	assert(0 < h);

	--h;

	set_tool.set_interval(interval<Field::Height>(h, h));
	m_setto_decrease.set_enabled(0 < h);
	m_setto_increase.set_enabled(true);
	update();
}


void Editor_Tool_Noise_Height_Options_Menu::clicked_setto_increase() {
	Editor_Set_Height_Tool & set_tool = m_noise_tool.set_tool();
	Field::Height h = set_tool.get_interval().min;

	assert(h == set_tool.get_interval().max);
	assert(h < MAX_FIELD_HEIGHT);

	++h;

	set_tool.set_interval(interval<Field::Height>(h, h));
	m_setto_decrease.set_enabled(true);
	m_setto_increase.set_enabled(h < MAX_FIELD_HEIGHT);
	update();
}
