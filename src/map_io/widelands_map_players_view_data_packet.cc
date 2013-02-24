/*
 * Copyright (C) 2007-2008, 2010-2013 by the Widelands Development Team
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

#include "widelands_map_players_view_data_packet.h"

#include "io/bitinbuffer.h"
#include "io/bitoutbuffer.h"
#include "logic/editor_game_base.h"
#include "logic/field.h"
#include "logic/game_data_error.h"
#include "logic/instances.h" //for g_flag_descr
#include "logic/player.h"
#include "logic/tribe.h"
#include "logic/widelands_streamread_inlines.h"
#include "logic/widelands_streamwrite_inlines.h"

#include "economy/road.h"

#include "log.h"

#include "upcast.h"

namespace Widelands {


#define PLAYERDIRNAME_TEMPLATE "player/%u"
#define DIRNAME_TEMPLATE PLAYERDIRNAME_TEMPLATE                     "/view"

#define UNSEEN_TIMES_CURRENT_PACKET_VERSION             1
#define UNSEEN_TIMES_FILENAME_TEMPLATE DIRNAME_TEMPLATE             "/unseen_times_%u"

#define NODE_IMMOVABLE_KINDS_CURRENT_PACKET_VERSION     1
#define NODE_IMMOVABLE_KINDS_FILENAME_TEMPLATE DIRNAME_TEMPLATE     "/node_immovable_kinds_%u"

#define NODE_IMMOVABLES_CURRENT_PACKET_VERSION          2
#define NODE_IMMOVABLES_FILENAME_TEMPLATE DIRNAME_TEMPLATE          "/node_immovables_%u"

#define ROADS_CURRENT_PACKET_VERSION                    1
#define ROADS_FILENAME_TEMPLATE DIRNAME_TEMPLATE                    "/roads_%u"

#define TERRAINS_CURRENT_PACKET_VERSION                 1
#define TERRAINS_FILENAME_TEMPLATE DIRNAME_TEMPLATE                 "/terrains_%u"

#define TRIANGLE_IMMOVABLE_KINDS_CURRENT_PACKET_VERSION 1
#define TRIANGLE_IMMOVABLE_KINDS_FILENAME_TEMPLATE DIRNAME_TEMPLATE "/triangle_immovable_kinds_%u"

#define TRIANGLE_IMMOVABLES_CURRENT_PACKET_VERSION      2
#define TRIANGLE_IMMOVABLES_FILENAME_TEMPLATE DIRNAME_TEMPLATE      "/triangle_immovables_%u"

#define OWNERS_CURRENT_PACKET_VERSION                   0
#define OWNERS_FILENAME_TEMPLATE DIRNAME_TEMPLATE                   "/owners_%u"

#define SURVEYS_CURRENT_PACKET_VERSION                  1
#define SURVEYS_FILENAME_TEMPLATE DIRNAME_TEMPLATE                  "/surveys_%u"

#define SURVEY_AMOUNTS_CURRENT_PACKET_VERSION           1
#define SURVEY_AMOUNTS_FILENAME_TEMPLATE DIRNAME_TEMPLATE           "/survey_amounts_%u"

#define SURVEY_TIMES_CURRENT_PACKET_VERSION             1
#define SURVEY_TIMES_FILENAME_TEMPLATE DIRNAME_TEMPLATE             "/survey_times_%u"

#define VISION_CURRENT_PACKET_VERSION                   1
#define VISION_FILENAME_TEMPLATE DIRNAME_TEMPLATE                   "/vision_%u"

#define BORDER_CURRENT_PACKET_VERSION                   0
#define BORDER_FILENAME_TEMPLATE DIRNAME_TEMPLATE                   "/border_%u"

#define FILENAME_SIZE 48

//  The map is traversed by row and column. In each step we process of one map
//  field (which is 1 node, 2 triangles and 3 edges that are stored together).
//  For this processing we need to keep track of 4 nodes:
//  *  f: the node of the processed field
//  * bl: the bottom left neighbour of f
//  * br: the bottom right neighbour of f
//  *  r: the right neighbour of f
//
//  The layout of the processing region is as follows:
//
//                     f------ r
//                    / \     /
//                   /  \    /
//                  / D \ R /
//                 /    \  /
//                /     \ /
//              bl------br

struct Map_Object_Data {
	Map_Object_Data() : map_object_descr(0), csi(0) {}
	const Map_Object_Descr                     * map_object_descr;
	const Player::Constructionsite_Information * csi;
};

inline static Map_Object_Data * read_unseen_immovable
	(const Editor_Game_Base & egbase,
	 BitInBuffer<2>         & immovable_kinds_file,
	 FileRead               & immovables_file,
	 uint8_t                & version
	)
{
	Map_Object_Data * m = new Map_Object_Data;
	try {
		switch (immovable_kinds_file.get()) {
		case 0:  //  The player sees no immovable.
			m->map_object_descr = 0;                                       break;
		case 1: //  The player sees a tribe or world immovable.
			m->map_object_descr = &immovables_file.Immovable_Type(egbase); break;
		case 2:  //  The player sees a flag.
			m->map_object_descr = &g_flag_descr;                           break;
		case 3: //  The player sees a building.
			m->map_object_descr = &immovables_file.Building_Type (egbase);
			if (version > 1) {
				// Read data from immovables file
				if (immovables_file.Unsigned8() == 1) { // the building is a constructionsite
					Player::Constructionsite_Information * csi = new Player::Constructionsite_Information;
					csi->becomes       = &immovables_file.Building_Type(egbase);
					if (immovables_file.Unsigned8() == 1)
						csi->was        = &immovables_file.Building_Type(egbase);
					csi->totaltime     =  immovables_file.Unsigned32();
					csi->completedtime =  immovables_file.Unsigned32();
					m->csi = csi;
				}
			}
			break;
		}
	} catch (const _wexception & e) {
		throw game_data_error(_("unseen immovable: %s"), e.what());
	}
	return m;
}

#define OPEN_INPUT_FILE(filetype, file, filename, filename_template, version) \
   char (filename)[FILENAME_SIZE];                                            \
   snprintf(filename, sizeof(filename), filename_template, plnum, version);   \
   filetype file;                                                             \
   try {(file).Open(fs, filename);}                                           \
   catch (const File_error &) {                                               \
      throw game_data_error                                                   \
         ("Map_Players_View_Data_Packet::Read: player %u:Could not open "     \
          "\"%s\" for reading. This file should exist when \"%s\" exists",    \
          plnum, filename, unseen_times_filename);                            \
   }

// Try to find the file with newest fitting version number
#define OPEN_INPUT_FILE_NEW_VERSION(filetype, file, filename, fileversion, filename_template, version) \
	uint8_t fileversion = version;                                                                      \
	filetype file;                                                                                      \
	char (filename)[FILENAME_SIZE];                                                                     \
	for (; fileversion > 0; --fileversion) {                                                            \
		snprintf(filename, sizeof(filename), filename_template, plnum, fileversion);                         \
		try {(file).Open(fs, filename); break;}                                                          \
		catch (...) {                                                                     \
			if (fileversion == 1)                                                                         \
				throw game_data_error                                                                      \
					("Map_Players_View_Data_Packet::Read: player %u:Could not open "                        \
					 "\"%s\" for reading. This file should exist when \"%s\" exists",                       \
					 plnum, filename, unseen_times_filename);                                               \
		}                                                                                                \
	}

#define CHECK_TRAILING_BYTES(file, filename)                                  \
   if (not (file).EndOfFile())                                                \
      throw game_data_error                                                   \
         ("Map_Players_View_Data_Packet::Read: player %u:"                    \
          "Found %lu trailing bytes in \"%s\"",                               \
          plnum,                                                              \
          static_cast<long unsigned int>((file).GetSize() - (file).GetPos()), \
          filename);                                                          \

void Map_Players_View_Data_Packet::Read
	(FileSystem            &       fs,
	 Editor_Game_Base      &       egbase,
	 bool                    const skip,
	 Map_Map_Object_Loader &)
	throw (_wexception)
{
	if (skip)
		return;

	const Map & map = egbase.map();
	const X_Coordinate mapwidth  = map.get_width ();
	const Y_Coordinate mapheight = map.get_height();
	Field & first_field = map[0];
	const Player_Number nr_players = map.get_nrplayers();
	iterate_players_existing_const(plnum, nr_players, egbase, player) {
		Player::Field * const player_fields = player->m_fields;
		uint32_t const gametime = egbase.get_gametime();

		char unseen_times_filename[FILENAME_SIZE];
		snprintf
			(unseen_times_filename, sizeof(unseen_times_filename),
			 UNSEEN_TIMES_FILENAME_TEMPLATE,
			 plnum, UNSEEN_TIMES_CURRENT_PACKET_VERSION);
		FileRead unseen_times_file;
		struct Not_Found {};

		if (!unseen_times_file.TryOpen(fs, unseen_times_filename)) {
			log
				("Map_Players_View_Data_Packet::Read: WARNING: Could not open "
				 "\"%s\" for reading. Assuming that the game is from an old "
				 "version without player point of view. Will give player %u "
				 "knowledge of unseen nodes, edges and triangles (but not "
				 "resources).",
				 unseen_times_filename, plnum);

			for
				(FCoords first_in_row(Coords(0, 0), &first_field);
				 first_in_row.y < mapheight;
				 ++first_in_row.y, first_in_row.field += mapwidth)
			{
				FCoords  r = first_in_row, br = map.bl_n(r);
				Map_Index  r_index =  r.field - &first_field;
				Map_Index br_index = br.field - &first_field;
				Player::Field *  r_player_field = player_fields +  r_index;
				Player::Field * br_player_field = player_fields + br_index;
				Vision  r_vision =  r_player_field->vision;
				Vision br_vision = br_player_field->vision;
				do {
					const FCoords f = r;
					Player::Field & f_player_field = *r_player_field;
					const Vision    f_vision = r_vision, bl_vision = br_vision;
					move_r(mapwidth, r, r_index); move_r(mapwidth, br, br_index);
					r_player_field  = player_fields +  r_index;
					br_player_field = player_fields + br_index;
					r_vision  =  r_player_field->vision;
					br_vision = br_player_field->vision;

					f_player_field.time_node_last_unseen = gametime;

					if (f_vision) { //  node
						//  owner
						f_player_field.owner = f.field->get_owned_by();
						assert(f_player_field.owner < 0x20);

						//  map_object_descr
						const Map_Object_Descr * map_object_descr;
						if (const BaseImmovable * base_immovable = f.field->get_immovable()) {
							map_object_descr = &base_immovable->descr();
							if (Road::IsRoadDescr(map_object_descr))
								map_object_descr = 0;
							else if (upcast(Building const, building, base_immovable))
								if (building->get_position() != f)
									//  TODO This is not the building's main position
									//  TODO so we can not see it. But it should be
									//  TODO possible to see it from a distance somehow.
									map_object_descr = 0;
						} else map_object_descr = 0;
						f_player_field.map_object_descr[TCoords<>::None] = map_object_descr;
					}

					{ //  triangles
						//  Must be initialized because the rendering code is
						//  accessing it even for triangles that the player does not
						//  see (it is the darkening that actually hides the ground
						//  from the player). This is important for worlds where the
						//  number of terrain types is not maximal (16), so that an
						//  uninitialized terrain index could cause a not found error
						//  in Descr_Maintainer<Terrain_Descr>::get(Terrain_Index).
						Field::Terrains terrains; terrains.d = terrains.r = 0;

						if (f_vision | bl_vision | br_vision)
							terrains.d = f.field->terrain_d();
						if (f_vision | br_vision |  r_vision)
							terrains.r = f.field->terrain_r();
						f_player_field.terrains = terrains;
					}

					{ //  edges
						uint8_t mask = 0;
						if (f_vision | bl_vision)
							mask  = Road_Mask << Road_SouthWest;
						if (f_vision | br_vision)
							mask |= Road_Mask << Road_SouthEast;
						if (f_vision |  r_vision)
							mask |= Road_Mask << Road_East;
						f_player_field.roads = f.field->get_roads() & mask;
					}

					//  The player is not given information about resources that he
					//  has discovered, because there is no such information in old
					//  savegames, except for the resource indicators that are
					//  merely immovables with limited lifetime.
				} while (r.x);
			}
			return;
		}

		// Verify the vision values
		FileRead vision_file;
		bool have_vision = false;

		try {
			char fname[FILENAME_SIZE];
			snprintf
				(fname, sizeof(fname),
				 VISION_FILENAME_TEMPLATE, plnum, VISION_CURRENT_PACKET_VERSION);
			vision_file.Open(fs, fname);
			have_vision = true;
		} catch (...) {}

		if (have_vision) {
			for
				(FCoords first_in_row(Coords(0, 0), &first_field);
				 first_in_row.y < mapheight;
				 ++first_in_row.y, first_in_row.field += mapwidth)
			{
				FCoords r = first_in_row;
				Map_Index r_index = r.field - &first_field;
				Player::Field * r_player_field = player_fields + r_index;
				do {
					Player::Field & f_player_field = *r_player_field;
					move_r(mapwidth, r, r_index);
					r_player_field  = player_fields + r_index;

					uint32_t file_vision = vision_file.Unsigned32();

					// There used to be a check here that the calculated, and the
					// loaded vision were the same. I removed this check, because
					// scripting could have given the player a permanent view of
					// this field. That's why we save this stuff in the first place!
					if (file_vision != f_player_field.vision)
						f_player_field.vision = file_vision;
				} while (r.x);
			}

			log("Vision check successful for player %u\n", plnum);
		}

		// Read the player's knowledge about all fields
		OPEN_INPUT_FILE
			(BitInBuffer<2>, node_immovable_kinds_file,
			 node_immovable_kinds_filename,
			 NODE_IMMOVABLE_KINDS_FILENAME_TEMPLATE,
			 NODE_IMMOVABLE_KINDS_CURRENT_PACKET_VERSION);

		OPEN_INPUT_FILE_NEW_VERSION
			(FileRead,       node_immovables_file,
			 node_immovables_filename, node_immovables_file_version,
			 NODE_IMMOVABLES_FILENAME_TEMPLATE,
			 NODE_IMMOVABLES_CURRENT_PACKET_VERSION);

		OPEN_INPUT_FILE
			(BitInBuffer<2>, roads_file,          roads_filename,
			 ROADS_FILENAME_TEMPLATE,        ROADS_CURRENT_PACKET_VERSION);

		OPEN_INPUT_FILE
			(BitInBuffer<4>, terrains_file,       terrains_filename,
			 TERRAINS_FILENAME_TEMPLATE,     TERRAINS_CURRENT_PACKET_VERSION);

		OPEN_INPUT_FILE
			(BitInBuffer<2>, triangle_immovable_kinds_file,
			 triangle_immovable_kinds_filename,
			 TRIANGLE_IMMOVABLE_KINDS_FILENAME_TEMPLATE,
			 TRIANGLE_IMMOVABLE_KINDS_CURRENT_PACKET_VERSION);

		OPEN_INPUT_FILE_NEW_VERSION
			(FileRead,       triangle_immovables_file,
			 triangle_immovables_filename, triangle_immovables_file_version,
			 TRIANGLE_IMMOVABLES_FILENAME_TEMPLATE,
			 TRIANGLE_IMMOVABLES_CURRENT_PACKET_VERSION);

		OPEN_INPUT_FILE
			(FileRead,       owners_file,         owners_filename,
			 OWNERS_FILENAME_TEMPLATE,       OWNERS_CURRENT_PACKET_VERSION);

		OPEN_INPUT_FILE
			(BitInBuffer<1>, surveys_file,        surveys_filename,
			 SURVEYS_FILENAME_TEMPLATE,      SURVEYS_CURRENT_PACKET_VERSION);

		OPEN_INPUT_FILE
			(BitInBuffer<4>, survey_amounts_file, survey_amounts_filename,
			 SURVEY_AMOUNTS_FILENAME_TEMPLATE,
			 SURVEY_AMOUNTS_CURRENT_PACKET_VERSION);

		OPEN_INPUT_FILE
			(FileRead,       survey_times_file,   survey_times_filename,
			 SURVEY_TIMES_FILENAME_TEMPLATE, SURVEY_TIMES_CURRENT_PACKET_VERSION);

		// Load border files
		char border_filename[FILENAME_SIZE];
		snprintf
			(border_filename, sizeof(border_filename),
			 BORDER_FILENAME_TEMPLATE, plnum, BORDER_CURRENT_PACKET_VERSION);
		BitInBuffer<1> border_file;
		bool borders = false;
		try {
			border_file.Open(fs, border_filename);
			borders = true;
		} catch (...) {
			log
				("Map_Players_View_Data_Packet::Read: No border file found, therefore no borders will be drawn "
				 "in unseen areas until they get at least once seen again.\n");
		}

		for
			(FCoords first_in_row(Coords(0, 0), &first_field);
			 first_in_row.y < mapheight;
			 ++first_in_row.y, first_in_row.field += mapwidth)
		{
			FCoords  r = first_in_row, br = map.bl_n(r);
			Map_Index  r_index =  r.field - &first_field;
			Map_Index br_index = br.field - &first_field;
			Player::Field *  r_player_field = player_fields +  r_index;
			Player::Field * br_player_field = player_fields + br_index;
			Vision  r_vision =  r_player_field->vision;
			Vision br_vision = br_player_field->vision;
			bool  r_everseen =  r_vision,  r_seen = 1 <  r_vision;
			bool br_everseen = br_vision, br_seen = 1 < br_vision;
			do {
				const FCoords f = r;
				Player::Field &  f_player_field =  *r_player_field;
				const Vision    f_vision = r_vision;
				const bool      f_everseen = r_everseen, bl_everseen = br_everseen;
				const bool      f_seen     = r_seen,     bl_seen     = br_seen;
				move_r(mapwidth, r, r_index); move_r(mapwidth, br, br_index);
				r_player_field  = player_fields +  r_index;
				br_player_field = player_fields + br_index;
				r_vision  =  r_player_field->vision;
				br_vision = br_player_field->vision;
				r_everseen  =  r_vision,  r_seen = 1 <  r_vision;
				br_everseen = br_vision, br_seen = 1 < br_vision;

				//  Store the player's view of roads and ownership in these
				//  temporary variables and save it in the player when set.
				uint8_t         roads = 0;
				Player_Number owner = 0;

				switch (f_vision) { //  owner and map_object_descr
				case 0:
					//  The player has never seen this node, so he has no
					//  information about it. Neither should he be informed about
					//  it now.
					break;
				case 1: {
					//  The player has seen the node but does not see it now. Load
					//  his information about the node from file.
					try {
						f_player_field.time_node_last_unseen = unseen_times_file.Unsigned32();
					} catch (const FileRead::File_Boundary_Exceeded & e) {
						throw game_data_error
							("Map_Players_View_Data_Packet::Read: player %u: in "
							 "\"%s\":%lu: node (%i, %i): unexpected end of file "
							 "while reading time_node_last_unseen",
							 plnum,
							 unseen_times_filename,
							 static_cast<long unsigned int>(unseen_times_file.GetPos() - 4),
							 f.x, f.y);
					}

					try {owner = owners_file.Unsigned8();}
					catch (const FileRead::File_Boundary_Exceeded & e) {
						throw game_data_error
							("Map_Players_View_Data_Packet::Read: player %u: in "
							 "\"%s\":%lu: node (%i, %i): unexpected end of file "
							 "while reading owner",
							 plnum,
							 unseen_times_filename,
							 static_cast<long unsigned int>
							 	(unseen_times_file.GetPos() - 1),
							 f.x, f.y);
					}
					if (nr_players < owner)
						throw game_data_error
							("Map_Players_View_Data_Packet::Read: player %u: in "
							 "\"%s\":%lu & 0xf: node (%i, %i): Player thinks that "
							 "this node is owned by player %u, but there are only %u "
							 "players",
							 plnum, owners_filename,
							 static_cast<long unsigned int>(owners_file.GetPos() - 1),
							 f.x, f.y, owner, nr_players);

					Map_Object_Data * mod =
						read_unseen_immovable
							(egbase, node_immovable_kinds_file, node_immovables_file, node_immovables_file_version);
					f_player_field.map_object_descr[TCoords<>::None] = mod->map_object_descr;
					f_player_field.constructionsite[TCoords<>::None] = mod->csi;

					// if there is a border file, read in whether this field had a border the last time it was seen
					if (borders) {
						f_player_field.border    = (border_file.get() == 1);
						f_player_field.border_r  = (border_file.get() == 1);
						f_player_field.border_br = (border_file.get() == 1);
						f_player_field.border_bl = (border_file.get() == 1);
					}


					break;
				}
				default:
					//  The player currently sees the node. Therefore his
					//  information about the node has not been saved. Fill in the
					//  information from the game state.

					//  owner
					owner = f.field->get_owned_by();
					assert(owner <= nr_players);

					//  map_object_descr
					const Map_Object_Descr * map_object_descr;
					if (const BaseImmovable * base_immovable = f.field->get_immovable()) {
						map_object_descr = &base_immovable->descr();
						if (Road::IsRoadDescr(map_object_descr))
							map_object_descr = 0;
						else if (upcast(Building const, building, base_immovable))
							if (building->get_position() != f)
								//  TODO This is not the building's main position so
								//  TODO we can not see it. But it should be possible
								//  TODO to see it from a distance somehow.
								map_object_descr = 0;
					} else map_object_descr = 0;
					f_player_field.map_object_descr[TCoords<>::None] = map_object_descr;
					break;
				}

				//  triangles
				if  (f_seen | bl_seen | br_seen) {
					//  The player currently sees the D triangle. Therefore his
					//  information about the triangle has not been saved. Fill in
					//  the information from the game state.
					f_player_field.terrains.d = f.field->terrain_d();
					f_player_field.map_object_descr[TCoords<>::D] = 0;
				} else if (f_everseen | bl_everseen | br_everseen) {
					//  The player has seen the D triangle but does not see it now.
					//  Load his information about the triangle from file.
					try {f_player_field.terrains.d = terrains_file.get();}
					catch (const FileRead::File_Boundary_Exceeded & e) {
						throw game_data_error
							("Map_Players_View_Data_Packet::Read: player %u: in "
							 "\"%s\": node (%i, %i) t = D: unexpected end of file "
							 "while reading terrain",
							 plnum, terrains_filename, f.x, f.y);
					}
					Map_Object_Data * mod =
						read_unseen_immovable
							(egbase, triangle_immovable_kinds_file, triangle_immovables_file,
							 triangle_immovables_file_version);
					f_player_field.map_object_descr[TCoords<>::D] = mod->map_object_descr;
					f_player_field.constructionsite[TCoords<>::D] = mod->csi;

				}
				if  (f_seen | br_seen | r_seen) {
					//  The player currently sees the R triangle. Therefore his
					//  information about the triangle has not been saved. Fill in
					//  the information from the game state.
					f_player_field.terrains.r = f.field->terrain_r();
					f_player_field.map_object_descr[TCoords<>::R] = 0;
				} else if (f_everseen | br_everseen | r_everseen) {
					//  The player has seen the R triangle but does not see it now.
					//  Load his information about the triangle from file.
					try {f_player_field.terrains.r = terrains_file.get();}
					catch (const FileRead::File_Boundary_Exceeded & e) {
						throw game_data_error
							("Map_Players_View_Data_Packet::Read: player %u: in "
							 "\"%s\": node (%i, %i) t = R: unexpected end of file "
							 "while reading terrain",
							 plnum, terrains_filename, f.x, f.y);
					}
					Map_Object_Data * mod =
						read_unseen_immovable
							(egbase, triangle_immovable_kinds_file, triangle_immovables_file,
							 triangle_immovables_file_version);
					f_player_field.map_object_descr[TCoords<>::R] = mod->map_object_descr;
					f_player_field.constructionsite[TCoords<>::R] = mod->csi;
				}

				{ //  edges
					uint8_t mask = 0;
					if (f_seen | bl_seen)
						mask  = Road_Mask << Road_SouthWest;
					else if (f_everseen | bl_everseen)
						//  The player has seen the SouthWest edge but does not see
						//  it now. Load his information about this edge from file.
						try {roads  = roads_file.get() << Road_SouthWest;}
						catch (const FileRead::File_Boundary_Exceeded & e) {
							throw game_data_error
								("Map_Players_View_Data_Packet::Read: player %u: in "
								 "\"%s\": node (%i, %i): unexpected end of file while "
								 "reading Road_SouthWest",
								 plnum, roads_filename, f.x, f.y);
						}
					if (f_seen | br_seen)
						mask |= Road_Mask << Road_SouthEast;
					else if (f_everseen | br_everseen)
						//  The player has seen the SouthEast edge but does not see
						//  it now. Load his information about this edge from file.
						try {roads |= roads_file.get() << Road_SouthEast;}
						catch (const FileRead::File_Boundary_Exceeded & e) {
							throw game_data_error
								("Map_Players_View_Data_Packet::Read: player %u: in "
								 "\"%s\": node (%i, %i): unexpected end of file while "
								 "reading Road_SouthEast",
								 plnum, roads_filename, f.x, f.y);
						}
					if (f_seen |  r_seen)
						mask |= Road_Mask << Road_East;
					else if (f_everseen |  r_everseen)
						//  The player has seen the      East edge but does not see
						//  it now. Load his information about this edge from file.
						try {roads |= roads_file.get() << Road_East;}
						catch (const FileRead::File_Boundary_Exceeded & e) {
							throw game_data_error
								("Map_Players_View_Data_Packet::Read: player %u: in "
								 "\"%s\": node (%i, %i): unexpected end of file while "
								 "reading Road_East",
								 plnum, roads_filename, f.x, f.y);
						}
					roads |= f.field->get_roads() & mask;
				}

				//  Now save this information in the player field.
				f_player_field.roads = roads;
				f_player_field.owner = owner;

				//  geologic survey
				try {
					if
						((f_everseen & bl_everseen & br_everseen)
						 and
						 surveys_file.get())
					{
						try {
							f_player_field.resource_amounts.d =
								survey_amounts_file.get();
						} catch (const FileRead::File_Boundary_Exceeded & e) {
							throw game_data_error
								("Map_Players_View_Data_Packet::Read: player %u: in "
								 "\"%s\": node (%i, %i) t = D: unexpected end of file "
								 "while reading resource amount of surveyed triangle",
								 plnum, survey_amounts_filename, f.x, f.y);
						}
						try {
							f_player_field.time_triangle_last_surveyed[TCoords<>::D] =
								survey_times_file.Unsigned32();
						} catch (const FileRead::File_Boundary_Exceeded & e) {
							throw game_data_error
								("Map_Players_View_Data_Packet::Read: player %u: in "
								 "\"%s\":%lu: node (%i, %i) t = D: unexpected end of "
								 "file while reading time_triangle_last_surveyed",
								 plnum, survey_times_filename,
								 static_cast<long unsigned int>
								 	(survey_times_file.GetPos() - 4),
								 f.x, f.y);
						}
					}
				} catch (const FileRead::File_Boundary_Exceeded & e) {
					throw game_data_error
						("Map_Players_View_Data_Packet::Read: player %u: in \"%s\": "
						 "node (%i, %i) t = D: unexpected end of file while reading "
						 "survey bit",
						 plnum, surveys_filename, f.x, f.y);
				}
				try {
					if
						((f_everseen & br_everseen &  r_everseen)
						 and
						 surveys_file.get())
					{
						try {
							f_player_field.resource_amounts.r =
								survey_amounts_file.get();
						} catch (const FileRead::File_Boundary_Exceeded & e) {
							throw game_data_error
								("Map_Players_View_Data_Packet::Read: player %u: in "
								 "\"%s\": node (%i, %i) t = R: unexpected end of file "
								 "while reading resource amount of surveyed triangle",
								 plnum, survey_amounts_filename, f.x, f.y);
						}
						try {
							f_player_field.time_triangle_last_surveyed[TCoords<>::R] =
								survey_times_file.Unsigned32();
						} catch (const FileRead::File_Boundary_Exceeded & e) {
							throw game_data_error
								("Map_Players_View_Data_Packet::Read: player %u: in "
								 "\"%s\":%lu: node (%i, %i) t = R: unexpected end of "
								 "file while reading time_triangle_last_surveyed",
								 plnum, survey_times_filename,
								 static_cast<long unsigned int>
								 	(survey_times_file.GetPos() - 4),
								 f.x, f.y);
						}
					}
				} catch (const FileRead::File_Boundary_Exceeded & e) {
					throw game_data_error
						("Map_Players_View_Data_Packet::Read: player %u: in \"%s\": "
						 "node (%i, %i) t = R: unexpected end of file while reading "
						 "survey bit",
						 plnum, surveys_filename, f.x, f.y);
				}
			} while (r.x);
		}
		CHECK_TRAILING_BYTES(unseen_times_file,    unseen_times_filename);
		CHECK_TRAILING_BYTES
			(node_immovable_kinds_file,     node_immovable_kinds_filename);
		CHECK_TRAILING_BYTES(node_immovables_file, node_immovables_filename);
		CHECK_TRAILING_BYTES(roads_file,           roads_filename);
		CHECK_TRAILING_BYTES(terrains_file,        terrains_filename);
		CHECK_TRAILING_BYTES
			(triangle_immovable_kinds_file, triangle_immovable_kinds_filename);
		CHECK_TRAILING_BYTES
			(triangle_immovables_file,      triangle_immovables_filename);
		CHECK_TRAILING_BYTES(owners_file,          owners_filename);
		CHECK_TRAILING_BYTES(surveys_file,         surveys_filename);
		CHECK_TRAILING_BYTES(survey_amounts_file,  survey_amounts_filename);
		CHECK_TRAILING_BYTES(survey_times_file,    survey_times_filename);
	}
}


inline static void write_unseen_immovable
	(Map_Object_Data const * map_object_data,
	 BitOutBuffer<2> & immovable_kinds_file, FileWrite & immovables_file)
{
	Map_Object_Descr const * const map_object_descr = map_object_data->map_object_descr;
	Player::Constructionsite_Information const * const csi = map_object_data->csi;
	assert(not Road::IsRoadDescr(map_object_descr));
	uint8_t immovable_kind;

	if (not map_object_descr)
		immovable_kind = 0;
	else if (upcast(Immovable_Descr const, immovable_descr, map_object_descr)) {
		immovable_kind = 1;
		immovables_file.Immovable_Type(*immovable_descr);
	} else if (map_object_descr == &g_flag_descr)
		immovable_kind = 2;
	else if (upcast(Building_Descr const, building_descr, map_object_descr)) {
		immovable_kind = 3;
		immovables_file.Building_Type(*building_descr);
		if (!csi)
			immovables_file.Unsigned8(0);
		else {
			// the building is a constructionsite
			immovables_file.Unsigned8(1);
			immovables_file.Building_Type(*csi->becomes);
			if (!csi->was)
				immovables_file.Unsigned8(0);
			else {
				// constructionsite is an enhancement, therefor we write down the enhancement
				immovables_file.Unsigned8(1);
				immovables_file.Building_Type(*csi->was);
			}
			immovables_file.Unsigned32(csi->totaltime);
			immovables_file.Unsigned32(csi->completedtime);
		}
	} else assert(false);
	immovable_kinds_file.put(immovable_kind);
}

#define WRITE(file, filename_template, version)                               \
   snprintf(filename, sizeof(filename), filename_template, plnum, version);   \
   (file).Write(fs, filename);                                                \

void Map_Players_View_Data_Packet::Write
	(FileSystem & fs, Editor_Game_Base & egbase, Map_Map_Object_Saver &)
throw (_wexception)
{
	fs.EnsureDirectoryExists("player");
	const Map & map = egbase.map();
	const X_Coordinate mapwidth  = map.get_width ();
	const Y_Coordinate mapheight = map.get_height();
	// \todo make first_field const when FCoords has been templatized so it can
	// have "const Field * field;"
	Field & first_field = map[0];
	const Player_Number nr_players = map.get_nrplayers();
	iterate_players_existing_const(plnum, nr_players, egbase, player)
		if (const Player::Field * const player_fields = player->m_fields) {
			FileWrite                   unseen_times_file;
			BitOutBuffer<2>     node_immovable_kinds_file;
			FileWrite                node_immovables_file;
			BitOutBuffer<2>                    roads_file;
			BitOutBuffer<4>                 terrains_file;
			BitOutBuffer<2> triangle_immovable_kinds_file;
			FileWrite            triangle_immovables_file;
			FileWrite                         owners_file;
			BitOutBuffer<1>                  surveys_file;
			BitOutBuffer<4>           survey_amounts_file;
			FileWrite                   survey_times_file;
			FileWrite                         vision_file;
			BitOutBuffer<1>                   border_file;
			for
				(FCoords first_in_row(Coords(0, 0), &first_field);
				 first_in_row.y < mapheight;
				 ++first_in_row.y, first_in_row.field += mapwidth)
			{
				FCoords  r = first_in_row, br = map.bl_n(r);
				Map_Index  r_index =  r.field - &first_field;
				Map_Index br_index = br.field - &first_field;
				const Player::Field *  r_player_field = player_fields +  r_index;
				const Player::Field * br_player_field = player_fields + br_index;
				Vision  r_vision =  r_player_field->vision;
				Vision br_vision = br_player_field->vision;
				bool  r_everseen =  r_vision,  r_seen = 1 <  r_vision;
				bool br_everseen = br_vision, br_seen = 1 < br_vision;
				do {
					const Player::Field &  f_player_field =  *r_player_field;
					const bool f_everseen = r_everseen, bl_everseen = br_everseen;
					const bool f_seen     = r_seen,     bl_seen     = br_seen;
					move_r(mapwidth, r, r_index); move_r(mapwidth, br, br_index);
					r_player_field  = player_fields +  r_index;
					br_player_field = player_fields + br_index;
					r_vision  =  r_player_field->vision;
					br_vision = br_player_field->vision;
					r_everseen  =  r_vision,  r_seen = 1 <  r_vision;
					br_everseen = br_vision, br_seen = 1 < br_vision;

					vision_file.Unsigned32(f_player_field.vision);

					if (not f_seen) {

						if (f_everseen) { //  node
							unseen_times_file.Unsigned32
								(f_player_field.time_node_last_unseen);
							assert(f_player_field.owner < 0x20);
							owners_file.Unsigned8(f_player_field.owner);
							Map_Object_Data * mod = new Map_Object_Data;
							mod->map_object_descr = f_player_field.map_object_descr[TCoords<>::None];
							mod->csi              = f_player_field.constructionsite[TCoords<>::None];
							write_unseen_immovable(mod, node_immovable_kinds_file, node_immovables_file);

							// write whether this field had a border the last time it was seen
							border_file.put(f_player_field.border    ? 1 : 0);
							border_file.put(f_player_field.border_r  ? 1 : 0);
							border_file.put(f_player_field.border_br ? 1 : 0);
							border_file.put(f_player_field.border_bl ? 1 : 0);
						}

						//  triangles
						if
							//  the player does not see the D triangle now but has
							//  seen it
							(not bl_seen & not br_seen &
							 (f_everseen | bl_everseen | br_everseen))
						{
							terrains_file.put(f_player_field.terrains.d);
							Map_Object_Data * mod = new Map_Object_Data;
							mod->map_object_descr = f_player_field.map_object_descr[TCoords<>::D];
							mod->csi              = f_player_field.constructionsite[TCoords<>::D];
							write_unseen_immovable(mod, triangle_immovable_kinds_file, triangle_immovables_file);
						}
						if
							//  the player does not see the R triangle now but has
							//  seen it
							(not br_seen & not  r_seen &
							 (f_everseen | br_everseen |  r_everseen))
						{
							terrains_file.put(f_player_field.terrains.r);
							Map_Object_Data * mod = new Map_Object_Data;
							mod->map_object_descr = f_player_field.map_object_descr[TCoords<>::R];
							mod->csi              = f_player_field.constructionsite[TCoords<>::R];
							write_unseen_immovable(mod, triangle_immovable_kinds_file, triangle_immovables_file);
						}

						//  edges
						if (not bl_seen & (f_everseen | bl_everseen))
							roads_file.put(f_player_field.road_sw());
						if (not br_seen & (f_everseen | br_everseen))
							roads_file.put(f_player_field.road_se());
						if (not  r_seen & (f_everseen |  r_everseen))
							roads_file.put(f_player_field.road_e ());
					}

					//  geologic survey
					if (f_everseen & bl_everseen & br_everseen) {
						const uint32_t time_last_surveyed =
							f_player_field.time_triangle_last_surveyed[TCoords<>::D];
						const uint8_t has_info = time_last_surveyed != 0xffffffff;
						surveys_file.put(has_info);
						if (has_info) {
							survey_amounts_file
								.put(f_player_field.resource_amounts.d);
							survey_times_file.Unsigned32(time_last_surveyed);
						}
					}
					if (f_everseen & br_everseen & r_everseen) {
						const uint32_t time_last_surveyed =
							f_player_field.time_triangle_last_surveyed[TCoords<>::R];
						const uint8_t has_info = time_last_surveyed != 0xffffffff;
						surveys_file.put(has_info);
						if (has_info) {
							survey_amounts_file
								.put(f_player_field.resource_amounts.r);
							survey_times_file.Unsigned32(time_last_surveyed);
						}
					}
				} while (r.x);
			}

			char filename[FILENAME_SIZE];

			snprintf(filename, sizeof(filename), PLAYERDIRNAME_TEMPLATE, plnum);
			fs.EnsureDirectoryExists(filename);
			snprintf(filename, sizeof(filename),       DIRNAME_TEMPLATE, plnum);
			fs.EnsureDirectoryExists(filename);

			WRITE
				(unseen_times_file,
				 UNSEEN_TIMES_FILENAME_TEMPLATE,
				 UNSEEN_TIMES_CURRENT_PACKET_VERSION);

			WRITE
				(node_immovable_kinds_file,
				 NODE_IMMOVABLE_KINDS_FILENAME_TEMPLATE,
				 NODE_IMMOVABLE_KINDS_CURRENT_PACKET_VERSION);

			WRITE
				(node_immovables_file,
				 NODE_IMMOVABLES_FILENAME_TEMPLATE,
				 NODE_IMMOVABLES_CURRENT_PACKET_VERSION);

			WRITE
				(roads_file,
				 ROADS_FILENAME_TEMPLATE,    ROADS_CURRENT_PACKET_VERSION);

			WRITE
				(terrains_file,
				 TERRAINS_FILENAME_TEMPLATE, TERRAINS_CURRENT_PACKET_VERSION);

			WRITE
				(triangle_immovable_kinds_file,
				 TRIANGLE_IMMOVABLE_KINDS_FILENAME_TEMPLATE,
				 TRIANGLE_IMMOVABLE_KINDS_CURRENT_PACKET_VERSION);

			WRITE
				(triangle_immovables_file,
				 TRIANGLE_IMMOVABLES_FILENAME_TEMPLATE,
				 TRIANGLE_IMMOVABLES_CURRENT_PACKET_VERSION);

			WRITE
				(owners_file,
				 OWNERS_FILENAME_TEMPLATE,   OWNERS_CURRENT_PACKET_VERSION);

			WRITE
				(surveys_file,
				 SURVEYS_FILENAME_TEMPLATE,  SURVEYS_CURRENT_PACKET_VERSION);

			WRITE
				(survey_amounts_file,
				 SURVEY_AMOUNTS_FILENAME_TEMPLATE,
				 SURVEY_AMOUNTS_CURRENT_PACKET_VERSION);

			WRITE
				(survey_times_file,
				 SURVEY_TIMES_FILENAME_TEMPLATE,
				 SURVEY_TIMES_CURRENT_PACKET_VERSION);

			WRITE
				(vision_file,
				 VISION_FILENAME_TEMPLATE,
				 VISION_CURRENT_PACKET_VERSION);

			WRITE
				(border_file,
				 BORDER_FILENAME_TEMPLATE,
				 BORDER_CURRENT_PACKET_VERSION);
		}
}

}
