/*
 * Copyright (C) 2002-2016 by the Widelands Development Team
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

#include "wui/general_statistics_menu.h"

#include <memory>

#include <boost/format.hpp>

#include "base/i18n.h"
#include "base/log.h"
#include "graphic/graphic.h"
#include "graphic/rendertarget.h"
#include "logic/constants.h"
#include "logic/editor_game_base.h"
#include "logic/game.h"
#include "logic/map_objects/tribes/tribe_descr.h"
#include "logic/map_objects/tribes/warelist.h"
#include "logic/player.h"
#include "scripting/lua_interface.h"
#include "scripting/lua_table.h"
#include "ui_basic/button.h"
#include "ui_basic/checkbox.h"
#include "ui_basic/slider.h"
#include "ui_basic/textarea.h"
#include "wui/interactive_player.h"

namespace {
static char const * const flag_pictures[] = {
	"images/players/genstats_enable_plr_01.png",
	"images/players/genstats_enable_plr_02.png",
	"images/players/genstats_enable_plr_03.png",
	"images/players/genstats_enable_plr_04.png",
	"images/players/genstats_enable_plr_05.png",
	"images/players/genstats_enable_plr_06.png",
	"images/players/genstats_enable_plr_07.png",
	"images/players/genstats_enable_plr_08.png"
};
} // namespace

using namespace Widelands;

#define PLOT_HEIGHT 130
#define NR_BASE_DATASETS 11

GeneralStatisticsMenu::GeneralStatisticsMenu
	(InteractiveGameBase & parent, GeneralStatisticsMenu::Registry & registry)
:
UI::UniqueWindow
	(&parent, "statistics_menu", &registry,
	 440, 400, _("General Statistics")),
         my_registry_   (&registry),
         box_           (this, 0, 0, UI::Box::Vertical, 0, 0, 5),
         plot_          (&box_, 0, 0, 430, PLOT_HEIGHT),
         selected_information_(0)
{
	assert (my_registry_);

	selected_information_ = my_registry_->selected_information;

	set_center_panel(&box_);
	box_.set_border(5, 5, 5, 5);

	// Setup plot data
	plot_.set_sample_rate(STATISTICS_SAMPLE_TIME);
	plot_.set_plotmode(WuiPlotArea::PLOTMODE_ABSOLUTE);
	Game & game = *parent.get_game();
	const Game::GeneralStatsVector & genstats =
		game.get_general_statistics();
	const Game::GeneralStatsVector::size_type
		general_statistics_size = genstats.size();

	// Is there a hook dataset?
	ndatasets_ = NR_BASE_DATASETS;
	std::unique_ptr<LuaTable> hook = game.lua().get_hook("custom_statistic");
	std::string cs_name, cs_pic;
	if (hook) {
		hook->do_not_warn_about_unaccessed_keys();
		cs_name = hook->get_string("name");
		cs_pic = hook->get_string("pic");
		ndatasets_++;
	}

	for
		(Game::GeneralStatsVector::size_type i = 0;
		 i < general_statistics_size;
		 ++i)
	{
		const RGBColor & color = Player::Colors[i];
		plot_.register_plot_data
			(i * ndatasets_ +  0, &genstats[i].land_size,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ +  1, &genstats[i].nr_workers,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ +  2, &genstats[i].nr_buildings,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ +  3, &genstats[i].nr_wares,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ +  4, &genstats[i].productivity,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ +  5, &genstats[i].nr_casualties,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ +  6, &genstats[i].nr_kills,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ +  7, &genstats[i].nr_msites_lost,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ +  8, &genstats[i].nr_msites_defeated,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ +  9, &genstats[i].nr_civil_blds_lost,
			 color);
		plot_.register_plot_data
			(i * ndatasets_ + 10, &genstats[i].miltary_strength,
			 color);
		if (hook) {
			plot_.register_plot_data
				(i * ndatasets_ + 11, &genstats[i].custom_statistic,
				 color);
		}
		if (game.get_player(i + 1)) // Show area plot
			plot_.show_plot
				(i * ndatasets_ + selected_information_,
				 my_registry_->selected_players[i]);
	}

	plot_.set_time(my_registry_->time);

	// Setup Widgets
	box_.add(&plot_, UI::Align::kTop);

	UI::Box * hbox1 = new UI::Box(&box_, 0, 0, UI::Box::Horizontal, 0, 0, 1);

	uint32_t plr_in_game = 0;
	PlayerNumber const nr_players = game.map().get_nrplayers();
	iterate_players_existing_novar(p, nr_players, game) ++plr_in_game;

	iterate_players_existing_const(p, nr_players, game, player) {
		const Image* player_image = g_gr->images().get(flag_pictures[p - 1]);
		assert(player_image);
		UI::Button & cb =
			*new UI::Button
				(hbox1, "playerbutton",
				 0, 0, 25, 25,
				 g_gr->images().get("images/ui_basic/but4.png"),
				 player_image,
				 player->get_name().c_str());
		cb.sigclicked.connect
			(boost::bind(&GeneralStatisticsMenu::cb_changed_to, this, p));
		cb.set_perm_pressed(my_registry_->selected_players[p - 1]);

		cbs_[p - 1] = &cb;

		hbox1->add(&cb, UI::Align::kLeft, false, true);
	} else //  player nr p does not exist
		cbs_[p - 1] = nullptr;

	box_.add(hbox1, UI::Align::kTop, true);

	UI::Box * hbox2 = new UI::Box(&box_, 0, 0, UI::Box::Horizontal, 0, 0, 1);

	UI::Radiobutton * btn;

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_landsize.png"),
		 _("Land"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_nrworkers.png"),
		 _("Workers"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_nrbuildings.png"),
		 _("Buildings"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_nrwares.png"),
		 _("Wares"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_productivity.png"),
		 _("Productivity"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_casualties.png"),
		 _("Casualties"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_kills.png"),
		 _("Kills"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_msites_lost.png"),
		 _("Military buildings lost"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_msites_defeated.png"),
		 _("Military buildings defeated"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_civil_blds_lost.png"),
		 _("Civilian buildings lost"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	radiogroup_.add_button
		(hbox2,
		 Point(0, 0),
		 g_gr->images().get("images/wui/stats/genstats_militarystrength.png"),
		 _("Military"),
		 &btn);
	hbox2->add(btn, UI::Align::kLeft, false, true);

	if (hook) {
		radiogroup_.add_button
			(hbox2,
			 Point(0, 0),
			 g_gr->images().get(cs_pic),
			 cs_name.c_str(),
			 &btn);
		hbox2->add(btn, UI::Align::kLeft, false, true);
	}

	radiogroup_.set_state(selected_information_);
	radiogroup_.changedto.connect
		(boost::bind(&GeneralStatisticsMenu::radiogroup_changed, this, _1));

	box_.add(hbox2, UI::Align::kTop, true);

	box_.add
		(new WuiPlotAreaSlider
			(&box_, plot_, 0, 0, 100, 45,
			 g_gr->images().get("images/ui_basic/but1.png"))
		, UI::Align::kTop
		, true);

}

GeneralStatisticsMenu::~GeneralStatisticsMenu() {
	Game & game = dynamic_cast<InteractiveGameBase&>(*get_parent()).game();
	if (game.is_loaded()) {
		// Save informations for recreation, if window is reopened
		my_registry_->selected_information = selected_information_;
		my_registry_->time = plot_.get_time();
		PlayerNumber const nr_players = game.map().get_nrplayers();
		iterate_players_existing_novar(p, nr_players, game) {
			my_registry_->selected_players[p - 1] = cbs_[p - 1]->get_perm_pressed();
		}
	}
}

/**
 * called when the help button was clicked
 */
// TODO(unknown): Implement help
void GeneralStatisticsMenu::clicked_help() {}


/*
 * Cb has been changed to this state
 */
void GeneralStatisticsMenu::cb_changed_to(int32_t const id)
{
	// This represents our player number
	cbs_[id - 1]->set_perm_pressed(!cbs_[id - 1]->get_perm_pressed());

	plot_.show_plot
		((id - 1) * ndatasets_ + selected_information_,
		 cbs_[id - 1]->get_perm_pressed());
}

/*
 * The radiogroup has changed
 */
void GeneralStatisticsMenu::radiogroup_changed(int32_t const id) {
	size_t const statistics_size =
		dynamic_cast<InteractiveGameBase&>(*get_parent()).game()
		.get_general_statistics().size();
	for (uint32_t i = 0; i < statistics_size; ++i)
		if (cbs_[i]) {
			plot_.show_plot
				(i * ndatasets_ + id, cbs_[i]->get_perm_pressed());
			plot_.show_plot
				(i * ndatasets_ + selected_information_, false);
		}
	selected_information_ = id;
}
