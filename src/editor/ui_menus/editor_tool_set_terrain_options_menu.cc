/*
 * Copyright (C) 2002-2004, 2006-2011 by the Widelands Development Team
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

#include "editor/ui_menus/editor_tool_set_terrain_options_menu.h"

#include <memory>

#include <SDL_keycode.h>
#include <boost/format.hpp>

#include "base/i18n.h"
#include "editor/editorinteractive.h"
#include "editor/tools/editor_set_terrain_tool.h"
#include "graphic/graphic.h"
#include "graphic/in_memory_image.h"
#include "graphic/rendertarget.h"
#include "graphic/terrain_texture.h"
#include "graphic/texture.h"
#include "logic/map.h"
#include "logic/world/editor_category.h"
#include "logic/world/terrain_description.h"
#include "logic/world/world.h"
#include "ui_basic/checkbox.h"
#include "ui_basic/panel.h"
#include "ui_basic/tabpanel.h"

namespace {

using namespace Widelands;

static const int32_t check[] = {
	TerrainDescription::GREEN,                                 //  "green"
	TerrainDescription::DRY,                                   //  "dry"
	TerrainDescription::DRY | TerrainDescription::MOUNTAIN,    //  "mountain"
	TerrainDescription::DRY | TerrainDescription::UNPASSABLE,  //  "unpassable"
	TerrainDescription::ACID | TerrainDescription::DRY |
		TerrainDescription::UNPASSABLE,  //  "dead" or "acid"
	TerrainDescription::UNPASSABLE | TerrainDescription::DRY | TerrainDescription::WATER,
	-1,  // end marker
};

UI::Checkbox* create_terrain_checkbox(UI::Panel* parent,
                                      const TerrainDescription& terrain_descr,
                                      std::vector<std::unique_ptr<const Image>>* offscreen_images) {
	const Image* green = g_gr->images().get("pics/terrain_green.png");
	const Image* water = g_gr->images().get("pics/terrain_water.png");
	const Image* mountain = g_gr->images().get("pics/terrain_mountain.png");
	const Image* dead = g_gr->images().get("pics/terrain_dead.png");
	const Image* unpassable = g_gr->images().get("pics/terrain_unpassable.png");
	const Image* dry = g_gr->images().get("pics/terrain_dry.png");

	constexpr int kSmallPicHeight = 20;
	constexpr int kSmallPicWidth = 20;

	std::vector<std::string> tooltips;

	for (size_t checkfor = 0; check[checkfor] >= 0; ++checkfor) {
		const TerrainDescription::Type ter_is = terrain_descr.get_is();
		if (ter_is != check[checkfor])
			continue;

		const Texture& terrain_texture = terrain_descr.get_texture(0);
		Texture* texture = new Texture(terrain_texture.width(), terrain_texture.height());
		texture->blit(Point(0, 0),
		              &terrain_texture,
		              Rect(0, 0, terrain_texture.width(), terrain_texture.height()));
		Point pt(1, terrain_texture.height() - kSmallPicHeight - 1);

		if (ter_is == TerrainDescription::GREEN) {
			texture->blit(pt, green->texture(), Rect(0, 0, green->width(), green->height()));
			pt.x += kSmallPicWidth + 1;
			/** TRANSLATORS: This is a terrain type tooltip in the editor */
			tooltips.push_back(_("arable"));
		} else {
			if (ter_is & TerrainDescription::WATER) {
				texture->blit(pt, water->texture(), Rect(0, 0, water->width(), water->height()));
				pt.x += kSmallPicWidth + 1;
				/** TRANSLATORS: This is a terrain type tooltip in the editor */
				tooltips.push_back(_("aquatic"));
			}
			else if (ter_is & TerrainDescription::MOUNTAIN) {
				texture->blit(pt, mountain->texture(), Rect(0, 0, mountain->width(), mountain->height()));
				pt.x += kSmallPicWidth + 1;
				/** TRANSLATORS: This is a terrain type tooltip in the editor */
				tooltips.push_back(_("mountainous"));
			}
			if (ter_is & TerrainDescription::ACID) {
				texture->blit(pt, dead->texture(), Rect(0, 0, dead->width(), dead->height()));
				pt.x += kSmallPicWidth + 1;
				/** TRANSLATORS: This is a terrain type tooltip in the editor */
				tooltips.push_back(_("dead"));
			}
			if (ter_is & TerrainDescription::UNPASSABLE) {
				texture->blit(
				   pt, unpassable->texture(), Rect(0, 0, unpassable->width(), unpassable->height()));
				pt.x += kSmallPicWidth + 1;
				/** TRANSLATORS: This is a terrain type tooltip in the editor */
				tooltips.push_back(_("unpassable"));
			}
			if (ter_is & TerrainDescription::DRY) {
				texture->blit(pt, dry->texture(), Rect(0, 0, dry->width(), dry->height()));
				/** TRANSLATORS: This is a terrain type tooltip in the editor */
				 tooltips.push_back(_("treeless"));
			}
		}

		// Make sure we delete this later on.
		offscreen_images->emplace_back(new_in_memory_image("dummy_hash", texture));
		break;
	}
	/** TRANSLATORS: %1% = terrain name, %2% = list of terrain types  */
	const std::string tooltip = ((boost::format("%1%: %2%"))
								  % terrain_descr.descname()
								  % i18n::localize_item_list(tooltips, i18n::ConcatenateWith::AND)).str();

	std::unique_ptr<const Image>& image = offscreen_images->back();
	UI::Checkbox* cb = new UI::Checkbox(parent, Point(0, 0), image.get(), tooltip);
	cb->set_desired_size(image->width() + 1, image->height() + 1);
	return cb;
}

}  // namespace

EditorToolSetTerrainOptionsMenu::EditorToolSetTerrainOptionsMenu(
   EditorInteractive& parent, EditorSetTerrainTool& tool, UI::UniqueWindow::Registry& registry)
   : EditorToolOptionsMenu(parent, registry, 0, 0, _("Terrain Select")) {
	const Widelands::World& world = parent.egbase().world();
	multi_select_menu_.reset(
	   new CategorizedItemSelectionMenu<Widelands::TerrainDescription, EditorSetTerrainTool>(
	      this,
	      world.editor_terrain_categories(),
	      world.terrains(),
	      [this](UI::Panel* cb_parent, const TerrainDescription& terrain_descr) {
		      return create_terrain_checkbox(cb_parent, terrain_descr, &offscreen_images_);
		   },
	      [this] {select_correct_tool();},
	      &tool));
	set_center_panel(multi_select_menu_.get());
}

EditorToolSetTerrainOptionsMenu::~EditorToolSetTerrainOptionsMenu() {
}
