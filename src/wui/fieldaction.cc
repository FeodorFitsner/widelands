/*
 * Copyright (C) 2002-2004, 2006-2011, 2013 by the Widelands Development Team
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

#include "wui/fieldaction.h"

#include "base/i18n.h"
#include "base/macros.h"
#include "economy/economy.h"
#include "economy/flag.h"
#include "economy/road.h"
#include "graphic/graphic.h"
#include "graphic/image_transformations.h"
#include "logic/attackable.h"
#include "logic/cmd_queue.h"
#include "logic/maphollowregion.h"
#include "logic/player.h"
#include "logic/soldier.h"
#include "logic/tribe.h"
#include "logic/warehouse.h"
#include "ui_basic/box.h"
#include "ui_basic/button.h"
#include "ui_basic/icongrid.h"
#include "ui_basic/tabpanel.h"
#include "ui_basic/textarea.h"
#include "ui_basic/unique_window.h"
#include "wui/actionconfirm.h"
#include "wui/attack_box.h"
#include "wui/game_debug_ui.h"
#include "wui/interactive_player.h"
#include "wui/overlay_manager.h"
#include "wui/waresdisplay.h"
#include "wui/watchwindow.h"

namespace Widelands {struct BuildingDescr;}
using Widelands::Building;
using Widelands::EditorGameBase;
using Widelands::Game;

#define BG_CELL_WIDTH  34 // extents of one cell
#define BG_CELL_HEIGHT 34

//sizes for the images in the build menu (containing building icons)
#define BUILDMENU_IMAGE_SIZE 30. // used for width and height

// The BuildGrid presents a selection of buildable buildings
struct BuildGrid : public UI::IconGrid {
	BuildGrid(UI::Panel* parent,
	          const RGBColor& player_color,
	          const Widelands::TribeDescr& tribe,
	          int32_t x,
	          int32_t y,
	          int32_t cols);

	boost::signals2::signal<void(Widelands::BuildingIndex)> buildclicked;
	boost::signals2::signal<void(Widelands::BuildingIndex)> buildmouseout;
	boost::signals2::signal<void(Widelands::BuildingIndex)> buildmousein;

	void add(Widelands::BuildingIndex);

private:
	void click_slot(int32_t idx);
	void mouseout_slot(int32_t idx);
	void mousein_slot(int32_t idx);

private:
	const RGBColor player_color_;
	const Widelands::TribeDescr& tribe_;
};

BuildGrid::BuildGrid(
		UI::Panel* parent, const RGBColor& player_color, const Widelands::TribeDescr& tribe,
		int32_t x, int32_t y, int32_t cols) :
	UI::IconGrid(parent, x, y, BG_CELL_WIDTH, BG_CELL_HEIGHT, cols),
	player_color_(player_color),
	tribe_(tribe)
{
	clicked.connect(boost::bind(&BuildGrid::click_slot, this, _1));
	mouseout.connect(boost::bind(&BuildGrid::mouseout_slot, this, _1));
	mousein.connect(boost::bind(&BuildGrid::mousein_slot, this, _1));
}

/*
===============
Add a new building to the list of buildable buildings
===============
*/
void BuildGrid::add(Widelands::BuildingIndex id)
{
	const Widelands::BuildingDescr & descr =
		*tribe_.get_building_descr(Widelands::BuildingIndex(id));
	const Image& anim_frame = g_gr->animations().get_animation(descr.get_animation("idle"))
		.representative_image(player_color_);
	const uint16_t image_w = anim_frame.width();
	const uint16_t image_h = anim_frame.height();
	double ratio = BUILDMENU_IMAGE_SIZE / std::max(image_w, image_h);
	const Image* menu_image = ImageTransformations::resize(&anim_frame, image_w * ratio, image_h * ratio);
	UI::IconGrid::add
		(descr.name(), menu_image,
		 reinterpret_cast<void *>(id),
		 descr.descname() + "<br><font size=11>" + _("Construction costs:") + "</font><br>" +
			waremap_to_richtext(tribe_, descr.buildcost()));
}


/*
===============
BuildGrid::click_slot [private]

The icon with the given index has been clicked. Figure out which building it
belongs to and trigger signal buildclicked.
===============
*/
void BuildGrid::click_slot(int32_t idx)
{
	buildclicked(static_cast<int32_t>(reinterpret_cast<intptr_t>(get_data(idx))));
}


/*
===============
BuildGrid::mouseout_slot [private]

The mouse pointer has left the icon with the given index. Figure out which
building it belongs to and trigger signal buildmouseout.
===============
*/
void BuildGrid::mouseout_slot(int32_t idx)
{
	buildmouseout(static_cast<int32_t>(reinterpret_cast<intptr_t>(get_data(idx))));
}


/*
===============
BuildGrid::mousein_slot [private]

The mouse pointer has entered the icon with the given index. Figure out which
building it belongs to and trigger signal buildmousein.
===============
*/
void BuildGrid::mousein_slot(int32_t idx)
{
	buildmousein(static_cast<int32_t>(reinterpret_cast<intptr_t>(get_data(idx))));
}



/*
==============================================================================

FieldActionWindow IMPLEMENTATION

==============================================================================
*/
class FieldActionWindow : public UI::UniqueWindow {
public:
	FieldActionWindow
		(InteractiveBase           * ibase,
		 Widelands::Player          * plr,
		 UI::UniqueWindow::Registry * registry);
	~FieldActionWindow();

	InteractiveBase & ibase() {
		return dynamic_cast<InteractiveBase&>(*get_parent());
	}

	void think() override;

	void init();
	void add_buttons_auto();
	void add_buttons_build(int32_t buildcaps, const RGBColor& player_color);
	void add_buttons_road(bool flag);
	void add_buttons_attack();

	void act_watch();
	void act_show_census();
	void act_show_statistics();
	void act_debug();
	void act_buildflag();
	void act_configure_economy();
	void act_ripflag();
	void act_buildroad();
	void act_abort_buildroad();
	void act_removeroad();
	void act_build              (Widelands::BuildingIndex);
	void building_icon_mouse_out(Widelands::BuildingIndex);
	void building_icon_mouse_in (Widelands::BuildingIndex);
	void act_geologist();
	void act_attack();      /// Launch the attack

	/// Total number of attackers available for a specific enemy flag
	uint32_t get_max_attackers();

private:
	uint32_t add_tab
		(const std::string & name,
		 ImageCatalog::Keys image_key,
		 UI::Panel * panel,
		 const std::string & tooltip_text = "");
	UI::Button & add_button
		(UI::Box *,
		 const char * name,
		 ImageCatalog::Keys image_key,
		 void (FieldActionWindow::*fn)(),
		 const std::string & tooltip_text,
		 bool repeating = false);
	void okdialog();

	Widelands::Player * m_plr;
	Widelands::Map    * m_map;
	OverlayManager & m_overlay_manager;

	Widelands::FCoords  m_node;

	UI::TabPanel      m_tabpanel;
	bool m_fastclick; // if true, put the mouse over first button in first tab
	uint32_t m_best_tab;
	OverlayManager::JobId m_workarea_preview_job_id;

	/// Variables to use with attack dialog.
	AttackBox * m_attack_box;
};

static const ImageCatalog::Keys pic_tab_buildhouse[] = {
	ImageCatalog::Keys::kFieldTabBuildSmall,
	ImageCatalog::Keys::kFieldTabBuildMedium,
	ImageCatalog::Keys::kFieldTabBuildBig,
	ImageCatalog::Keys::kFieldTabBuildPort
};
static const std::string tooltip_tab_build[] = {
	_("Build small building"),
	_("Build medium building"),
	_("Build large building"),
	_("Build port building")
};
static const std::string name_tab_build[] = {"small", "medium", "big", "port"};

/*
===============
Initialize a field action window, creating the appropriate buttons.
===============
*/
FieldActionWindow::FieldActionWindow
	(InteractiveBase           * const ib,
	 Widelands::Player          * const plr,
	 UI::UniqueWindow::Registry * const registry)
:
	UI::UniqueWindow(ib, "field_action", registry, 68, 34, _("Action")),
	m_plr(plr),
	m_map(&ib->egbase().map()),
	m_overlay_manager(*m_map->get_overlay_manager()),
	m_node(ib->get_sel_pos().node, &(*m_map)[ib->get_sel_pos().node]),
	m_tabpanel(this, 0, 0, g_gr->cataloged_image(ImageCatalog::Keys::kButton1)),
	m_fastclick(true),
	m_best_tab(0),
	m_workarea_preview_job_id(0),
	m_attack_box(nullptr)
{
	ib->set_sel_freeze(true);


	set_center_panel(&m_tabpanel);
}


FieldActionWindow::~FieldActionWindow()
{
	if (m_workarea_preview_job_id)
		m_overlay_manager.remove_overlay(m_workarea_preview_job_id);
	ibase().set_sel_freeze(false);
	delete m_attack_box;
}


void FieldActionWindow::think() {
	if
		(m_plr && m_plr->vision(m_node.field - &ibase().egbase().map()[0]) <= 1
		 && !m_plr->see_all())
		die();
}


/*
===============
Initialize after buttons have been registered.
This mainly deals with mouse placement
===============
*/
void FieldActionWindow::init()
{
	center_to_parent(); // override UI::UniqueWindow position
	move_out_of_the_way();

	// Now force the mouse onto the first button
	set_mouse_pos
		(Point(17 + BG_CELL_WIDTH * m_best_tab, m_fastclick ? 51 : 17));

	// Will only do something if we explicitly set another fast click panel
	// than the first button
	warp_mouse_to_fastclick_panel();
}


/*
===============
Add the buttons you normally get when clicking on a field.
===============
*/
void FieldActionWindow::add_buttons_auto()
{
	UI::Box * buildbox = nullptr;
	UI::Box & watchbox = *new UI::Box(&m_tabpanel, 0, 0, UI::Box::Horizontal);

	// Add road-building actions
	upcast(InteractiveGameBase, igbase, &ibase());

	const Widelands::PlayerNumber owner = m_node.field->get_owned_by();

	if (!igbase || igbase->can_see(owner)) {
		Widelands::BaseImmovable * const imm = m_map->get_immovable(m_node);
		const bool can_act = igbase ? igbase->can_act(owner) : true;

		// The box with road-building buttons
		buildbox = new UI::Box(&m_tabpanel, 0, 0, UI::Box::Horizontal);

		if (upcast(Widelands::Flag, flag, imm)) {
			// Add flag actions
			if (can_act) {
				add_button
					(buildbox, "build_road",
					 ImageCatalog::Keys::kFieldRoadBuild,
					 &FieldActionWindow::act_buildroad,
					 _("Build road"));

				Building * const building = flag->get_building();

				if
					(!building
					 ||
					 (building->get_playercaps() & Building::PCap_Bulldoze))
					add_button
						(buildbox, "rip_flag",
						 ImageCatalog::Keys::kFieldFlagDestroy,
						 &FieldActionWindow::act_ripflag,
						 _("Destroy this flag"));
			}

			if (dynamic_cast<Game const *>(&ibase().egbase())) {
				add_button
					(buildbox, "configure_economy",
					 ImageCatalog::Keys::kStatsWaresNumber,
					 &FieldActionWindow::act_configure_economy,
					 _("Configure economy"));
				if (can_act)
					add_button
						(buildbox, "geologist",
						 ImageCatalog::Keys::kFieldGeologist,
						 &FieldActionWindow::act_geologist,
						 _("Send geologist to explore site"));
			}
		} else {
			const int32_t buildcaps = m_plr ? m_plr->get_buildcaps(m_node) : 0;

			// Add house building
			if ((buildcaps & Widelands::BUILDCAPS_SIZEMASK) ||
			    (buildcaps & Widelands::BUILDCAPS_MINE)) {
				assert(igbase->get_player());
				add_buttons_build(buildcaps, igbase->get_player()->get_playercolor());
			}

			// Add build actions
			if ((m_fastclick = buildcaps & Widelands::BUILDCAPS_FLAG))
				add_button
					(buildbox, "build_flag",
					 ImageCatalog::Keys::kFieldFlagBuild,
					 &FieldActionWindow::act_buildflag,
					 _("Place a flag"));

			if (can_act && dynamic_cast<const Widelands::Road *>(imm))
				add_button
					(buildbox, "destroy_road",
					 ImageCatalog::Keys::kFieldRoadDestroy,
					 &FieldActionWindow::act_removeroad,
					 _("Destroy a road"));
		}
	} else if
		(m_plr &&
		 1
		 <
		 m_plr->vision
		 	(Widelands::Map::get_index
		 	 	(m_node, ibase().egbase().map().get_width())))
		add_buttons_attack ();

	//  Watch actions, only when game (no use in editor) same for statistics.
	//  census is ok
	if (dynamic_cast<const Game *>(&ibase().egbase())) {
		add_button
			(&watchbox, "watch",
			 ImageCatalog::Keys::kFieldWatch,
			 &FieldActionWindow::act_watch,
			 _("Watch field in a separate window"));
		add_button
			(&watchbox, "statistics",
			 ImageCatalog::Keys::kFieldStatistics,
			 &FieldActionWindow::act_show_statistics,
			 _("Toggle building statistics display"));
	}
	add_button
		(&watchbox, "census",
		 ImageCatalog::Keys::kFieldCensus,
		 &FieldActionWindow::act_show_census,
		 _("Toggle building label display"));

	if (ibase().get_display_flag(InteractiveBase::dfDebug))
		add_button
			(&watchbox, "debug",
			 ImageCatalog::Keys::kFieldDebug,
			 &FieldActionWindow::act_debug,
			 _("Debug window"));

	// Add tabs
	if (buildbox && buildbox->get_nritems())
		add_tab("roads", ImageCatalog::Keys::kFieldTabBuildRoad, buildbox, _("Build road"));

	add_tab("watch", ImageCatalog::Keys::kFieldTabWatch, &watchbox, _("Watch"));

}

void FieldActionWindow::add_buttons_attack ()
{
	UI::Box & a_box = *new UI::Box(&m_tabpanel, 0, 0, UI::Box::Horizontal);

	if
		(upcast
		 	(Widelands::Attackable, attackable, m_map->get_immovable(m_node)))
	{
		if
			(m_plr && m_plr->is_hostile(attackable->owner()) &&
			 attackable->can_attack())
		{
			m_attack_box = new AttackBox(&a_box, m_plr, &m_node, 0, 0);
			a_box.add(m_attack_box, UI::Box::AlignTop);

			set_fastclick_panel
				(&add_button
				 (&a_box, "attack",
				  ImageCatalog::Keys::kBuildingAttack,
				  &FieldActionWindow::act_attack,
				  _("Start attack")));
		}
	}

	if (a_box.get_nritems()) { //  add tab
		add_tab("attack", ImageCatalog::Keys::kFieldAttack, &a_box, _("Attack"));
	}
}

/*
===============
Add buttons for house building.
===============
*/
void FieldActionWindow::add_buttons_build(int32_t buildcaps, const RGBColor& player_color)
{
	if (!m_plr)
		return;
	BuildGrid * bbg_house[4] = {nullptr, nullptr, nullptr, nullptr};
	BuildGrid * bbg_mine = nullptr;

	const Widelands::TribeDescr & tribe = m_plr->tribe();

	m_fastclick = false;

	const Widelands::BuildingIndex nr_buildings = tribe.get_nrbuildings();
	for
		(Widelands::BuildingIndex id = 0;
		 id < nr_buildings;
		 ++id)
	{
		const Widelands::BuildingDescr & descr = *tribe.get_building_descr(id);
		BuildGrid * * ppgrid;

		//  Some building types cannot be built (i.e. construction site) and not
		//  allowed buildings.
		if (dynamic_cast<const Game *>(&ibase().egbase())) {
			if (!descr.is_buildable() || !m_plr->is_building_type_allowed(id))
				continue;
		} else if (!descr.is_buildable() && !descr.is_enhanced())
			continue;

		// Figure out if we can build it here, and in which tab it belongs
		if (descr.get_ismine()) {
			if (!(buildcaps & Widelands::BUILDCAPS_MINE))
				continue;

			ppgrid = &bbg_mine;
		} else {
			int32_t size = descr.get_size() - Widelands::BaseImmovable::SMALL;

			if ((buildcaps & Widelands::BUILDCAPS_SIZEMASK) < size + 1)
				continue;
			if (descr.get_isport() && !(buildcaps & Widelands::BUILDCAPS_PORT))
				continue;

			if (descr.get_isport())
				ppgrid = &bbg_house[3];
			else
				ppgrid = &bbg_house[size];
		}

		// Allocate the tab's grid if necessary
		if (!*ppgrid) {
			*ppgrid = new BuildGrid(&m_tabpanel, player_color, tribe, 0, 0, 5);
			(*ppgrid)->buildclicked.connect(boost::bind(&FieldActionWindow::act_build, this, _1));
			(*ppgrid)->buildmouseout.connect
				(boost::bind(&FieldActionWindow::building_icon_mouse_out, this, _1));

			(*ppgrid)->buildmousein.connect
				(boost::bind(&FieldActionWindow::building_icon_mouse_in, this, _1));
		}

		// Add it to the grid
		(*ppgrid)->add(id);
	}

	// Add all necessary tabs
	for (int32_t i = 0; i < 4; ++i)
		if (bbg_house[i])
			m_tabpanel.activate
				(m_best_tab = add_tab
				 	(name_tab_build[i], pic_tab_buildhouse[i],
				 	 bbg_house[i],
				 	 i18n::translate(tooltip_tab_build[i])));

	if (bbg_mine)
		m_tabpanel.activate
			(m_best_tab = add_tab
				("mines", ImageCatalog::Keys::kFieldTabBuildMine, bbg_mine, _("Build mines")));
}


/*
===============
Buttons used during road building: Set flag here and Abort
===============
*/
void FieldActionWindow::add_buttons_road(bool flag)
{
	UI::Box & buildbox = *new UI::Box(&m_tabpanel, 0, 0, UI::Box::Horizontal);

	if (flag)
		add_button
			(&buildbox, "build_flag",
			 ImageCatalog::Keys::kFieldFlagBuild,
			 &FieldActionWindow::act_buildflag, _("Build flag"));

	add_button
		(&buildbox, "cancel_road",
		 ImageCatalog::Keys::kButtonMenuAbort,
		 &FieldActionWindow::act_abort_buildroad, _("Cancel road"));

	// Add the box as tab
	add_tab("roads", ImageCatalog::Keys::kFieldRoadBuild, &buildbox, _("Build roads"));
}


/*
===============
Convenience function: Adds a new tab to the main tab panel
===============
*/
uint32_t FieldActionWindow::add_tab
	(const std::string & name, ImageCatalog::Keys image_key,
	 UI::Panel * panel, const std::string & tooltip_text)
{
	return
		m_tabpanel.add
			(name, g_gr->cataloged_image(image_key), panel, tooltip_text);
}


UI::Button & FieldActionWindow::add_button
	(UI::Box           * const box,
	 const char        * const name,
	 ImageCatalog::Keys        image_key,
	 void (FieldActionWindow::*fn)(),
	 const std::string & tooltip_text,
	 bool                repeating)
{
	UI::Button & button =
		*new UI::Button
			(box, name,
			 0, 0, 34, 34,
			 g_gr->cataloged_image(ImageCatalog::Keys::kButton2),
			 g_gr->cataloged_image(image_key),
			 tooltip_text);
	button.sigclicked.connect(boost::bind(fn, this));
	button.set_repeating(repeating);
	box->add
		(&button, UI::Box::AlignTop);

	return button;
}

/*
===============
Call this from the button handlers.
It resets the mouse to its original position and closes the window
===============
*/
void FieldActionWindow::okdialog()
{
	ibase().warp_mouse_to_node(m_node);
	die();
}

/*
===============
Open a watch window for the given field and delete self.
===============
*/
void FieldActionWindow::act_watch()
{
	upcast(InteractiveGameBase, igbase, &ibase());
	show_watch_window(*igbase, m_node);
	okdialog();
}


/*
===============
Toggle display of census and statistics for buildings, respectively.
===============
*/
void FieldActionWindow::act_show_census()
{
	ibase().set_display_flag
		(InteractiveBase::dfShowCensus,
		 !ibase().get_display_flag(InteractiveBase::dfShowCensus));
	okdialog();
}

void FieldActionWindow::act_show_statistics()
{
	ibase().set_display_flag
		(InteractiveBase::dfShowStatistics,
		 !ibase().get_display_flag(InteractiveBase::dfShowStatistics));
	okdialog();
}


/*
===============
Show a debug widow for this field.
===============
*/
void FieldActionWindow::act_debug()
{
	show_field_debug(ibase(), m_node);
}


/*
===============
Build a flag at this field
===============
*/
void FieldActionWindow::act_buildflag()
{
	upcast(Game, game, &ibase().egbase());
	if (game)
		game->send_player_build_flag(m_plr->player_number(), m_node);
	else
		m_plr->build_flag(m_node);

	if (ibase().is_building_road())
		ibase().finish_build_road();
	else if (game) {
		upcast(InteractivePlayer, iaplayer, &ibase());
		iaplayer->set_flag_to_connect(m_node);
	}

	okdialog();
}


void FieldActionWindow::act_configure_economy()
{
	if (upcast(const Widelands::Flag, flag, m_node.field->get_immovable()))
		flag->get_economy()->show_options_window();
}


/*
===============
Remove the flag at this field
===============
*/
void FieldActionWindow::act_ripflag()
{
	okdialog();
	Widelands::EditorGameBase & egbase = ibase().egbase();
	upcast(Game, game, &egbase);
	upcast(InteractivePlayer, iaplayer, &ibase());

	if (upcast(Widelands::Flag, flag, m_node.field->get_immovable())) {
		if (Building * const building = flag->get_building()) {
			if (building->get_playercaps() & Building::PCap_Bulldoze) {
				if (get_key_state(SDL_SCANCODE_LCTRL) || get_key_state(SDL_SCANCODE_RCTRL)) {
					game->send_player_bulldoze
						(*flag, get_key_state(SDL_SCANCODE_LCTRL) || get_key_state(SDL_SCANCODE_RCTRL));
				}
				else {
					show_bulldoze_confirm(*iaplayer, *building, flag);
				}
			}
		} else {
			game->send_player_bulldoze
					(*flag, get_key_state(SDL_SCANCODE_LCTRL) || get_key_state(SDL_SCANCODE_RCTRL));
		}
	}
}


/*
===============
Start road building.
===============
*/
void FieldActionWindow::act_buildroad()
{
	//if we area already building a road just ignore this
	if (!ibase().is_building_road()) {
		ibase().start_build_road(m_node, m_plr->player_number());
		okdialog();
	}
}

/*
===============
Abort building a road.
===============
*/
void FieldActionWindow::act_abort_buildroad()
{
	if (!ibase().is_building_road())
		return;

	ibase().abort_build_road();
	okdialog();
}

/*
===============
Remove the road at the given field
===============
*/
void FieldActionWindow::act_removeroad()
{
	Widelands::EditorGameBase & egbase = ibase().egbase();
	if (upcast(Widelands::Road, road, egbase.map().get_immovable(m_node))) {
		upcast(Game, game, &ibase().egbase());
		game->send_player_bulldoze
			(*road, get_key_state(SDL_SCANCODE_LCTRL) || get_key_state(SDL_SCANCODE_RCTRL));
	}
	okdialog();
}


/*
===============
Start construction of the building with the give description index
===============
*/
void FieldActionWindow::act_build(Widelands::BuildingIndex idx)
{
	upcast(Game, game, &ibase().egbase());
	upcast(InteractivePlayer, iaplayer, &ibase());

	game->send_player_build(iaplayer->player_number(), m_node, Widelands::BuildingIndex(idx));
	ibase().reference_player_tribe(m_plr->player_number(), &m_plr->tribe());
	iaplayer->set_flag_to_connect(game->map().br_n(m_node));
	okdialog();
}


void FieldActionWindow::building_icon_mouse_out
	(Widelands::BuildingIndex)
{
	if (m_workarea_preview_job_id) {
		m_overlay_manager.remove_overlay(m_workarea_preview_job_id);
		m_workarea_preview_job_id = 0;
	}
}


void FieldActionWindow::building_icon_mouse_in
	(const Widelands::BuildingIndex idx)
{
	if (ibase().m_show_workarea_preview && !m_workarea_preview_job_id) {
		const WorkareaInfo & workarea_info =
			m_plr->tribe().get_building_descr(Widelands::BuildingIndex(idx))
			->m_workarea_info;
		m_workarea_preview_job_id = ibase().show_work_area(workarea_info, m_node);
	}
}


/*
===============
Call a geologist on this flag.
===============
*/
void FieldActionWindow::act_geologist()
{
	upcast(Game, game, &ibase().egbase());
	if (upcast(Widelands::Flag, flag, game->map().get_immovable(m_node))) {
		game->send_player_flagaction (*flag);
	}
	okdialog();
}

/**
 * Here there are a problem: the sender of an event is always the owner of
 * were is done this even. But for attacks, the owner of an event is the
 * player who start an attack, so is needed to get an extra parameter to
 * the send_player_enemyflagaction, the player number
 */
void FieldActionWindow::act_attack ()
{
	assert(m_attack_box);
	upcast(Game, game, &ibase().egbase());
	if (upcast(Building, building, game->map().get_immovable(m_node)))
		if (m_attack_box->soldiers() > 0) {
			upcast(InteractivePlayer const, iaplayer, &ibase());
			game->send_player_enemyflagaction(
			   building->base_flag(),
				iaplayer->player_number(),
			   m_attack_box->soldiers() /*  number of soldiers */);
		}
	okdialog();
}

/*
===============
show_field_action

Perform a field action (other than building options).
Bring up a field action window or continue road building.
===============
*/
void show_field_action
	(InteractiveBase           * const ibase,
	 Widelands::Player          * const player,
	 UI::UniqueWindow::Registry * const registry)
{
	// Force closing of old fieldaction windows. This is necessary because
	// show_field_action() does not always open a FieldActionWindow (e.g.
	// connecting the road we are building to an existing flag)
	delete registry->window;
	*registry = UI::UniqueWindow::Registry();

	if (!ibase->is_building_road()) {
		FieldActionWindow & w = *new FieldActionWindow(ibase, player, registry);
		w.add_buttons_auto();
		return w.init();
	}

	const Widelands::Map & map = player->egbase().map();

	// we're building a road right now
	const Widelands::FCoords target =
		map.get_fcoords(ibase->get_sel_pos().node);

	// if user clicked on the same field again, build a flag
	if (target == ibase->get_build_road_end()) {
		FieldActionWindow & w = *new FieldActionWindow(ibase, player, registry);
		w.add_buttons_road
			(target != ibase->get_build_road_start()
			 &&
			 (player->get_buildcaps(target) & Widelands::BUILDCAPS_FLAG));
		return w.init();
	}

	// append or take away from the road
	if (!ibase->append_build_road(target)) {
		FieldActionWindow & w = *new FieldActionWindow(ibase, player, registry);
		w.add_buttons_road(false);
		w.init();
		return;
	}

	// did he click on a flag or a road where a flag can be built?

	if (upcast(const Widelands::PlayerImmovable, i, map.get_immovable(target)))
	{
		bool finish = false;
		if      (dynamic_cast<const Widelands::Flag *>(i))
			finish = true;
		else if (dynamic_cast<const Widelands::Road *>(i))
			if (player->get_buildcaps(target) & Widelands::BUILDCAPS_FLAG) {
				upcast(Game, game, &player->egbase());
				game->send_player_build_flag(player->player_number(), target);
				finish = true;
			}
		if (finish)
			ibase->finish_build_road();
	}
}
