/*
 * Copyright (C) 2002-2009 by the Widelands Development Team
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

#include "ui_fsmenu/singleplayer.h"

#include "base/i18n.h"
#include "graphic/graphic.h"
#include "graphic/text_constants.h"

FullscreenMenuSinglePlayer::FullscreenMenuSinglePlayer() :
	FullscreenMenuMainMenu(),

// Title
	title
		(this,
		 get_w() / 2, m_title_y,
		 _("Single Player"), UI::Align::kHCenter),

// Buttons
	vbox(this, m_box_x, m_box_y, UI::Box::Vertical,
		  m_butw, get_h() - m_box_y, m_padding),
	new_game
		(&vbox, "new_game", 0, 0, m_butw, m_buth, g_gr->images().get(m_button_background),
		 _("New Game"), "", true, false),
	campaign
		(&vbox, "campaigns", 0, 0, m_butw, m_buth, g_gr->images().get(m_button_background),
		 _("Campaigns"), "", true, false),
	load_game
		(&vbox, "load_game", 0, 0, m_butw, m_buth, g_gr->images().get(m_button_background),
		 _("Load Game"), "", true, false),
	back
		(&vbox, "back", 0, 0, m_butw, m_buth, g_gr->images().get(m_button_background),
		 _("Back"), "", true, false)
{
	new_game.sigclicked.connect
		(boost::bind
			(&FullscreenMenuSinglePlayer::end_modal<FullscreenMenuBase::MenuTarget>,
			 boost::ref(*this),
			 FullscreenMenuBase::MenuTarget::kNewGame));
	campaign.sigclicked.connect
		(boost::bind
			(&FullscreenMenuSinglePlayer::end_modal<FullscreenMenuBase::MenuTarget>,
			 boost::ref(*this),
			 FullscreenMenuBase::MenuTarget::kCampaign));
	load_game.sigclicked.connect
		(boost::bind
			(&FullscreenMenuSinglePlayer::end_modal<FullscreenMenuBase::MenuTarget>,
			 boost::ref(*this),
			 FullscreenMenuBase::MenuTarget::kLoadGame));
	back.sigclicked.connect
		(boost::bind
			(&FullscreenMenuSinglePlayer::end_modal<FullscreenMenuBase::MenuTarget>,
			 boost::ref(*this),
			 FullscreenMenuBase::MenuTarget::kBack));

	title.set_font(ui_fn(), fs_big(), UI_FONT_CLR_FG);

	vbox.add(&new_game, UI::Align::kHCenter);
	vbox.add(&campaign, UI::Align::kHCenter);

	vbox.add_space(m_buth);

	vbox.add(&load_game, UI::Align::kHCenter);

	vbox.add_space(6 * m_buth);

	vbox.add(&back, UI::Align::kHCenter);

	vbox.set_size(m_butw, get_h() - vbox.get_y());
}

void FullscreenMenuSinglePlayer::clicked_ok() {
	end_modal<FullscreenMenuBase::MenuTarget>(FullscreenMenuBase::MenuTarget::kNewGame);
}
