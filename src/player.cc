/*
 * Copyright (C) 2002-2003, 2006-2007 by the Widelands Development Team
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

#include "player.h"

#include "attack_controller.h"
#include "cmd_queue.h"
#include "error.h"
#include "game.h"
#include "militarysite.h"
#include "soldier.h"
#include "sound/sound_handler.h"
#include "transport.h"
#include "trainingsite.h"
#include "tribe.h"
#include "warehouse.h"
#include "wexception.h"


extern Map_Object_Descr g_road_descr;

//
//
// class Player
//
//
Player::Player
(Editor_Game_Base  & the_egbase,
 const int type,
 const Player_Number plnum,
 const Tribe_Descr & tribe_descr,
 const std::string & name,
 const uchar * const playercolor)
:
m_see_all(false),
m_egbase (the_egbase),
m_type   (type),
m_plnum  (plnum),
m_tribe  (tribe_descr),
m_fields (0)
{

	for(int i = 0; i < 4; i++)
		m_playercolor[i] = RGBColor(playercolor[i*3 + 0], playercolor[i*3 + 1], playercolor[i*3 + 2]);

   set_name(name);

   // Allow all buildings per default
   int i;
   m_allowed_buildings.resize(m_tribe.get_nrbuildings());
   for(i=0; i<m_tribe.get_nrbuildings(); i++)
      m_allowed_buildings[i]=true;

}

Player::~Player() {
	delete[] m_fields;
}
/*
===============
Player::init

Prepare the player for in-game action
===============
*/
void Player::init(const bool place_headquarters) {
	const Map & map = egbase().map();
	if (place_headquarters) {
		const Tribe_Descr & trdesc = m_tribe;
		Player_Area<Area<FCoords> > starting_area
			(m_plnum,
			 Area<FCoords>(map.get_fcoords(map.get_starting_pos(m_plnum)), 0));
		//try {
			Warehouse & headquarter = dynamic_cast<Warehouse &>
				(*egbase().warp_building
				 (starting_area,
				  starting_area.player_number,
				  trdesc.get_building_index("headquarters")));
			starting_area.radius = headquarter.get_conquers();
			egbase().conquer_area(starting_area);
			trdesc.load_warehouse_with_start_wares(egbase(), headquarter);
		//} catch (const Descr_Maintainer<Building_Descr>::Nonexistent) {
			//throw wexception("Tribe %s lacks headquarters", tribe.get_name());
		//}
	}
}


/**
 * Allocate the fields array that contains player-specific field information.
 */
void Player::allocate_map()
{
	const Map & map = egbase().map();
	log("Player::init(&map=%p)\n", &map);
	assert(map.get_width ());
	assert(map.get_height());
	m_fields = new Field[map.max_index()];
	log("Player::allocate_map: %p\n", m_fields);
}


/*
===============
Player::get_buildcaps

Return filtered buildcaps that take the player's territory into account.
===============
*/
FieldCaps Player::get_buildcaps(const FCoords fc) const {
	const Map & map = egbase().map();
	uchar buildcaps = fc.field->get_caps();
	const uchar player_number = m_plnum;

	if (not fc.field->is_interior(player_number)) buildcaps = 0;

	// Check if a building's flag can't be build due to ownership
	else if (buildcaps & BUILDCAPS_BUILDINGMASK) {
		FCoords flagcoords;
		map.get_brn(fc, &flagcoords);
		if (not flagcoords.field->is_interior(player_number))
			buildcaps &= ~BUILDCAPS_BUILDINGMASK;

		//  Prevent big buildings that would swell over borders.
		if ((buildcaps & BUILDCAPS_BIG) == BUILDCAPS_BIG
		    and
		    (not map.tr_n(fc).field->is_interior(player_number)
		     or
		     not map.tl_n(fc).field->is_interior(player_number)
		     or
		     not map. l_n(fc).field->is_interior(player_number)))
			buildcaps &= ~BUILDCAPS_SMALL;
	}

	return static_cast<const FieldCaps>(buildcaps);
}


/*
===============
Player::build_flag

Build a flag, checking that it's legal to do so.
===============
*/
void Player::build_flag(Coords c)
{
	int buildcaps = get_buildcaps(egbase().map().get_fcoords(c));

	if (buildcaps & BUILDCAPS_FLAG) Flag::create(&m_egbase, this, c);
}


/*
===============
Player::build_road

Build a road along the given path.
Perform sanity checks (ownership, flags).

Note: the diagnostic log messages aren't exactly errors. They might happen
in some situations over the network.
===============
*/
void Player::build_road(const Path & path) {
	Map & map = egbase().map();
	Flag *start, *end;

	BaseImmovable * imm = map.get_immovable(path.get_start());
	if (!imm || imm->get_type() != Map_Object::FLAG) {
		log("%i: building road, missed start flag\n", get_player_number());
		return;
	}
	start = (Flag *)imm;

	imm = map.get_immovable(path.get_end());
	if (!imm || imm->get_type() != Map_Object::FLAG) {
		log("%i: building road, missed end flag\n", get_player_number());
		return;
	}
	end = (Flag *)imm;

	// Verify ownership of the path
	FCoords coords = egbase().map().get_fcoords(path.get_start());

	const int laststep = path.get_nsteps() - 1;
	for (int i = 0; i < laststep; ++i) {
		const Direction dir = path[i];
		coords = map.get_neighbour(coords, dir);

		imm = map.get_immovable(coords);
		if (imm && imm->get_size() >= BaseImmovable::SMALL) {
			log("%i: building road, small immovable in the way, type=%d\n", get_player_number(), imm->get_type());
			return;
		}
		int caps = get_buildcaps(coords);
		if (!(caps & MOVECAPS_WALK)) {
			log("%i: building road, unwalkable\n", get_player_number());
			return;
		}
	}

	// fine, we can build the road
	Road::create(&m_egbase, Road_Normal, start, end, path);
}


/*
===============
Player::build

Place a construction site, checking that it's legal to do so.
===============
*/
void Player::build(Coords c, int idx)
{
	int buildcaps;
	Building_Descr* descr;

	// Validate building type
	if (idx < 0 || idx >= tribe().get_nrbuildings())
		return;
	descr = tribe().get_building_descr(idx);

	if (!descr->get_buildable())
		return;


	// Validate build position
	const Map & map = egbase().map();
	map.normalize_coords(&c);
	buildcaps = get_buildcaps(map.get_fcoords(c));

	if (descr->get_ismine())
		{
		if (!(buildcaps & BUILDCAPS_MINE))
			return;
		}
	else
		{
		if ((buildcaps & BUILDCAPS_SIZEMASK) < (descr->get_size() - BaseImmovable::SMALL + 1))
			return;
		}

	egbase().warp_constructionsite(c, m_plnum, idx);
}


/*
===============
Player::bulldoze

Bulldoze the given road, flag or building.
===============
*/
void Player::bulldoze(PlayerImmovable* imm)
{
	Building* building;

	// General security check
	if (imm->get_owner() != this)
		return;

	// Extended security check
	switch(imm->get_type()) {
	case Map_Object::BUILDING:
		building = (Building*)imm;
		if (!(building->get_playercaps() & (1 << Building::PCap_Bulldoze)))
			return;
		break;

	case Map_Object::FLAG:
		building = ((Flag*)imm)->get_building();
		if (building && !(building->get_playercaps() & (1 << Building::PCap_Bulldoze))) {
			log("Player trying to rip flag (%u) with undestroyable building (%u)\n", imm->get_serial(),
					building->get_serial());
			return;
		}
		break;

	case Map_Object::ROAD:
		break; // no additional check

	default:
		throw wexception("Player::bulldoze(%u): bad immovable type %u", imm->get_serial(), imm->get_type());
	}

	// Now destroy it
	imm->destroy(&egbase());
}

void Player::start_stop_building(PlayerImmovable* imm) {
	if (imm->get_owner() != this)
		return;
	if (imm->get_type() == Map_Object::BUILDING) {
		Building *bld = (Building*)imm;
		bld->set_stop(!bld->get_stop());
	}
}

/*
 * enhance this building, remove it, but give the constructionsite
 * an idea of enhancing
 */
void Player::enhance_building
(Building * building, const Building_Descr::Index index_of_new_building)
{
	if (building->get_owner() == this) {
		const Building_Descr::Index index_of_old_building =
			tribe().get_building_index(building->name().c_str());
		const Coords position = building->get_position();

      // Get workers and soldiers
		//  Make copies of the vectors, because the originals are destroyed with
		//  The building.
		const std::vector<Worker  *> workers  = building->get_workers();
		const std::vector<Soldier *> soldiers = building->has_soldiers() ?
			dynamic_cast<ProductionSite &>(*building).get_soldiers()
			:
			std::vector<Soldier *>();

		building->remove(&egbase()); //  no fire or stuff
		//  Hereafter the old building does not exist and building is a dangling
		//  pointer.
		building = egbase().warp_constructionsite
			(position, m_plnum, index_of_new_building, index_of_old_building);
		//  Hereafter building points to the new building.

		Game & game = dynamic_cast<Game &>(egbase());

		// Reassign the workers and soldiers.
		// Note that this will make sure they stay within the economy;
		// However, they are no longer associated with the building as
		// workers of that buiding, which is why they will leave for a
		// warehouse.
		const std::vector<Worker *>::const_iterator workers_end = workers.end();
		for
			(std::vector<Worker *>::const_iterator it = workers.begin();
			 it != workers_end;
			 ++it)
		{
			Worker & worker = **it;
			worker.set_location(building);
		}

		// Reassign the soldier
		const std::vector<Soldier *>::const_iterator soldiers_end =
			soldiers.end();
		for
			(std::vector<Soldier *>::const_iterator it = soldiers.begin();
			 it != soldiers_end;
			 ++it)
		{
			Soldier & soldier = **it;
			soldier.set_location(building);
		}
   }
}


/*
===============
Player::flagaction

Perform an action on the given flag.
===============
*/
void Player::flagaction(Flag* flag, int action)
{
	if (Game * const game = dynamic_cast<Game * const>(&egbase()))
		if (flag->get_owner() == this) {// Additional security check.
		switch (action) {
		case FLAGACTION_GEOLOGIST:
			//try {
				flag->add_flag_job
					(game, tribe().get_worker_index("geologist"), "expedition");
			/*} catch (Descr_Maintainer<Worker_Descr>::Nonexistent) {
				log("Tribe defines no geologist\n");
			}*/
			break;
		default:
			log("Player sent bad flagaction = %i\n", action);
		}
	}
}

/*
 * allow building
 *
 * Disable or enable a building for a player
 */
void Player::allow_building(int i, bool t) {
	assert(i < m_tribe.get_nrbuildings());
	m_allowed_buildings.resize(m_tribe.get_nrbuildings());

   m_allowed_buildings[i]=t;
}

/*
 * Economy stuff below
 */
void Player::add_economy(Economy* eco) {
   if(has_economy(eco)) return;
   m_economies.push_back(eco);
}

void Player::remove_economy(Economy* eco) {
   if(!has_economy(eco)) return;
   std::vector<Economy*>::iterator i = m_economies.begin();
   while(i!=m_economies.end()) {
      if(*i == eco) {
         m_economies.erase(i);
         return;
      }
      ++i;
   }
   assert(0); // Never here
}

bool Player::has_economy(Economy* eco) {
   std::vector<Economy*>::iterator  i = m_economies.begin();
   while(i!=m_economies.end()) {
      if( *i == eco) return true;
      ++i;
   }
   return false;
}

int Player::get_economy_number(Economy* eco) {
   assert(has_economy(eco));

   std::vector<Economy*>::iterator  i = m_economies.begin();
   while(i!=m_economies.end()) {
      if( *i == eco) return (i - m_economies.begin());
      ++i;
   }
   assert(0); // never here
   return 0;
}

/************  Military stuff  **********/

/*
==========
Player::change_training_options

Change the training priotity values
==========
*/
void Player::change_training_options(PlayerImmovable* imm, int atr, int val) {
    if (imm->get_owner() != this)
        return;
    if (imm->get_type() == Map_Object::BUILDING) {
        TrainingSite* ts=static_cast<TrainingSite*>(imm);
        if (val>0)
            ts->add_pri((enum tAttribute) atr);
        else
            ts->sub_pri((enum tAttribute) atr);
    }
}

/*
===========
Player::drop_soldier

Forces the drop of given soldier at given house
===========
*/
void Player::drop_soldier(PlayerImmovable* imm, Soldier* soldier) {
    if (imm->get_owner() != this)
        return;
    if ((soldier->get_worker_type() == Worker_Descr::SOLDIER) &&
        (imm->get_type() >= Map_Object::BUILDING)) {
            Building* ms= static_cast<Building*>(imm);
            ms->drop_soldier (soldier->get_serial());
    }
}

//TODO val might (theoretically) be >1 or <-1, but there's always an inc/dec by one
void Player::change_soldier_capacity (PlayerImmovable* imm, int val) {
	if (imm->get_owner() != this)
		return;
	if (imm->get_type() == Map_Object::BUILDING) {
		//Building* ts=static_cast<TrainingSite*>(imm);
		if (val>0)
			((Building*) imm)->soldier_capacity_up();
		else
			((Building*)imm)->soldier_capacity_down();
	}
}

/*
===============
Player::enemyflagaction

Perform an action on the given enemy flag.
===============
*/
void Player::enemyflagaction(Flag* flag, int action, int attacker, int num, int)
{
   if (attacker != get_player_number())
      throw wexception ("Player (%d) is not the sender of an attack (%d)", attacker, get_player_number());

	if (Game * const game = dynamic_cast<Game * const>(&egbase())) {

   assert (num >= 0);

log("++Player::EnemyFlagAction()\n");
   // Additional security check LOOK, if equal exit!!
   if (flag->get_owner() == this)
      return;
log("--Player::EnemyFlagAction() Checkpoint!\n");

   switch(action) {

      case ENEMYFLAGACTION_ATTACK:
         {
            game->create_attack_controller(flag,attacker,flag->get_owner()->get_player_number(),(uint)num);
            break;
         }

      default:
         log("Player sent bad enemyflagaction = %i\n", action);
		}
   }
}


inline void Player::discover_node
(const Map     & map,
 const ::Field & first_map_field,
 const FCoords   f,
 Field         & field)
throw ()
{
	assert(0 <= f.x);
	assert(f.x < map.get_width());
	assert(0 <= f.y);
	assert(f.y < map.get_height());
	assert(&map[0] <= f.field);
	assert           (f.field < &map[0] + map.max_index());
	assert(m_fields <= &field);
	assert            (&field < m_fields + map.max_index());
	assert(field.vision <= 1);

	{// discover everything (above the ground) in this field
		field.terrains = f.field->get_terrains();
		field.roads    = f.field->get_roads   ();
		field.owner    = f.field->get_owned_by();
		{//  map_object_descr[TCoords::None]

			const Map_Object_Descr * map_object_descr;
			if (const BaseImmovable * base_immovable = f.field->get_immovable()) {
				map_object_descr = &base_immovable->descr();
				if (map_object_descr == &g_road_descr) map_object_descr = 0;
				else if
					(const Building * const building =
					 dynamic_cast<const Building * const>(base_immovable))
					if (building->get_position() != f)
						//  TODO This is not the buildidng's main position so we can
						//  TODO not see it. But it should be possible to see it from
						//  TODO a distance somehow.
						map_object_descr = 0;
			} else map_object_descr = 0;
			field.map_object_descr[TCoords<>::None] = map_object_descr;
		}
	}
	{//  discover the D triangle and the SW edge of the top right neighbour field
		FCoords tr = map.tr_n(f);
		Field & tr_field = m_fields[tr.field - &first_map_field];
		if (tr_field.vision <= 1) {
			tr_field.terrains.d = tr.field->terrain_d();
			tr_field.roads &= ~(Road_Mask << Road_SouthWest);
			tr_field.roads |= Road_Mask << Road_SouthWest & tr.field->get_roads();
		}
	}
	{//  discover both triangles and the SE edge of the top left  neighbour field
		FCoords tl = map.tl_n(f);
		Field & tl_field = m_fields[tl.field - &first_map_field];
		if (tl_field.vision <= 1) {
			tl_field.terrains = tl.field->get_terrains();
			tl_field.roads &= ~(Road_Mask << Road_SouthEast);
			tl_field.roads |= Road_Mask << Road_SouthEast & tl.field->get_roads();
		}
	}
	{//  discover the R triangle and the  E edge of the     left  neighbour field
		FCoords l = map.l_n(f);
		Field & l_field = m_fields[l.field - &first_map_field];
		if (l_field.vision <= 1) {
			l_field.terrains.r = l.field->terrain_r();
			l_field.roads &= ~(Road_Mask << Road_East);
			l_field.roads |= Road_Mask << Road_East & l.field->get_roads();
		}
	}
}

void Player::see_node
(const Map                  & map,
 const ::Field              & first_map_field,
 const FCoords                f,
 const Editor_Game_Base::Time gametime,
 const bool                   lasting)
throw ()
{
	assert(0 <= f.x);
	assert(f.x < map.get_width());
	assert(0 <= f.y);
	assert(f.y < map.get_height());
	assert(&map[0] <= f.field);
	assert           (f.field < &first_map_field + map.max_index());

	Field & field = m_fields[f.field - &first_map_field];
	assert(m_fields <= &field);
	assert            (&field < m_fields + map.max_index());
	Vision vision = field.vision;
	if (vision == 0) vision = 1;
	if (vision == 1) {
		if (not lasting) field.time_node_last_unseen = gametime;
		discover_node(map, first_map_field, f, field);
	}
	vision += lasting;
	field.vision = vision;
}
