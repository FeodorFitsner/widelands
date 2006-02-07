/*
 * Copyright (C) 2002 by the Widelands Development Team
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

#include "constants.h"
#include "game.h"
#include "player.h"
#include "playerdescrgroup.h"
#include "types.h"
#include "ui_button.h"
#include "ui_checkbox.h"
#include "tribe.h"
#include "wexception.h"

static const char* default_names[MAX_PLAYERS+1] = {
   "", 
   _("Player 1"),
   _("Player 2"), 
   _("Player 3"), 
   _("Player 4"), 
   _("Player 5"), 
   _("Player 6"), 
   _("Player 7"), 
   _("Player 8"), 
   };

void PlayerDescriptionGroup::allow_changes(changemode_t t) {
   m_allow_changes=t;
   m_btnEnablePlayer->set_visible(t & CHANGE_ENABLED);
   m_btnPlayerType->set_enabled(false);
   m_btnPlayerTribe->set_enabled(t & CHANGE_TRIBE);
}

PlayerDescriptionGroup::PlayerDescriptionGroup(UIPanel* parent, int x, int y, Game* game, int plnum, bool highlight)
	: UIPanel(parent, x, y, 450, 20)
{
	m_game = game;
	m_plnum = plnum;
	m_allow_changes=CHANGE_EVERYTHING;
	m_current_tribe=0;
	m_playertype=Player::playerLocal; //just for initalization

	m_enabled = false;
	set_visible(false);

	// create sub-panels
	m_plr_name=new UITextarea(this, 0, 0, 100, 20, _("Player 1"), Align_Left);

	m_btnEnablePlayer = new UICheckbox(this, 88, 0);
	m_btnEnablePlayer->set_state(true);
	m_btnEnablePlayer->changedto.set(this, &PlayerDescriptionGroup::enable_player);

	m_btnPlayerType = new UIButton(this, 116, 0, 120, 20, highlight?3:1);
	m_btnPlayerType->clicked.set(this, &PlayerDescriptionGroup::toggle_playertype);

	m_btnPlayerTribe = new UIButton(this, 244, 0, 120, 20, highlight?3:1);
	m_btnPlayerTribe->clicked.set(this, &PlayerDescriptionGroup::toggle_playertribe);

	Tribe_Descr::get_all_tribes(&m_tribes);
	m_btnPlayerTribe->set_title(m_tribes[m_current_tribe].c_str());
	
	set_player_type (Player::playerAI);
}

/** PlayerDescriptionGroup::set_enabled(bool enable)
 *
 + The group is enabled if the map has got a starting position for this player.
 * We need to update the Game class accordingly.
 */
void PlayerDescriptionGroup::set_enabled(bool enable)
{
	if(!m_allow_changes) return;
	
	if (enable == m_enabled)
		return;

	m_enabled = enable;

	if (!m_enabled)
	{
		if (m_btnEnablePlayer->get_state())
			m_game->remove_player(m_plnum);
		set_visible(false);
	}
	else
	{
		if (m_btnEnablePlayer->get_state()) {
			m_game->add_player(m_plnum, m_playertype, m_tribes[m_current_tribe].c_str(), default_names[m_plnum]);
         m_game->get_player(m_plnum)->init(m_game,0); // Small initializes
      }
      
      const char* string = 0;
		switch(m_playertype) {
		case Player::playerLocal:
		case Player::playerRemote: string = _("Human"); break;
		case Player::playerAI: string = _("Computer"); break;
		}
		m_btnPlayerType->set_title(string);
		m_btnPlayerType->set_visible(m_btnEnablePlayer->get_state());
		m_btnPlayerTribe->set_visible(m_btnEnablePlayer->get_state());

		set_visible(true);
	}

	changed.call();
}

/** PlayerDescriptionGroup::enable_player(bool on)
 *
 * Update the Game when the checkbox is changed.
 */
void PlayerDescriptionGroup::enable_player(bool on)
{
//	if (!(m_allow_changes&CHANGE_ENABLED)) return;

	if (on) {
      m_game->add_player(m_plnum, m_playertype, m_tribes[m_current_tribe].c_str(), default_names[m_plnum]);
      m_game->get_player(m_plnum)->init(m_game,0); // Small initializes
   } else
		m_game->remove_player(m_plnum);

	m_btnPlayerType->set_visible(on);
	m_btnPlayerTribe->set_visible(on);
	changed.call();
}

void PlayerDescriptionGroup::toggle_playertype()
{
	// NOOP: toggling the player type is currently not possible
}

/*
 * toggles the tribe the player will play
 */
void PlayerDescriptionGroup::toggle_playertribe(void)
{
	if (!(m_allow_changes&CHANGE_TRIBE))
		return;

	++m_current_tribe;
	if(m_current_tribe==m_tribes.size()) m_current_tribe=0;
	m_btnPlayerTribe->set_title(m_tribes[m_current_tribe].c_str());

	// set the player
	m_game->remove_player(m_plnum);
	m_game->add_player(m_plnum, m_playertype, m_tribes[m_current_tribe].c_str(), default_names[m_plnum]);
   m_game->get_player(m_plnum)->init(m_game,0); // Small initializes
}

/*
 * set the current player tribe
 */
void PlayerDescriptionGroup::set_player_tribe(std::string str) {
   uint i=0;
   for(i=0; i<m_tribes.size(); i++) {
      if(m_tribes[i]==str) {
         m_current_tribe=i;
         m_btnPlayerTribe->set_title(m_tribes[m_current_tribe].c_str());
         // set the player
         m_game->remove_player(m_plnum);
         m_game->add_player(m_plnum, m_playertype, m_tribes[m_current_tribe].c_str(), default_names[m_plnum]);
         m_game->get_player(m_plnum)->init(m_game,0); // Small initializes
         return;
      }
   }
   // never here
   throw wexception("Map defines tribe %s, but it doens't exist!\n", str.c_str());
}

/*
 * Set players name
 */
void PlayerDescriptionGroup::set_player_name(std::string str) {
   m_plr_name->set_text(str.c_str());
}

void PlayerDescriptionGroup::set_player_type(int type)
{
	if (m_playertype==type)
		return;

	m_playertype=type;
	
	if (m_enabled) {
		m_btnPlayerType->set_title((type!=Player::playerAI)?_("Human"):_("Computer"));
		
		m_game->remove_player (m_plnum);
		m_game->add_player (m_plnum, m_playertype, m_tribes[m_current_tribe].c_str(), default_names[m_plnum]);
      m_game->get_player(m_plnum)->init(m_game,0); // Small initializes
	}
}

