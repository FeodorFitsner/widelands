/*
 * Copyright (C) 2007-2011 by the Widelands Development Team
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

#include "wui/interactive_gamebase.h"

#include <boost/format.hpp>

#include "base/macros.h"
#include "graphic/font_handler1.h"
#include "graphic/rendertarget.h"
#include "logic/findbob.h"
#include "logic/game.h"
#include "logic/game_controller.h"
#include "logic/player.h"
#include "logic/ship.h"
#include "profile/profile.h"
#include "wui/game_summary.h"
#include "wui/text_constants.h"
#include "wui/text_layout.h"

namespace {

std::string speed_string(int const speed) {
	if (speed) {
		return (boost::format("%u.%ux") % (speed / 1000) % (speed / 100 % 10)).str();
	}
	return _("PAUSE");
}

}  // namespace

InteractiveGameBase::InteractiveGameBase
	(Widelands::Game & _game, Section & global_s,
	 PlayerType pt, bool const chatenabled, bool const multiplayer)
	:
	InteractiveBase(_game, global_s),
	m_chatProvider(nullptr),
	m_building_census_format
		(global_s.get_string("building_census_format",       "%N")),
	m_building_statistics_format
		(global_s.get_string("building_statistics_format",   "%t")),
	m_building_tooltip_format
		(global_s.get_string("building_tooltip_format",      "%r")),
	m_chatenabled(chatenabled),
	m_multiplayer(multiplayer),
	m_playertype(pt)
{}

/// \return a pointer to the running \ref Game instance.
Widelands::Game * InteractiveGameBase::get_game() const
{
	return dynamic_cast<Widelands::Game *>(&egbase());
}


Widelands::Game & InteractiveGameBase::    game() const
{
	return dynamic_cast<Widelands::Game&>(egbase());
}

void InteractiveGameBase::set_chat_provider(ChatProvider & chat)
{
	m_chatProvider = &chat;
	m_chatOverlay->set_chat_provider(chat);

	m_chatenabled = true;
}

ChatProvider * InteractiveGameBase::get_chat_provider()
{
	return m_chatProvider;
}

void InteractiveGameBase::draw_overlay(RenderTarget& dst) {
	InteractiveBase::draw_overlay(dst);

	GameController* game_controller = game().game_controller();
	// Display the gamespeed.
	if (game_controller != nullptr) {
		std::string game_speed;
		uint32_t const real = game_controller->real_speed();
		uint32_t const desired = game_controller->desired_speed();
		if (real == desired) {
			if (real != 1000) {
				game_speed = as_uifont(speed_string(real), UI_FONT_SIZE_SMALL);
			}
		} else {
			game_speed = as_uifont((boost::format
											/** TRANSLATORS: actual_speed (desired_speed) */
											(_("%1$s (%2$s)")) %
											speed_string(real) % speed_string(desired)).str(),
										  UI_FONT_SIZE_SMALL);
		}

		if (!game_speed.empty()) {
			dst.blit(Point(get_w() - 5,  5),
						UI::g_fh1->render(game_speed),
						BlendMode::UseAlpha,
						UI::Align_TopRight);
		}
	}
}


/**
 * See if we can reasonably open a ship window at the current selection position.
 * If so, do it and return true; otherwise, return false.
 */
bool InteractiveGameBase::try_show_ship_window()
{
	Widelands::Map & map(game().map());
	Widelands::Area<Widelands::FCoords> area(map.get_fcoords(get_sel_pos().node), 1);

	if (!(area.field->nodecaps() & Widelands::MOVECAPS_SWIM))
		return false;

	std::vector<Widelands::Bob *> ships;
	if (!map.find_bobs(area, &ships, Widelands::FindBobShip()))
		return false;

	for (Widelands::Bob * temp_ship : ships) {
		if (upcast(Widelands::Ship, ship, temp_ship)) {
			if (can_see(ship->get_owner()->player_number())) {
				ship->show_window(*this);
				return true;
			}
		}
	}

	return false;
}

void InteractiveGameBase::show_game_summary()
{
	if (m_game_summary.window) {
		m_game_summary.window->set_visible(true);
		m_game_summary.window->think();
		return;
	}
	new GameSummaryScreen(this, &m_game_summary);
}
