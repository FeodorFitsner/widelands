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

#include "graphic/graphic.h"
#include "graphic/rendertarget.h"
#include "logic/map_objects/tribes/warehouse.h"
#include "logic/player.h"
#include "logic/playercommand.h"
#include "ui_basic/tabpanel.h"
#include "wui/buildingwindow.h"
#include "wui/portdockwaresdisplay.h"
#include "wui/waresdisplay.h"

using Widelands::Warehouse;

static const char pic_tab_wares[] = "images/wui/buildings/menu_tab_wares.png";
static const char pic_tab_workers[] = "images/wui/buildings/menu_tab_workers.png";
static const char pic_tab_dock_wares[] = "images/wui/buildings/menu_tab_wares_dock.png";
static const char pic_tab_dock_workers[] = "images/wui/buildings/menu_tab_workers_dock.png";
static const char pic_tab_expedition[] = "images/wui/buildings/start_expedition.png";

static const char pic_policy_prefer[] = "images/wui/buildings/stock_policy_prefer.png";
static const char pic_policy_dontstock[] = "images/wui/buildings/stock_policy_dontstock.png";
static const char pic_policy_remove[] = "images/wui/buildings/stock_policy_remove.png";

/**
 * Extends the wares display to show and modify stock policy of items.
 */
class WarehouseWaresDisplay : public WaresDisplay {
public:
	WarehouseWaresDisplay
		(UI::Panel * parent, uint32_t width,
		 Warehouse & wh, Widelands::WareWorker type, bool selectable);

protected:
	void draw_ware(RenderTarget & dst, Widelands::DescriptionIndex ware) override;

private:
	Warehouse & warehouse_;
};

WarehouseWaresDisplay::WarehouseWaresDisplay
	(UI::Panel * parent, uint32_t width,
	 Warehouse & wh, Widelands::WareWorker type, bool selectable)
:
WaresDisplay(parent, 0, 0, wh.owner().tribe(), type, selectable),
warehouse_(wh)
{
	set_inner_size(width, 0);
	add_warelist(type == Widelands::wwWORKER ? warehouse_.get_workers() : warehouse_.get_wares());
	if (type == Widelands::wwWORKER)
	{
		const std::vector<Widelands::DescriptionIndex> & worker_types_without_cost =
			warehouse_.owner().tribe().worker_types_without_cost();
		for (size_t i = 0; i < worker_types_without_cost.size(); ++i)
		{
			hide_ware(worker_types_without_cost.at(i));
		}
	}
}

void WarehouseWaresDisplay::draw_ware(RenderTarget & dst, Widelands::DescriptionIndex ware)
{
	WaresDisplay::draw_ware(dst, ware);

	Warehouse::StockPolicy policy = warehouse_.get_stock_policy(get_type(), ware);
	const Image* pic = nullptr;
	switch (policy) {
	case Warehouse::SP_Prefer:
		pic = g_gr->images().get(pic_policy_prefer);
		break;
	case Warehouse::SP_DontStock:
		pic = g_gr->images().get(pic_policy_dontstock);
		break;
	case Warehouse::SP_Remove:
		pic = g_gr->images().get(pic_policy_remove);
		break;
	case Warehouse::SP_Normal:
		// don't draw anything for the normal policy
		return;
	}
	assert(pic != nullptr);

	dst.blit(ware_position(ware), pic);
}

/**
 * Wraps the wares display together with some buttons
 */
struct WarehouseWaresPanel : UI::Box {
	WarehouseWaresPanel
		(UI::Panel * parent, uint32_t width,
		 InteractiveGameBase &, Warehouse &, Widelands::WareWorker type);

	void set_policy(Warehouse::StockPolicy);
private:
	InteractiveGameBase & gb_;
	Warehouse & wh_;
	bool can_act_;
	Widelands::WareWorker type_;
	WarehouseWaresDisplay display_;
};

WarehouseWaresPanel::WarehouseWaresPanel
	(UI::Panel * parent, uint32_t width,
	 InteractiveGameBase & gb, Warehouse & wh, Widelands::WareWorker type)
:
	UI::Box(parent, 0, 0, UI::Box::Vertical),
	gb_(gb),
	wh_(wh),
	can_act_(gb_.can_act(wh_.owner().player_number())),
	type_(type),
	display_(this, width, wh_, type_, can_act_)
{
	add(&display_, UI::Align::kLeft, true);

	if (can_act_) {
		UI::Box * buttons = new UI::Box(this, 0, 0, UI::Box::Horizontal);
		UI::Button * b;
		add(buttons, UI::Align::kLeft);

#define ADD_POLICY_BUTTON(policy, policyname, tooltip)                                           \
		b = new UI::Button                                                             \
			(buttons, #policy, 0, 0, 34, 34,                                                  \
			 g_gr->images().get("images/ui_basic/but4.png"),                                   \
			 g_gr->images().get("images/wui/buildings/stock_policy_button_" #policy ".png"),      \
			 tooltip),                                                                        \
		b->sigclicked.connect \
			(boost::bind(&WarehouseWaresPanel::set_policy, this, Warehouse::SP_##policyname)), \
        buttons->add(b, UI::Align::kHCenter);

		ADD_POLICY_BUTTON(normal, Normal, _("Normal policy"))
		ADD_POLICY_BUTTON(prefer, Prefer, _("Preferably store selected wares here"))
		ADD_POLICY_BUTTON(dontstock, DontStock, _("Do not store selected wares here"))
		ADD_POLICY_BUTTON(remove, Remove, _("Remove selected wares from here"))
	}
}

/**
 * Add Buttons policy buttons
 */
void WarehouseWaresPanel::set_policy(Warehouse::StockPolicy newpolicy) {
	if (gb_.can_act(wh_.owner().player_number())) {
		bool is_workers = type_ == Widelands::wwWORKER;
		const std::set<Widelands::DescriptionIndex> indices =
				is_workers ? wh_.owner().tribe().workers() : wh_.owner().tribe().wares();

		for (const Widelands::DescriptionIndex& index : indices) {
			if (display_.ware_selected(index)) {
				gb_.game().send_player_command
					(*new Widelands::CmdSetStockPolicy
						(gb_.game().get_gametime(),
						 wh_.owner().player_number(),
						 wh_, is_workers,
						 index, newpolicy));
			}
		}
	}
}


/**
 * Status window for warehouses
 */
struct WarehouseWindow : public BuildingWindow {
	WarehouseWindow
		(InteractiveGameBase & parent, Warehouse &, UI::Window * & registry);

	Warehouse & warehouse() {
		return dynamic_cast<Warehouse&>(building());
	}
};

/**
 * Create the tabs of a warehouse window.
 */
WarehouseWindow::WarehouseWindow
	(InteractiveGameBase & parent,
	 Warehouse            & wh,
	 UI::Window *         & registry)
	: BuildingWindow(parent, wh, registry)
{
	get_tabs()->add
		("wares",
		 g_gr->images().get(pic_tab_wares),
		 new WarehouseWaresPanel
			(get_tabs(),
			 Width,
			 igbase(),
			 warehouse(),
			 Widelands::wwWARE),
		 _("Wares"));
	get_tabs()->add
		("workers",
		 g_gr->images().get(pic_tab_workers),
		 new WarehouseWaresPanel
			(get_tabs(),
			 Width,
			 igbase(),
			 warehouse(),
			 Widelands::wwWORKER),
		 _("Workers"));

	if (Widelands::PortDock * pd = wh.get_portdock()) {
		get_tabs()->add
			("dock_wares",
			 g_gr->images().get(pic_tab_dock_wares),
			 create_portdock_wares_display(get_tabs(), Width, *pd, Widelands::wwWARE),
			 _("Wares waiting to be shipped"));
		get_tabs()->add
			("dock_workers",
			 g_gr->images().get(pic_tab_dock_workers),
			 create_portdock_wares_display(get_tabs(), Width, *pd, Widelands::wwWORKER),
			 _("Workers waiting to embark"));
		if (pd->expedition_started()) {
			get_tabs()->add
				("expedition_wares_queue",
				 g_gr->images().get(pic_tab_expedition),
				 create_portdock_expedition_display(get_tabs(), warehouse(), igbase()),
				 _("Expedition"));
		}
	}
}

/**
 * Create the status window describing the warehouse.
 */
void Widelands::Warehouse::create_options_window
	(InteractiveGameBase & parent, UI::Window * & registry)
{
	new WarehouseWindow(parent, *this, registry);
}
