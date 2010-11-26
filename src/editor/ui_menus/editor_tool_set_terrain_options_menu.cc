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

#include "editor_tool_set_terrain_options_menu.h"

#include "editor/tools/editor_set_terrain_tool.h"
#include "editor/editorinteractive.h"
#include "i18n.h"
#include "logic/map.h"
#include <SDL_keysym.h>
#include "wlapplication.h"
#include "logic/world.h"
#include "logic/worlddata.h"

#include "ui_basic/button.h"
#include "ui_basic/panel.h"
#include "ui_basic/checkbox.h"

#include "graphic/graphic.h"
#include "graphic/texture.h"
#include "graphic/rendertarget.h"

#include "log.h"

Editor_Tool_Set_Terrain_Options_Menu:: Editor_Tool_Set_Terrain_Options_Menu
	(Editor_Interactive         & parent,
	 Editor_Set_Terrain_Tool    & tool,
	 UI::UniqueWindow::Registry & registry)
	:
	Editor_Tool_Options_Menu(parent, registry, 0, 0, _("Terrain Select")),
	m_cur_selection         (this, 0, 0, 0, 20, UI::Align_Center),
	m_tool                  (tool)
{
	Widelands::World & world = parent.egbase().map().world();
	Widelands::Terrain_Index const nr_terrains = world.get_nr_terrains();
	const uint32_t terrains_in_row = static_cast<uint32_t>
		(ceil(sqrt(static_cast<float>(nr_terrains))));


	int32_t check[] = {
		0,                                            //  "green"
		TERRAIN_DRY,                                  //  "dry"
		TERRAIN_DRY|TERRAIN_MOUNTAIN,                 //  "mountain"
		TERRAIN_DRY|TERRAIN_UNPASSABLE,               //  "unpassable"
		TERRAIN_ACID|TERRAIN_DRY|TERRAIN_UNPASSABLE,  //  "dead" or "acid"
		TERRAIN_UNPASSABLE|TERRAIN_DRY|TERRAIN_WATER,
	};

	m_checkboxes.resize(nr_terrains);

	const PictureID green =
		g_gr->get_picture(PicMod_Game, "pics/terrain_green.png");
	const PictureID water =
		g_gr->get_picture(PicMod_Game, "pics/terrain_water.png");
	const PictureID mountain =
		g_gr->get_picture(PicMod_Game, "pics/terrain_mountain.png");
	const PictureID dead =
		g_gr->get_picture(PicMod_Game, "pics/terrain_dead.png");
	const PictureID unpassable =
		g_gr->get_picture(PicMod_Game, "pics/terrain_unpassable.png");
	const PictureID dry =
		g_gr->get_picture(PicMod_Game, "pics/terrain_dry.png");
#define small_pich 20
#define small_picw 20

	uint32_t cur_x = 0;
	Point pos(hmargin(), vmargin());
	for (size_t checkfor = 0; checkfor < 6; ++checkfor)
		for (Widelands::Terrain_Index i  = 0; i < nr_terrains; ++i) {

			const uint8_t ter_is = world.get_ter(i).get_is();
			if (ter_is != check[checkfor])
				continue;

			if (cur_x == terrains_in_row) {
				cur_x = 0;
				pos.x  = hmargin();
				pos.y += TEXTURE_HEIGHT + vspacing();
			}

			PictureID surface;

			// If offscreen rendering is not available only the terrain (and not
			// the terrain type) is shown.
			// TODO: Find a way to render this without offscreen rendering
			//       or implement offscreen rendering for opengl
			if (g_gr->caps().offscreen_rendering)
			{
				//  create a surface for this
				surface = g_gr->create_picture_surface(64, 64);

				//  get the rendertarget for this
				RenderTarget target(surface->impl().surface);

				//  first, blit the terrain texture
				target.blit
					(Point(0, 0),
					 g_gr->get_picture
					 	(PicMod_Game,
					 	 g_gr->get_maptexture_data
					 	 	(world.terrain_descr(i).get_texture())
					 	 ->get_texture_picture()));

				Point pic(1, 64 - small_pich - 1);

				//  check is green
				if (ter_is == 0) {
					target.blit(pic, green);
					pic.x += small_picw + 1;
				} else {
					if (ter_is & TERRAIN_WATER) {
						target.blit(pic, water);
						pic.x += small_picw + 1;
					}
					if (ter_is & TERRAIN_MOUNTAIN) {
						target.blit(pic, mountain);
						pic.x += small_picw + 1;
					}
					if (ter_is & TERRAIN_ACID) {
						target.blit(pic, dead);
						pic.x += small_picw + 1;
					}
					if (ter_is & TERRAIN_UNPASSABLE) {
						target.blit(pic, unpassable);
						pic.x += small_picw + 1;
					}
					if (ter_is & TERRAIN_DRY)
						target.blit(pic, dry);
				}
			} else {
				surface = g_gr->get_picture
					(PicMod_Game,
					 g_gr->get_maptexture_data(world.terrain_descr(i).get_texture())
					 ->get_texture_picture());
			}

			//  Save this surface, so we can free it later on.
			m_surfaces.push_back(surface);

			UI::Checkbox & cb = *new UI::Checkbox(this, pos, surface);
			cb.set_size(TEXTURE_WIDTH + 1, TEXTURE_HEIGHT + 1);
			cb.set_id(i);
			cb.set_state(m_tool.is_enabled(i));
			cb.changedtoid.set
				(this, &Editor_Tool_Set_Terrain_Options_Menu::selected);
			m_checkboxes[i] = &cb;

			pos.x += TEXTURE_WIDTH + hspacing();
			++cur_x;
		}
	pos.y += TEXTURE_HEIGHT + vspacing();

	set_inner_size
		(terrains_in_row * (TEXTURE_WIDTH + hspacing()) +
		 2 * hmargin() - hspacing(),
		 pos.y + m_cur_selection.get_h() + vmargin());
	pos.x = get_inner_w() / 2;
	m_cur_selection.set_pos(pos);

	std::string buf = _("Current:");
	uint32_t j = m_tool.get_nr_enabled();
	for (Widelands::Terrain_Index i = 0; j; ++i)
		if (m_tool.is_enabled(i)) {
			buf += " ";
			buf += world.get_ter(i).name();
			--j;
		}
	m_cur_selection.set_text(buf);
}


Editor_Tool_Set_Terrain_Options_Menu::~Editor_Tool_Set_Terrain_Options_Menu()
{
}


void Editor_Tool_Set_Terrain_Options_Menu::selected
	(int32_t const n, bool const t)
{
	//  FIXME This code is erroneous. It checks the current key state. What it
	//  FIXME needs is the key state at the time the mouse was clicked. See the
	//  FIXME usage comment for get_key_state.
	const bool multiselect =
		get_key_state(SDLK_LCTRL) | get_key_state(SDLK_RCTRL);
	if (not t and (not multiselect or m_tool.get_nr_enabled() == 1))
		m_checkboxes[n]->set_state(true);
	else {
		if (not multiselect) {
			for (uint32_t i = 0; m_tool.get_nr_enabled(); ++i)
				m_tool.enable(i, false);
			//  disable all checkboxes
			const uint32_t size = m_checkboxes.size();
			//TODO: the uint32_t cast is ugly!
			for
				(uint32_t i = 0; i < size;
				 ++i, i += i == static_cast<uint32_t>(n))
			{
				m_checkboxes[i]->changedtoid.set
					(this,
					 static_cast
					 	<void (Editor_Tool_Set_Terrain_Options_Menu::*)
					 	(int32_t, bool)>
					 	(0));
				m_checkboxes[i]->set_state(false);
				m_checkboxes[i]->changedtoid.set
					(this, &Editor_Tool_Set_Terrain_Options_Menu::selected);
			}
		}

		m_tool.enable(n, t);
		select_correct_tool();

		std::string buf = _("Current:");
		Widelands::World const & world =
			ref_cast<Editor_Interactive, UI::Panel>(*get_parent())
			.egbase().map().world();
		uint32_t j = m_tool.get_nr_enabled();
		for (Widelands::Terrain_Index i = 0; j; ++i)
			if (m_tool.is_enabled(i)) {
				buf += " ";
				buf += world.get_ter(i).name();
				--j;
			}

		m_cur_selection.set_text(buf.c_str());
		m_cur_selection.set_pos
			(Point
			 	((get_inner_w() - m_cur_selection.get_w()) / 2,
			 	 m_cur_selection.get_y()));
	}
}
