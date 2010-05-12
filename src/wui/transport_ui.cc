/*
 * Copyright (C) 2008-2010 by the Widelands Development Team
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

#include "economy/economy.h"
#include "graphic/font_handler.h"
#include "interactive_gamebase.h"
#include "logic/item_ware_descr.h"
#include "logic/player.h"
#include "logic/playercommand.h"
#include "graphic/rendertarget.h"
#include "logic/tribe.h"
#include "ui_basic/button.h"
#include "ui_basic/tabpanel.h"
#include "ui_basic/unique_window.h"

using Widelands::Economy;
using Widelands::Editor_Game_Base;
using Widelands::Game;
using Widelands::Item_Ware_Descr;
using Widelands::Ware_Index;
using Widelands::Worker_Descr;

struct Economy_Options_Window : public UI::UniqueWindow {
	Economy_Options_Window(Interactive_GameBase & parent, Economy & economy)
		:
		UI::UniqueWindow
			(&parent, &economy.m_optionswindow_registry, 0, 0,
			 _("Economy options")),
		m_tabpanel(*this, economy)
	{
		fit_inner(m_tabpanel);
	}

	virtual void think() {
		Interactive_GameBase const & igbase =
			ref_cast<Interactive_GameBase, UI::Panel>(*get_parent());
		Widelands::Player_Number const owner =
			m_tabpanel.economy().owner().player_number();
		if (not igbase.can_see(owner))
			die();
		bool const can_act = igbase.can_act(owner);
		for
			(Ware_Type_Box * b =
			 	dynamic_cast<Ware_Type_Box *>
			 		(m_tabpanel.m_ware_target_quantities.get_first_child());
			 b;
			 b = dynamic_cast<Ware_Type_Box *>(b->get_next_sibling()))
		{
			Ware_Index const i = b->ware_type;
			Economy::Target_Quantity const & tq =
				m_tabpanel.economy().ware_target_quantity(i);
			b->decrease_permanent.set_enabled(can_act and 1 < tq.permanent);
			b->increase_permanent.set_enabled(can_act);
			b->decrease_temporary.set_enabled(can_act and 1 < tq.temporary);
			b->increase_temporary.set_enabled(can_act);
			b->reset             .set_enabled(can_act and tq.last_modified);
		}
		for
			(Worker_Type_Box * b =
			 	dynamic_cast<Worker_Type_Box *>
			 		(m_tabpanel.m_worker_target_quantities.get_first_child());
			 b;
			 b = dynamic_cast<Worker_Type_Box *>(b->get_next_sibling()))
		{
			Ware_Index const i = b->worker_type;
			Economy::Target_Quantity const & tq =
				m_tabpanel.economy().worker_target_quantity(i);
			b->decrease_permanent.set_enabled(can_act and 1 < tq.permanent);
			b->increase_permanent.set_enabled(can_act);
			b->decrease_temporary.set_enabled(can_act and 1 < tq.temporary);
			b->increase_temporary.set_enabled(can_act);
			b->reset             .set_enabled(can_act and tq.last_modified);
		}
	}

private:
	struct Ware_Type_Box : public UI::Panel {
		Ware_Type_Box
			(UI::Box               &       parent,
			 Ware_Index              const _ware_type,
			 Item_Ware_Descr const &       _descr)
			:
			UI::Panel  (&parent, 0, 0, 420, 24),
			decrease_permanent(*this),
			increase_permanent(*this),
			decrease_temporary(*this),
			increase_temporary(*this),
			reset             (*this),
			ware_type         (_ware_type),
			descr             (_descr)
		{}

		virtual void draw(RenderTarget & dst) {
			dst.blit(Point(0, 0), descr.icon());
			UI::g_fh->draw_string
				(dst,
				 UI_FONT_NAME, UI_FONT_SIZE_SMALL, UI_FONT_CLR_FG, UI_FONT_CLR_BG,
				 Point(26, 12),
				 descr.descname());
			Economy::Target_Quantity const & tq =
				economy().ware_target_quantity(ware_type);
			char buffer[32];
			sprintf(buffer, "%u", tq.permanent);
			UI::g_fh->draw_string
				(dst,
				 UI_FONT_NAME, UI_FONT_SIZE_SMALL, UI_FONT_CLR_FG, UI_FONT_CLR_BG,
				 Point(188, 12),
				 buffer,
				 UI::Align_CenterRight);
			sprintf(buffer, "%u", tq.temporary);
			UI::g_fh->draw_string
				(dst,
				 UI_FONT_NAME, UI_FONT_SIZE_SMALL, UI_FONT_CLR_FG, UI_FONT_CLR_BG,
				 Point(278, 12),
				 buffer,
				 UI::Align_CenterRight);
			UI::Panel::draw(dst);
		}

		Economy & economy() const {
			return
				ref_cast<Tab_Panel, UI::Panel>
					(*ref_cast<UI::Box, UI::Panel>(*get_parent()).get_parent())
				.economy();
		}

		struct Decrease_Permanent : public UI::Button {
			Decrease_Permanent(Ware_Type_Box & parent) :
				UI::Button
					(&parent, 190, 0, 24, 24,
					 g_gr->get_no_picture(),
					 g_gr->get_picture(PicMod_UI, "pics/scrollbar_down.png"),
					 _("Decrease permanent target quantity"),
					 true)
			{
				set_repeating(true);
			}
			void clicked() {
				Ware_Type_Box const & parent =
					ref_cast<Ware_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Ware_Index const ware_type = parent.ware_type;
				Economy::Target_Quantity const & tq =
					e.ware_target_quantity(ware_type);
				assert(tq.permanent <= tq.temporary);
				if (1 < tq.permanent) {
					Widelands::Player & player = e.owner();
					Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
					game.send_player_command
						(*new Widelands::Cmd_SetWareTargetQuantity
						 	(game.get_gametime(), player.player_number(),
						 	 player.get_economy_number(&e), ware_type,
						 	 tq.permanent - 1, tq.temporary));
				}
			}
		} decrease_permanent;

		struct Increase_Permanent : public UI::Button {
			Increase_Permanent(Ware_Type_Box & parent) :
				UI::Button
					(&parent, 214, 0, 24, 24,
					 g_gr->get_no_picture(),
					 g_gr->get_picture(PicMod_UI, "pics/scrollbar_up.png"),
					 _("Increase permanent target quantity"))
			{
				set_repeating(true);
			}
			void clicked() {
				Ware_Type_Box const & parent =
					ref_cast<Ware_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Ware_Index const ware_type = parent.ware_type;
				Economy::Target_Quantity const & tq =
					e.ware_target_quantity(ware_type);
				assert(tq.permanent <= tq.temporary);
				uint32_t const new_permanent = tq.permanent + 1;
				Widelands::Player & player = e.owner();
				Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
				game.send_player_command
					(*new Widelands::Cmd_SetWareTargetQuantity
					 	(game.get_gametime(), player.player_number(),
					 	 player.get_economy_number(&e), ware_type,
					 	 new_permanent, std::max(new_permanent, tq.temporary)));
			}
		} increase_permanent;

		struct Decrease_Temporary : public UI::Button {
			Decrease_Temporary(Ware_Type_Box & parent) :
				UI::Button
					(&parent, 280, 0, 24, 24,
					 g_gr->get_no_picture(),
					 g_gr->get_picture(PicMod_UI, "pics/scrollbar_down.png"),
					 _("Decrease temporary target quantity"))
			{
				set_repeating(true);
			}
			void clicked() {
				Ware_Type_Box const & parent =
					ref_cast<Ware_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Ware_Index const ware_type = parent.ware_type;
				Economy::Target_Quantity const & tq =
					e.ware_target_quantity(ware_type);
				assert(tq.permanent <= tq.temporary);
				if (1 < tq.temporary) {
					uint32_t const new_temporary = tq.temporary - 1;
					Widelands::Player & player = e.owner();
					Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
					game.send_player_command
						(*new Widelands::Cmd_SetWareTargetQuantity
						 	(game.get_gametime(), player.player_number(),
						 	 player.get_economy_number(&e), ware_type,
						 	 std::min(tq.permanent, new_temporary), new_temporary));
				}
			}
		} decrease_temporary;

		struct Increase_Temporary : public UI::Button {
			Increase_Temporary(Ware_Type_Box & parent) :
				UI::Button
					(&parent, 304, 0, 24, 24,
					 g_gr->get_no_picture(),
					 g_gr->get_picture(PicMod_UI, "pics/scrollbar_up.png"),
					 _("Increase temporary target quantity"))
			{
				set_repeating(true);
			}
			void clicked() {
				Ware_Type_Box const & parent =
					ref_cast<Ware_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Ware_Index const ware_type = parent.ware_type;
				Economy::Target_Quantity const & tq =
					e.ware_target_quantity(ware_type);
				assert(tq.permanent <= tq.temporary);
				Widelands::Player & player = e.owner();
				Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
				game.send_player_command
					(*new Widelands::Cmd_SetWareTargetQuantity
					 	(game.get_gametime(), player.player_number(),
					 	 player.get_economy_number(&e), ware_type,
					 	 tq.permanent, tq.temporary + 1));
			}
		} increase_temporary;

		struct Reset : UI::Button {
			Reset(Ware_Type_Box & parent) :
				UI::Button
					(&parent, 330, 0, 90, 24, g_gr->get_no_picture(),
					 _("Reset"), _("Reset target quantity to default value"))
			{}
			void clicked() {
				Ware_Type_Box const & parent =
					ref_cast<Ware_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Widelands::Player & player = e.owner();
				Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
				game.send_player_command
					(*new Widelands::Cmd_ResetWareTargetQuantity
					 	(game.get_gametime(), player.player_number(),
					 	 player.get_economy_number(&e), parent.ware_type));
			}
		} reset;

		Ware_Index              const ware_type;
		Item_Ware_Descr const &       descr;
	};

	struct Worker_Type_Box : public UI::Panel {
		Worker_Type_Box
			(UI::Box            &       parent,
			 Ware_Index           const _worker_type,
			 Worker_Descr const &       _descr)
			:
			UI::Panel  (&parent, 0, 0, 420, 24),
			decrease_permanent(*this),
			increase_permanent(*this),
			decrease_temporary(*this),
			increase_temporary(*this),
			reset             (*this),
			worker_type       (_worker_type),
			descr             (_descr)
		{}

		virtual void draw(RenderTarget & dst) {
			dst.blit(Point(0, 0), descr.icon());
			UI::g_fh->draw_string
				(dst,
				 UI_FONT_NAME, UI_FONT_SIZE_SMALL, UI_FONT_CLR_FG, UI_FONT_CLR_BG,
				 Point(26, 12),
				 descr.descname());
			Economy::Target_Quantity const & tq =
				economy().worker_target_quantity(worker_type);
			char buffer[32];
			sprintf(buffer, "%u", tq.permanent);
			UI::g_fh->draw_string
				(dst,
				 UI_FONT_NAME, UI_FONT_SIZE_SMALL, UI_FONT_CLR_FG, UI_FONT_CLR_BG,
				 Point(188, 12),
				 buffer,
				 UI::Align_CenterRight);
			sprintf(buffer, "%u", tq.temporary);
			UI::g_fh->draw_string
				(dst,
				 UI_FONT_NAME, UI_FONT_SIZE_SMALL, UI_FONT_CLR_FG, UI_FONT_CLR_BG,
				 Point(278, 12),
				 buffer,
				 UI::Align_CenterRight);
			UI::Panel::draw(dst);
		}

		Economy & economy() const {
			return
				ref_cast<Tab_Panel, UI::Panel>
					(*ref_cast<UI::Box, UI::Panel>(*get_parent()).get_parent())
				.economy();
		}

		struct Decrease_Permanent : public UI::Button {
			Decrease_Permanent(Worker_Type_Box & parent) :
				UI::Button
					(&parent, 190, 0, 24, 24,
					 g_gr->get_no_picture(),
					 g_gr->get_picture(PicMod_UI, "pics/scrollbar_down.png"),
					 _("Decrease permanent target quantity"),
					 true)
			{
				set_repeating(true);
			}
			void clicked() {
				Worker_Type_Box const & parent =
					ref_cast<Worker_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Ware_Index const worker_type = parent.worker_type;
				Economy::Target_Quantity const & tq =
					e.worker_target_quantity(worker_type);
				assert(tq.permanent <= tq.temporary);
				if (1 < tq.permanent) {
					Widelands::Player & player = e.owner();
					Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
					game.send_player_command
						(*new Widelands::Cmd_SetWorkerTargetQuantity
						 	(game.get_gametime(), player.player_number(),
						 	 player.get_economy_number(&e), worker_type,
						 	 tq.permanent - 1, tq.temporary));
				}
			}
		} decrease_permanent;

		struct Increase_Permanent : public UI::Button {
			Increase_Permanent(Worker_Type_Box & parent) :
				UI::Button
					(&parent, 214, 0, 24, 24,
					 g_gr->get_no_picture(),
					 g_gr->get_picture(PicMod_UI, "pics/scrollbar_up.png"),
					 _("Increase permanent target quantity"))
			{
				set_repeating(true);
			}
			void clicked() {
				Worker_Type_Box const & parent =
					ref_cast<Worker_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Ware_Index const worker_type = parent.worker_type;
				Economy::Target_Quantity const & tq =
					e.worker_target_quantity(worker_type);
				assert(tq.permanent <= tq.temporary);
				uint32_t const new_permanent = tq.permanent + 1;
				Widelands::Player & player = e.owner();
				Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
				game.send_player_command
					(*new Widelands::Cmd_SetWorkerTargetQuantity
					 	(game.get_gametime(), player.player_number(),
					 	 player.get_economy_number(&e), worker_type,
					 	 new_permanent, std::max(new_permanent, tq.temporary)));
			}
		} increase_permanent;

		struct Decrease_Temporary : public UI::Button {
			Decrease_Temporary(Worker_Type_Box & parent) :
				UI::Button
					(&parent, 280, 0, 24, 24,
					 g_gr->get_no_picture(),
					 g_gr->get_picture(PicMod_UI, "pics/scrollbar_down.png"),
					 _("Decrease temporary target quantity"))
			{
				set_repeating(true);
			}
			void clicked() {
				Worker_Type_Box const & parent =
					ref_cast<Worker_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Ware_Index const worker_type = parent.worker_type;
				Economy::Target_Quantity const & tq =
					e.worker_target_quantity(worker_type);
				assert(tq.permanent <= tq.temporary);
				if (1 < tq.temporary) {
					uint32_t const new_temporary = tq.temporary - 1;
					Widelands::Player & player = e.owner();
					Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
					game.send_player_command
						(*new Widelands::Cmd_SetWorkerTargetQuantity
						 	(game.get_gametime(), player.player_number(),
						 	 player.get_economy_number(&e), worker_type,
						 	 std::min(tq.permanent, new_temporary), new_temporary));
				}
			}
		} decrease_temporary;

		struct Increase_Temporary : public UI::Button {
			Increase_Temporary(Worker_Type_Box & parent) :
				UI::Button
					(&parent, 304, 0, 24, 24,
					 g_gr->get_no_picture(),
					 g_gr->get_picture(PicMod_UI, "pics/scrollbar_up.png"),
					 _("Increase temporary target quantity"))
			{
				set_repeating(true);
			}
			void clicked() {
				Worker_Type_Box const & parent =
					ref_cast<Worker_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Ware_Index const worker_type = parent.worker_type;
				Economy::Target_Quantity const & tq =
					e.worker_target_quantity(worker_type);
				assert(tq.permanent <= tq.temporary);
				Widelands::Player & player = e.owner();
				Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
				game.send_player_command
					(*new Widelands::Cmd_SetWorkerTargetQuantity
					 	(game.get_gametime(), player.player_number(),
					 	 player.get_economy_number(&e), worker_type,
					 	 tq.permanent, tq.temporary + 1));
			}
		} increase_temporary;

		struct Reset : UI::Button {
			Reset(Worker_Type_Box & parent) :
				UI::Button
					(&parent, 330, 0, 90, 24, g_gr->get_no_picture(),
					 _("Reset"), _("Reset target quantity to default value"))
			{}
			void clicked() {
				Worker_Type_Box const & parent =
					ref_cast<Worker_Type_Box, UI::Panel>(*get_parent());
				Economy & e = parent.economy();
				Widelands::Player & player = e.owner();
				Game & game = ref_cast<Game, Editor_Game_Base>(player.egbase());
				game.send_player_command
					(*new Widelands::Cmd_ResetWorkerTargetQuantity
					 	(game.get_gametime(), player.player_number(),
					 	 player.get_economy_number(&e), parent.worker_type));
			}
		} reset;

		Ware_Index           const worker_type;
		Worker_Descr const &       descr;
	};

	struct Tab_Panel : public UI::Tab_Panel {
		Tab_Panel(Economy_Options_Window & parent, Economy & _economy) :
			UI::Tab_Panel
				(&parent, 0, 0, g_gr->get_picture(PicMod_UI, "pics/but1.png")),
			m_ware_target_quantities  (*this, _economy.owner().tribe()),
			m_worker_target_quantities(*this, _economy.owner().tribe()),
			m_economy(_economy)
		{
			add
				(g_gr->get_picture(PicMod_UI, "pics/genstats_nrwares.png"),
				 &m_ware_target_quantities,
				 _("Ware type target quantities"));
			add
				(g_gr->get_picture(PicMod_UI, "pics/genstats_nrworkers.png"),
				 &m_worker_target_quantities,
				 _("Worker type target quantities"));
			set_snapparent(true);
			resize();
		}

		Economy & economy() const {return m_economy;}

		struct Ware_Target_Quantities : public UI::Box {
			Ware_Target_Quantities
				(Tab_Panel & parent, Widelands::Tribe_Descr const & tribe)
				:
				UI::Box
					(&parent, 0, 0, UI::Box::Vertical,
					 g_gr->get_xres() - 80, g_gr->get_yres() - 100)
			{
				Ware_Index const nr_wares = tribe.get_nrwares();
				for (Ware_Index i = Ware_Index::First(); i < nr_wares; ++i) {
					Item_Ware_Descr const & descr = *tribe.get_ware_descr(i);
					if (descr.has_demand_check())
						add(new Ware_Type_Box(*this, i, descr), UI::Box::AlignTop);
				}
				set_scrolling(true);
			}
		} m_ware_target_quantities;
		struct Worker_Target_Quantities : public UI::Box {
			Worker_Target_Quantities
				(Tab_Panel & parent, Widelands::Tribe_Descr const & tribe)
				:
				UI::Box
					(&parent, 0, 0, UI::Box::Vertical,
					 g_gr->get_xres() - 80, g_gr->get_yres() - 100)
			{
				Ware_Index const nr_workers = tribe.get_nrworkers();
				for (Ware_Index i = Ware_Index::First(); i < nr_workers; ++i) {
					Worker_Descr const & descr = *tribe.get_worker_descr(i);
					if (descr.has_demand_check())
						add(new Worker_Type_Box(*this, i, descr), UI::Box::AlignTop);
				}
				set_scrolling(true);
			}
		} m_worker_target_quantities;
		Economy & m_economy;
	} m_tabpanel;
};


/**
 * \todo: Neither this function nor the UI Registry should be part
 * of Economy. Economy should be made an observerable class where
 * users can register for change updates. The registry should be
 * moved to InteractivePlayer or some other UI component.
 */
void Economy::show_options_window() {
	if (m_optionswindow_registry.window)
		m_optionswindow_registry.window->move_to_top();
	else
		new Economy_Options_Window
			(ref_cast<Interactive_GameBase, Interactive_Base>
			 	(*owner().egbase().get_ibase()),
			 *this);
}
