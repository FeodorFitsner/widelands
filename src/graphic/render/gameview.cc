/*
 * Copyright (C) 2010-2012 by the Widelands Development Team
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

#include "gameview.h"
#include "upcast.h"

#include "economy/road.h"
#include "economy/flag.h"

#include "wui/minimap.h"
#include "wui/mapviewpixelconstants.h"

#include "logic/field.h"
#include "logic/map.h"
#include "logic/player.h"

#include "graphic/graphic.h"
#include "graphic/rendertarget.h"
#include "graphic/surface.h"
#include "surface_sdl.h"
#include "graphic/texture.h"

#include "wui/overlay_manager.h"

#include "terrain_sdl.h"

using Widelands::BaseImmovable;
using Widelands::Coords;
using Widelands::FCoords;
using Widelands::Map;
using Widelands::Map_Object_Descr;
using Widelands::Player;
using Widelands::TCoords;
using namespace  Widelands;

///This is used by rendermap to calculate the brightness of the terrain.
inline static Sint8 node_brightness
	(Widelands::Time   const gametime,
	 Widelands::Time   const last_seen,
	 Widelands::Vision const vision,
	 int8_t                  result)
{
	if      (vision == 0)
		result = -128;
	else if (vision == 1) {
		assert(last_seen <= gametime);
		Widelands::Duration const time_ago = gametime - last_seen;
		result =
			static_cast<Sint16>
			(((static_cast<Sint16>(result) + 128) >> 1)
			 *
			 (1.0 + (time_ago < 45000 ? expf(-8.46126929e-5 * time_ago) : 0)))
			-
			128;
	}

	return result;
}


#define RENDERMAP_INITIALIZANTONS                                             \
   viewofs -= m_offset;                                                       \
                                                                              \
   Map                   const & map             = egbase.map();              \
   Widelands::World      const & world           = map.world();               \
   Overlay_Manager       const & overlay_manager = map.get_overlay_manager(); \
   uint32_t const                mapwidth        = map.get_width();           \
   int32_t minfx, minfy;                                                      \
   int32_t maxfx, maxfy;                                                      \
                                                                              \
   /* hack to prevent negative numbers */                                     \
   minfx = (viewofs.x + (TRIANGLE_WIDTH >> 1)) / TRIANGLE_WIDTH - 1;          \
                                                                              \
   minfy = viewofs.y / TRIANGLE_HEIGHT;                                       \
   maxfx = (viewofs.x + (TRIANGLE_WIDTH >> 1) + m_rect.w) / TRIANGLE_WIDTH;   \
   maxfy = (viewofs.y + m_rect.h) / TRIANGLE_HEIGHT;                          \
   maxfx +=  1; /* because of big buildings */                                \
   maxfy += 10; /* because of heights */                                      \
                                                                              \
   int32_t dx              = maxfx - minfx + 1;                               \
   int32_t dy              = maxfy - minfy + 1;                               \
   int32_t linear_fy       = minfy;                                           \
   bool row_is_forward     = linear_fy & 1;                                   \
   int32_t b_posy          = linear_fy * TRIANGLE_HEIGHT - viewofs.y;         \


/**
 * Loop through fields row by row. For each field, draw ground textures, then
 * roads, then immovables, then bobs, then overlay stuff (build icons etc...)
 */
void GameView::rendermap
	(Widelands::Editor_Game_Base const &       egbase,
	 Widelands::Player           const &       player,
	 Point                                     viewofs)
{
	m_surface->fill_rect(m_rect, RGBAColor(0, 0, 0, 255));

	if (player.see_all())
		return rendermap(egbase, viewofs);

	RENDERMAP_INITIALIZANTONS;

	const Player::Field * const first_player_field = player.fields();
	Widelands::Time const gametime = egbase.get_gametime();

	rendermap_init();

	while (dy--) {
		const int32_t posy = b_posy;
		b_posy += TRIANGLE_HEIGHT;
		const int32_t linear_fx = minfx;
		FCoords r(Coords(linear_fx, linear_fy));
		FCoords br(Coords(linear_fx - not row_is_forward, linear_fy + 1));
		int32_t r_posx =
			r.x * TRIANGLE_WIDTH
			+
			row_is_forward * (TRIANGLE_WIDTH / 2)
			-
			viewofs.x;
		int32_t br_posx = r_posx - TRIANGLE_WIDTH / 2;

		// Calculate safe (bounded) field coordinates and get field pointers
		map.normalize_coords(r);
		map.normalize_coords(br);
		Widelands::Map_Index  r_index = Map::get_index (r, mapwidth);
		r.field = &map[r_index];
		Widelands::Map_Index br_index = Map::get_index(br, mapwidth);
		br.field = &map[br_index];
		const Player::Field *  r_player_field = first_player_field +  r_index;
		const Player::Field * br_player_field = first_player_field + br_index;
		FCoords tr, f;
		map.get_tln(r, &tr);
		map.get_ln(r, &f);
		Widelands::Map_Index tr_index = tr.field - &map[0];
		const Texture * f_r_texture =
			g_gr->get_maptexture_data
				(world
				 .terrain_descr(first_player_field[f.field - &map[0]].terrains.r)
				 .get_texture());

		uint32_t count = dx;

		while (count--) {
			const FCoords bl = br;
			const Player::Field &  f_player_field =  *r_player_field;
			const Player::Field & bl_player_field = *br_player_field;
			f = r;
			const int32_t f_posx = r_posx, bl_posx = br_posx;
			const Texture & l_r_texture = *f_r_texture;
			move_r(mapwidth, tr, tr_index);
			move_r(mapwidth,  r,  r_index);
			move_r(mapwidth, br, br_index);
			r_player_field  = first_player_field +  r_index;
			br_player_field = first_player_field + br_index;
			r_posx  += TRIANGLE_WIDTH;
			br_posx += TRIANGLE_WIDTH;
			const Texture & tr_d_texture =
				*g_gr->get_maptexture_data
					(world.terrain_descr(first_player_field[tr_index].terrains.d)
					 .get_texture());
			const Texture & f_d_texture =
				*g_gr->get_maptexture_data
					(world.terrain_descr(f_player_field.terrains.d).get_texture());
			f_r_texture =
				g_gr->get_maptexture_data
					(world.terrain_descr(f_player_field.terrains.r).get_texture());

			uint8_t const roads =
				f_player_field.roads | overlay_manager.get_road_overlay(f);

			Vertex f_vert
				(f_posx, posy - f.field->get_height() * HEIGHT_FACTOR,
				 node_brightness
				 	(gametime, f_player_field.time_node_last_unseen,
				 	 f_player_field.vision, f.field->get_brightness()),
				 0, 0);
			Vertex r_vert
				(r_posx, posy - r.field->get_height() * HEIGHT_FACTOR,
				 node_brightness
				 	(gametime, r_player_field->time_node_last_unseen,
				 	 r_player_field->vision, r.field->get_brightness()),
				 TRIANGLE_WIDTH, 0);
			Vertex bl_vert
				(bl_posx, b_posy - bl.field->get_height() * HEIGHT_FACTOR,
				 node_brightness
				 	(gametime, bl_player_field.time_node_last_unseen,
				 	 bl_player_field.vision, bl.field->get_brightness()),
				 0, 64);
			Vertex br_vert
				(br_posx, b_posy - br.field->get_height() * HEIGHT_FACTOR,
				 node_brightness
				 	(gametime, br_player_field->time_node_last_unseen,
				 	 br_player_field->vision, br.field->get_brightness()),
				 TRIANGLE_WIDTH, 64);

			if (row_is_forward) {
				f_vert.tx += TRIANGLE_WIDTH / 2;
				r_vert.tx += TRIANGLE_WIDTH / 2;
			} else {
				f_vert.tx += TRIANGLE_WIDTH;
				r_vert.tx += TRIANGLE_WIDTH;
				bl_vert.tx += TRIANGLE_WIDTH / 2;
				br_vert.tx += TRIANGLE_WIDTH / 2;
			}

			draw_field //  Render ground
				(m_rect,
				 f_vert, r_vert, bl_vert, br_vert,
				 roads,
				 tr_d_texture, l_r_texture, f_d_texture, *f_r_texture);
		}

		++linear_fy;
		row_is_forward = not row_is_forward;
	}

	{
		const int32_t dx2        = maxfx - minfx + 1;
		int32_t dy2              = maxfy - minfy + 1;
		int32_t linear_fy2       = minfy;
		bool row_is_forward2 = linear_fy2 & 1;
		int32_t b_posy2          = linear_fy2 * TRIANGLE_HEIGHT - viewofs.y;

		while (dy2--) {
			const int32_t posy = b_posy2;
			b_posy2 += TRIANGLE_HEIGHT;

			{ //  Draw things on the node.
				const int32_t linear_fx = minfx;
				FCoords r(Coords(linear_fx, linear_fy2));
				FCoords br
					(Coords(linear_fx - not row_is_forward2, linear_fy2 + 1));

				//  Calculate safe (bounded) field coordinates and get field
				//  pointers.
				map.normalize_coords(r);
				map.normalize_coords(br);
				Widelands::Map_Index  r_index = Map::get_index (r, mapwidth);
				r.field = &map[r_index];
				Widelands::Map_Index br_index = Map::get_index(br, mapwidth);
				br.field = &map[br_index];
				FCoords tr, f;
				map.get_tln(r, &tr);
				map.get_ln(r, &f);
				bool r_is_border;
				uint8_t f_owner_number = f.field->get_owned_by(); //  FIXME PPoV
				uint8_t r_owner_number;
				r_is_border = r.field->is_border(); //  FIXME PPoV
				r_owner_number = r.field->get_owned_by(); //  FIXME PPoV
				uint8_t br_owner_number = br.field->get_owned_by(); //  FIXME PPoV
				Player::Field const * r_player_field = first_player_field + r_index;
				const Player::Field * br_player_field = first_player_field + br_index;
				Widelands::Vision  r_vision =  r_player_field->vision;
				Widelands::Vision br_vision = br_player_field->vision;
				Point r_pos
					(linear_fx * TRIANGLE_WIDTH
					 +
					 row_is_forward2 * (TRIANGLE_WIDTH / 2)
					 -
					 viewofs.x,
					 posy - r.field->get_height() * HEIGHT_FACTOR);
				Point br_pos
					(r_pos.x - TRIANGLE_WIDTH / 2,
					 b_posy2 - br.field->get_height() * HEIGHT_FACTOR);

				int32_t count = dx2;

				while (count--) {
					f = r;
					const Player::Field & f_player_field = *r_player_field;
					move_r(mapwidth, tr);
					move_r(mapwidth,  r,  r_index);
					move_r(mapwidth, br, br_index);
					r_player_field  = first_player_field +  r_index;
					br_player_field = first_player_field + br_index;

					//  FIXME PPoV
					const uint8_t tr_owner_number = tr.field->get_owned_by();

					const bool f_is_border = r_is_border;
					const uint8_t l_owner_number = f_owner_number;
					const uint8_t bl_owner_number = br_owner_number;
					f_owner_number = r_owner_number;
					r_is_border = r.field->is_border();         //  FIXME PPoV
					r_owner_number = r.field->get_owned_by();   //  FIXME PPoV
					br_owner_number = br.field->get_owned_by(); //  FIXME PPoV
					Widelands::Vision const  f_vision =  r_vision;
					Widelands::Vision const bl_vision = br_vision;
					r_vision  = player.vision (r_index);
					br_vision = player.vision(br_index);
					const Point f_pos = r_pos, bl_pos = br_pos;
					r_pos = Point
						(r_pos.x + TRIANGLE_WIDTH,
						 posy - r.field->get_height() * HEIGHT_FACTOR);
					br_pos = Point
						(br_pos.x + TRIANGLE_WIDTH,
						 b_posy2 - br.field->get_height() * HEIGHT_FACTOR);

					//  Render border markes on and halfway between border nodes.
					if (f_is_border) {
						const Player & owner = egbase.player(f_owner_number);
						uint32_t const anim = owner.frontier_anim();
						if (1 < f_vision)
							drawanim(f_pos, anim, 0, &owner);
						if
							((f_vision | r_vision)
							 and
							 r_owner_number == f_owner_number
							 and
							 ((tr_owner_number == f_owner_number)
							  xor
							  (br_owner_number == f_owner_number)))
							drawanim(middle(f_pos, r_pos), anim, 0, &owner);
						if
							((f_vision | bl_vision)
							 and
							 bl_owner_number == f_owner_number
							 and
							 ((l_owner_number == f_owner_number)
							  xor
							  (br_owner_number == f_owner_number)))
							drawanim(middle(f_pos, bl_pos), anim, 0, &owner);
						if
							((f_vision | br_vision)
							 and
							 br_owner_number == f_owner_number
							 and
							 ((r_owner_number == f_owner_number)
							  xor
							  (bl_owner_number == f_owner_number)))
							drawanim(middle(f_pos, br_pos), anim, 0, &owner);
					}

					if (1 < f_vision) { // Render stuff that belongs to the node.

						// Render bobs
						// TODO - rendering order?
						//  This must be defined somehow. Some bobs have a higher
						//  priority than others. Maybe this priority is a moving
						//  versus non-moving bobs thing? draw_ground implies that
						//  this doesn't render map objects. Are there any overdraw
						//  issues with the current rendering order?

						// Draw Map_Objects hooked to this field
						if (BaseImmovable * const imm = f.field->get_immovable())
							imm->draw(egbase, *this, f, f_pos);
						for
							(Widelands::Bob * bob = f.field->get_first_bob();
							 bob;
							 bob = bob->get_next_bob())
							bob->draw(egbase, *this, f_pos);

						//  Render overlays on nodes.
						Overlay_Manager::Overlay_Info
							overlay_info[MAX_OVERLAYS_PER_NODE];

						const Overlay_Manager::Overlay_Info * const end =
							overlay_info
							+
							overlay_manager.get_overlays(f, overlay_info);

						for
							(const Overlay_Manager::Overlay_Info * it = overlay_info;
							 it < end;
							 ++it)
							blit(f_pos - it->hotspot, it->picid);
					} else if (f_vision == 1)
						if
							(const Map_Object_Descr * const map_object_descr =
							 f_player_field.map_object_descr[TCoords<>::None])
						{
							Player const * const owner = f_owner_number ? egbase.get_player(f_owner_number) : 0;
							if
								(const Player::Constructionsite_Information * const csinf =
								 f_player_field.constructionsite[TCoords<>::None])
							{
								// draw the partly finished constructionsite
								uint32_t anim;
								try {
									anim = csinf->becomes->get_animation("build");
								} catch (Map_Object_Descr::Animation_Nonexistent) {
									try {
										anim = csinf->becomes->get_animation("unoccupied");
									} catch (Map_Object_Descr::Animation_Nonexistent) {
										anim = csinf->becomes->get_animation("idle");
									}
								}
								const AnimationGfx::Index nr_frames = g_gr->nr_frames(anim);
								uint32_t cur_frame =
									csinf->totaltime ? csinf->completedtime * nr_frames / csinf->totaltime : 0;
								uint32_t tanim = cur_frame * FRAME_LENGTH;
								uint32_t w, h;
								g_gr->get_animation_size(anim, tanim, w, h);
								uint32_t lines = h * csinf->completedtime * nr_frames;
								if (csinf->totaltime)
									lines /= csinf->totaltime;
								assert(h * cur_frame <= lines);
								lines -= h * cur_frame; //  This won't work if pictures have various sizes.

								if (cur_frame) // not the first frame
									// draw the prev frame from top to where next image will be drawing
									drawanimrect
										(f_pos, anim, tanim - FRAME_LENGTH, owner, Rect(Point(0, 0), w, h - lines));
								else if (csinf->was) {
									// Is the first frame, but there was another building here before,
									// get its last build picture and draw it instead.
									uint32_t a;
									try {
										a = csinf->was->get_animation("unoccupied");
									} catch (Map_Object_Descr::Animation_Nonexistent) {
										a = csinf->was->get_animation("idle");
									}
									drawanimrect
										(f_pos, a, tanim - FRAME_LENGTH, owner, Rect(Point(0, 0), w, h - lines));
								}
								assert(lines <= h);
								drawanimrect(f_pos, anim, tanim, owner, Rect(Point(0, h - lines), w, lines));
							} else if (upcast(const Building_Descr, building, map_object_descr)) {
								// this is a building therefore we either draw unoccupied or idle animation
								uint32_t picid;
								try {
									picid = building->get_animation("unoccupied");
								} catch (Map_Object_Descr::Animation_Nonexistent) {
									picid = building->get_animation("idle");
								}
								drawanim(f_pos, picid, 0, owner);
							} else if (const uint32_t picid = map_object_descr->main_animation()) {
								drawanim(f_pos, picid, 0, owner);
							} else if (map_object_descr == &Widelands::g_flag_descr) {
								drawanim(f_pos, owner->flag_anim(), 0, owner);
							}
						}
				}
			}

			if (false) { //  Draw things on the R-triangle (nothing to draw yet).
				const int32_t linear_fx = minfx;
				FCoords r(Coords(linear_fx, linear_fy2));
				FCoords b(Coords(linear_fx - not row_is_forward2, linear_fy2 + 1));
				int32_t posx =
					(linear_fx - 1) * TRIANGLE_WIDTH
					+
					(row_is_forward2 + 1) * (TRIANGLE_WIDTH / 2)
					-
					viewofs.x;

				//  Calculate safe (bounded) field coordinates.
				map.normalize_coords(r);
				map.normalize_coords(b);

				//  Get field pointers.
				r.field = &map[Map::get_index(r, mapwidth)];
				b.field = &map[Map::get_index(b, mapwidth)];

				int32_t count = dx2;

				//  One less iteration than for nodes and D-triangles.
				while (--count) {
					const FCoords f = r;
					map.get_rn(r, &r);
					map.get_rn(b, &b);
					posx += TRIANGLE_WIDTH;

					//  FIXME Implement visibility rules for objects on triangles
					//  FIXME when they are used in the game. The only things that
					//  FIXME are drawn on triangles now (except the ground) are
					//  FIXME overlays for the editor terrain tool, and the editor
					//  FIXME does not need visibility rules.
					{ //  FIXME Visibility check here.
						Overlay_Manager::Overlay_Info overlay_info
							[MAX_OVERLAYS_PER_TRIANGLE];
						const Overlay_Manager::Overlay_Info & overlay_info_end = *
							(overlay_info
							 +
							 overlay_manager.get_overlays
							 	(TCoords<>(f, TCoords<>::R), overlay_info));

						for
							(const Overlay_Manager::Overlay_Info * it = overlay_info;
							 it < &overlay_info_end;
							 ++it)
							blit
								(Point
								 	(posx,
								 	 posy
								 	 +
								 	 (TRIANGLE_HEIGHT
								 	  -
								 	  (f.field->get_height()
								 	   +
								 	   r.field->get_height()
								 	   +
								 	   b.field->get_height())
								 	  *
								 	  HEIGHT_FACTOR)
								 	 /
								 	 3)
								 -
								 it->hotspot,
								 it->picid);
					}
				}
			}

			if (false) { //  Draw things on the D-triangle (nothing to draw yet).
				const int32_t linear_fx = minfx;
				FCoords f(Coords(linear_fx - 1, linear_fy2));
				FCoords br
					(Coords(linear_fx - not row_is_forward2, linear_fy2 + 1));
				int32_t posx =
					(linear_fx - 1) * TRIANGLE_WIDTH
					+
					row_is_forward2 * (TRIANGLE_WIDTH / 2)
					-
					viewofs.x;

				//  Calculate safe (bounded) field coordinates.
				map.normalize_coords(f);
				map.normalize_coords(br);

				//  Get field pointers.
				f.field  = &map[Map::get_index(f,  mapwidth)];
				br.field = &map[Map::get_index(br, mapwidth)];

				int32_t count = dx2;

				while (count--) {
					const FCoords bl = br;
					map.get_rn(f, &f);
					map.get_rn(br, &br);
					posx += TRIANGLE_WIDTH;

					{ //  FIXME Visibility check here.
						Overlay_Manager::Overlay_Info overlay_info
							[MAX_OVERLAYS_PER_TRIANGLE];
						Overlay_Manager::Overlay_Info const * const overlay_info_end
							=
							overlay_info
							+
							overlay_manager.get_overlays
								(TCoords<>(f, TCoords<>::D), overlay_info);

						for
							(const Overlay_Manager::Overlay_Info * it = overlay_info;
							 it < overlay_info_end;
							 ++it)
							blit
								(Point
								 	(posx,
								 	 posy
								 	 +
								 	 ((TRIANGLE_HEIGHT * 2)
								 	  -
								 	  (f.field->get_height()
								 	   +
								 	   bl.field->get_height()
								 	   +
								 	   br.field->get_height())
								 	  *
								 	  HEIGHT_FACTOR)
								 	 /
								 	 3)
								 -
								 it->hotspot,
								 it->picid);
					}
				}
			}

			++linear_fy2;
			row_is_forward2 = not row_is_forward2;
		}
	}

	rendermap_deint();

	g_gr->reset_texture_animation_reminder();
}

void GameView::rendermap
	(Widelands::Editor_Game_Base const &       egbase,
	 Point                                     viewofs)
{
	RENDERMAP_INITIALIZANTONS;

	rendermap_init();

	while (dy--) {
		const int32_t posy = b_posy;
		b_posy += TRIANGLE_HEIGHT;
		const int32_t linear_fx = minfx;
		FCoords r(Coords(linear_fx, linear_fy));
		FCoords br(Coords(linear_fx - not row_is_forward, linear_fy + 1));
		int32_t r_posx =
			r.x * TRIANGLE_WIDTH
			+
			row_is_forward * (TRIANGLE_WIDTH / 2)
			-
			viewofs.x;
		int32_t br_posx = r_posx - TRIANGLE_WIDTH / 2;

		// Calculate safe (bounded) field coordinates and get field pointers
		map.normalize_coords(r);
		map.normalize_coords(br);
		Widelands::Map_Index  r_index = Map::get_index (r, mapwidth);
		r.field = &map[r_index];
		Widelands::Map_Index br_index = Map::get_index(br, mapwidth);
		br.field = &map[br_index];
		FCoords tr, f;
		map.get_tln(r, &tr);
		map.get_ln(r, &f);
		const Texture * f_r_texture =
			g_gr->get_maptexture_data
				(world.terrain_descr(f.field->terrain_r()).get_texture());

		uint32_t count = dx;

		while (count--) {
			const FCoords bl = br;
			f = r;
			const int32_t f_posx = r_posx, bl_posx = br_posx;
			const Texture & l_r_texture = *f_r_texture;
			move_r(mapwidth, tr);
			move_r(mapwidth,  r,  r_index);
			move_r(mapwidth, br, br_index);
			r_posx  += TRIANGLE_WIDTH;
			br_posx += TRIANGLE_WIDTH;
			const Texture & tr_d_texture =
				*g_gr->get_maptexture_data
					(world.terrain_descr(tr.field->terrain_d()).get_texture());
			const Texture & f_d_texture =
				*g_gr->get_maptexture_data
					(world.terrain_descr(f.field->terrain_d()).get_texture());
			f_r_texture =
				g_gr->get_maptexture_data
					(world.terrain_descr(f.field->terrain_r()).get_texture());

			const uint8_t roads =
				f.field->get_roads() | overlay_manager.get_road_overlay(f);

			Vertex f_vert
				(f_posx, posy - f.field->get_height() * HEIGHT_FACTOR,
				 f.field->get_brightness(),
				 0, 0);
			Vertex r_vert
				(r_posx, posy - r.field->get_height() * HEIGHT_FACTOR,
				 r.field->get_brightness(),
				 TRIANGLE_WIDTH, 0);
			Vertex bl_vert
				(bl_posx, b_posy - bl.field->get_height() * HEIGHT_FACTOR,
				 bl.field->get_brightness(),
				 0, 64);
			Vertex br_vert
				(br_posx, b_posy - br.field->get_height() * HEIGHT_FACTOR,
				 br.field->get_brightness(),
				 TRIANGLE_WIDTH, 64);

			if (row_is_forward) {
				f_vert.tx += TRIANGLE_WIDTH / 2;
				r_vert.tx += TRIANGLE_WIDTH / 2;
			} else {
				bl_vert.tx -= TRIANGLE_WIDTH / 2;
				br_vert.tx -= TRIANGLE_WIDTH / 2;
			}

			draw_field //  Render ground
				(m_rect,
				 f_vert, r_vert, bl_vert, br_vert,
				 roads,
				 tr_d_texture, l_r_texture, f_d_texture, *f_r_texture);
		}

		++linear_fy;
		row_is_forward = not row_is_forward;
	}

	{
		const int32_t dx2 = maxfx - minfx + 1;
		int32_t dy2 = maxfy - minfy + 1;
		int32_t linear_fy2 = minfy;
		bool row_is_forward2 = linear_fy2 & 1;
		int32_t b_posy2 = linear_fy2 * TRIANGLE_HEIGHT - viewofs.y;

		while (dy2--) {
			const int32_t posy = b_posy2;
			b_posy2 += TRIANGLE_HEIGHT;

			{ //  Draw things on the node.
				const int32_t linear_fx = minfx;
				FCoords r(Coords(linear_fx, linear_fy2));
				FCoords br
					(Coords(linear_fx - not row_is_forward2, linear_fy2 + 1));

				//  Calculate safe (bounded) field coordinates and get field
				//  pointers.
				map.normalize_coords(r);
				map.normalize_coords(br);
				Widelands::Map_Index  r_index = Map::get_index (r, mapwidth);
				r.field = &map[r_index];
				Widelands::Map_Index br_index = Map::get_index(br, mapwidth);
				br.field = &map[br_index];
				FCoords tr, f;
				map.get_tln(r, &tr);
				map.get_ln(r, &f);
				bool r_is_border;
				uint8_t f_owner_number = f.field->get_owned_by();
				uint8_t r_owner_number;
				r_is_border = r.field->is_border();
				r_owner_number = r.field->get_owned_by();
				uint8_t br_owner_number = br.field->get_owned_by();
				Point r_pos
					(linear_fx * TRIANGLE_WIDTH
					 +
					 row_is_forward2 * (TRIANGLE_WIDTH / 2)
					 -
					 viewofs.x,
					 posy - r.field->get_height() * HEIGHT_FACTOR);
				Point br_pos
					(r_pos.x - TRIANGLE_WIDTH / 2,
					 b_posy2 - br.field->get_height() * HEIGHT_FACTOR);

				int32_t count = dx2;

				while (count--) {
					f = r;
					move_r(mapwidth, tr);
					move_r(mapwidth,  r,  r_index);
					move_r(mapwidth, br, br_index);
					const uint8_t tr_owner_number = tr.field->get_owned_by();
					const bool f_is_border = r_is_border;
					const uint8_t l_owner_number = f_owner_number;
					const uint8_t bl_owner_number = br_owner_number;
					f_owner_number = r_owner_number;
					r_is_border = r.field->is_border();
					r_owner_number = r.field->get_owned_by();
					br_owner_number = br.field->get_owned_by();
					const Point f_pos = r_pos, bl_pos = br_pos;
					r_pos = Point
						(r_pos.x + TRIANGLE_WIDTH,
						 posy - r.field->get_height() * HEIGHT_FACTOR);
					br_pos = Point
						(br_pos.x + TRIANGLE_WIDTH,
						 b_posy2 - br.field->get_height() * HEIGHT_FACTOR);

					//  Render border markes on and halfway between border nodes.
					if (f_is_border) {
						const Player & owner = egbase.player(f_owner_number);
						uint32_t const anim = owner.frontier_anim();
						drawanim(f_pos, anim, 0, &owner);
						if
							(r_owner_number == f_owner_number
							 and
							 ((tr_owner_number == f_owner_number)
							  xor
							  (br_owner_number == f_owner_number)))
							drawanim(middle(f_pos, r_pos), anim, 0, &owner);
						if
							(bl_owner_number == f_owner_number
							 and
							 ((l_owner_number == f_owner_number)
							  xor
							  (br_owner_number == f_owner_number)))
							drawanim(middle(f_pos, bl_pos), anim, 0, &owner);
						if
							(br_owner_number == f_owner_number
							 and
							 ((r_owner_number == f_owner_number)
							  xor
							  (bl_owner_number == f_owner_number)))
							drawanim(middle(f_pos, br_pos), anim, 0, &owner);
					}

					{ // Render stuff that belongs to the node.

						// Render bobs
						// TODO - rendering order?
						//  This must be defined somehow. Some bobs have a higher
						//  priority than others. Maybe this priority is a moving
						//  versus non-moving bobs thing? draw_ground implies that
						//  this doesn't render map objects. Are there any overdraw
						//  issues with the current rendering order?

						// Draw Map_Objects hooked to this field
						if (BaseImmovable * const imm = f.field->get_immovable())
							imm->draw(egbase, *this, f, f_pos);
						for
							(Widelands::Bob * bob = f.field->get_first_bob();
							 bob;
							 bob = bob->get_next_bob())
							bob->draw(egbase, *this, f_pos);

						//  Render overlays on nodes.
						Overlay_Manager::Overlay_Info
							overlay_info[MAX_OVERLAYS_PER_NODE];

						const Overlay_Manager::Overlay_Info * const end =
							overlay_info
							+
							overlay_manager.get_overlays(f, overlay_info);

						for
							(const Overlay_Manager::Overlay_Info * it = overlay_info;
							 it < end;
							 ++it)
							blit(f_pos - it->hotspot, it->picid);
					}
				}
			}

			{ //  Draw things on the R-triangle.
				const int32_t linear_fx = minfx;
				FCoords r(Coords(linear_fx, linear_fy2));
				FCoords b
					(Coords(linear_fx - not row_is_forward2, linear_fy2 + 1));
				int32_t posx =
					(linear_fx - 1) * TRIANGLE_WIDTH
					+
					(row_is_forward2 + 1) * (TRIANGLE_WIDTH / 2)
					-
					viewofs.x;

				//  Calculate safe (bounded) field coordinates.
				map.normalize_coords(r);
				map.normalize_coords(b);

				//  Get field pointers.
				r.field = &map[Map::get_index(r, mapwidth)];
				b.field = &map[Map::get_index(b, mapwidth)];

				int32_t count = dx2;

				//  One less iteration than for nodes and D-triangles.
				while (--count) {
					const FCoords f = r;
					map.get_rn(r, &r);
					map.get_rn(b, &b);
					posx += TRIANGLE_WIDTH;

					{
						Overlay_Manager::Overlay_Info overlay_info
							[MAX_OVERLAYS_PER_TRIANGLE];
						Overlay_Manager::Overlay_Info const & overlay_info_end =
							*
							(overlay_info
							 +
							 overlay_manager.get_overlays
							 	(TCoords<>(f, TCoords<>::R), overlay_info));

						for
							(Overlay_Manager::Overlay_Info const * it = overlay_info;
							 it < &overlay_info_end;
							 ++it)
							blit
								(Point
								 	(posx,
								 	 posy
								 	 +
								 	 (TRIANGLE_HEIGHT
								 	  -
								 	  (f.field->get_height()
								 	   +
								 	   r.field->get_height()
								 	   +
								 	   b.field->get_height())
								 	  *
								 	  HEIGHT_FACTOR)
								 	 /
								 	 3)
								 -
								 it->hotspot,
								 it->picid);
					}
				}
			}

			{ //  Draw things on the D-triangle.
				const int32_t linear_fx = minfx;
				FCoords f(Coords(linear_fx - 1, linear_fy2));
				FCoords br(Coords(linear_fx - not row_is_forward2, linear_fy2 + 1));
				int32_t posx =
					(linear_fx - 1) * TRIANGLE_WIDTH
					+
					row_is_forward2 * (TRIANGLE_WIDTH / 2)
					-
					viewofs.x;

				//  Calculate safe (bounded) field coordinates.
				map.normalize_coords(f);
				map.normalize_coords(br);

				//  Get field pointers.
				f.field  = &map[Map::get_index(f,  mapwidth)];
				br.field = &map[Map::get_index(br, mapwidth)];

				int32_t count = dx2;

				while (count--) {
					const FCoords bl = br;
					map.get_rn(f, &f);
					map.get_rn(br, &br);
					posx += TRIANGLE_WIDTH;

					{
						Overlay_Manager::Overlay_Info overlay_info
							[MAX_OVERLAYS_PER_TRIANGLE];
						const Overlay_Manager::Overlay_Info & overlay_info_end = *
							(overlay_info
							 +
							 overlay_manager.get_overlays
							 	(TCoords<>(f, TCoords<>::D), overlay_info));

						for
							(const Overlay_Manager::Overlay_Info * it = overlay_info;
							 it < &overlay_info_end;
							 ++it)
							blit
								(Point
								 	(posx,
								 	 posy
								 	 +
								 	 ((TRIANGLE_HEIGHT * 2)
								 	  -
								 	  (f.field->get_height()
								 	   +
								 	   bl.field->get_height()
								 	   +
								 	   br.field->get_height())
								 	  *
								 	  HEIGHT_FACTOR)
								 	 /
								 	 3)
								 -
								 it->hotspot,
								 it->picid);
					}
				}
			}

			++linear_fy2;
			row_is_forward2 = not row_is_forward2;
		}
	}

	rendermap_deint();

	g_gr->reset_texture_animation_reminder();
}


/**
 * Renders a minimap into the current window. The field at viewpoint will be
 * in the top-left corner of the window. Flags specifies what information to
 * display (see Minimap_XXX enums).
 *
 * Calculate the field at the top-left corner of the clipping rect
 * The entire clipping rect will be used for drawing.
 */
void GameView::renderminimap
	(Widelands::Editor_Game_Base const &       egbase,
	 Player                      const * const player,
	 Point                               const viewpoint,
	 uint32_t                            const flags)
{
	draw_minimap(egbase, player, m_rect, viewpoint - m_offset, flags);
}



/**
 * Draw ground textures and roads for the given parallelogram (two triangles)
 * into the bitmap.
 *
 * Vertices:
 *   - f_vert vertex of the field
 *   - r_vert vertex right of the field
 *   - bl_vert vertex bottom left of the field
 *   - br_vert vertex bottom right of the field
 *
 * Textures:
 *   - f_r_texture Terrain of the triangle right of the field
 *   - f_d_texture Terrain of the triangle under of the field
 *   - tr_d_texture Terrain of the triangle top of the right triangle ??
 *   - l_r_texture Terrain of the triangle left of the down triangle ??
 *
 *             (tr_d)
 *
 *       (f) *------* (r)
 *          / \  r /
 *  (l_r)  /   \  /
 *        /  d  \/
 *  (bl) *------* (br)
 */

void GameView::draw_field
	(Rect          & subwin,
	 Vertex  const &  f_vert,
	 Vertex  const &  r_vert,
	 Vertex  const & bl_vert,
	 Vertex  const & br_vert,
	 uint8_t         roads,
	 Texture const & tr_d_texture,
	 Texture const &  l_r_texture,
	 Texture const &  f_d_texture,
	 Texture const &  f_r_texture)
{

}

/*
 * Blend to colors; only needed for calc_minimap_color below
 */

inline static uint32_t blend_color
	(SDL_PixelFormat const &       format,
	 uint32_t                const clr1,
	 Uint8 const r2, Uint8 const g2, Uint8 const b2)
{
	Uint8 r1, g1, b1;
	SDL_GetRGB(clr1, &const_cast<SDL_PixelFormat &>(format), &r1, &g1, &b1);
	return
		SDL_MapRGB
			(&const_cast<SDL_PixelFormat &>(format),
			 (r1 + r2) / 2, (g1 + g2) / 2, (b1 + b2) / 2);
}

/*
===============
Return the color to be used in the minimap for the given field.
===============
*/

inline static uint32_t calc_minimap_color
	(SDL_PixelFormat             const &       format,
	 Widelands::Editor_Game_Base const &       egbase,
	 Widelands::FCoords                  const f,
	 uint32_t                            const flags,
	 Widelands::Player_Number            const owner,
	 bool                                const see_details)
{
	uint32_t pixelcolor = 0;

	if (flags & MiniMap::Terrn) {
		pixelcolor =
			g_gr->
			get_maptexture_data
				(egbase.map().world()
				 .terrain_descr(f.field->terrain_d()).get_texture())
			->get_minimap_color(f.field->get_brightness());
	}

	if (flags & MiniMap::Owner) {
		if (0 < owner) { //  If owned, get the player's color...
			const RGBColor & player_color = egbase.player(owner).get_playercolor();

			//  ...and add the player's color to the old color.
			pixelcolor = blend_color
				(format,
				 pixelcolor,
				 player_color.r(),  player_color.g(), player_color.b());
		}
	}

	if (see_details)
		if (upcast(PlayerImmovable const, immovable, f.field->get_immovable())) {
			if (flags & MiniMap::Roads and dynamic_cast<Road const *>(immovable))
				pixelcolor = blend_color(format, pixelcolor, 255, 255, 255);
			if
				((flags & MiniMap::Flags and dynamic_cast<Flag const *>(immovable))
				 or
				 (flags & MiniMap::Bldns
				  and
				  dynamic_cast<Widelands::Building const *>(immovable)))
				pixelcolor =
					SDL_MapRGB
						(&const_cast<SDL_PixelFormat &>(format), 255, 255, 255);
		}

	return pixelcolor;
}


/*
===============
Used to draw a dotted frame border on the mini map.
===============
 */
template<typename T>
static bool draw_minimap_frameborder
	(Widelands::FCoords  const f,
	 Point               const ptopleft,
	 Point               const pbottomright,
	 int32_t             const mapwidth,
	 int32_t             const mapheight,
	 int32_t             const modx,
	 int32_t             const mody)
{
	bool isframepixel = false;

	if (ptopleft.x <= pbottomright.x) {
		if
			(f.x >= ptopleft.x && f.x <= pbottomright.x
			 && (f.y == ptopleft.y || f.y == pbottomright.y)
			 && f.x % 2 == modx)
			isframepixel = true;
	} else {
		if
			(((f.x >= ptopleft.x && f.x <= mapwidth)
			  ||
			  (f.x >= 0 && f.x <= pbottomright.x))
			 &&
			 (f.y == ptopleft.y || f.y == pbottomright.y)
			 &&
			 (f.x % 2) == modx)
			isframepixel = true;
	}

	if (ptopleft.y <= pbottomright.y) {
		if
			(f.y >= ptopleft.y && f.y <= pbottomright.y
			 && (f.x == ptopleft.x || f.x == pbottomright.x)
			 && f.y % 2 == mody)
			isframepixel = true;
	} else {
		if
			(((f.y >= ptopleft.y && f.y <= mapheight)
			  ||
			  (f.y >= 0 && f.y <= pbottomright.y))
			 &&
			 (f.x == ptopleft.x || f.x == pbottomright.x)
			 &&
			 f.y % 2 == mody)
			isframepixel = true;
	}

	return isframepixel;
}

/*
 *
 *
 *
 */
template<typename T>
static void draw_minimap_int
	(Uint8                             * const pixels,
	 uint16_t                            const pitch,
	 SDL_PixelFormat             const &       format,
	 int32_t                            const mapwidth,
	 Widelands::Editor_Game_Base const &       egbase,
	 Widelands::Player           const * const player,
	 Rect                                const rc,
	 Point                               const viewpoint,
	 uint32_t                            const flags)
{
	Widelands::Map const & map = egbase.map();

	int32_t mapheight = (flags & MiniMap::Zoom2 ? rc.h / 2 : rc.h);

	// size of the display frame
	int32_t xsize = g_gr->get_xres() / TRIANGLE_WIDTH / 2;
	int32_t ysize = g_gr->get_yres() / TRIANGLE_HEIGHT / 2;

	Point ptopleft; // top left point of the current display frame
	ptopleft.x = viewpoint.x + mapwidth / 2 - xsize;
	if (ptopleft.x < 0) ptopleft.x += mapwidth;
	ptopleft.y = viewpoint.y + mapheight / 2 - ysize;
	if (ptopleft.y < 0) ptopleft.y += mapheight;

	Point pbottomright; // bottom right point of the current display frame
	pbottomright.x = viewpoint.x + mapwidth / 2 + xsize;
	if (pbottomright.x >= mapwidth) pbottomright.x -= mapwidth;
	pbottomright.y = viewpoint.y + mapheight / 2 + ysize;
	if (pbottomright.y >= mapheight) pbottomright.y -= mapheight;

	uint32_t modx = pbottomright.x % 2;
	uint32_t mody = pbottomright.y % 2;

	if (not player or player->see_all()) for (uint32_t y = 0; y < rc.h; ++y) {
		Uint8 * pix = pixels + (rc.y + y) * pitch + rc.x * sizeof(T);
		Widelands::FCoords f
			(Widelands::Coords
			 	(viewpoint.x, viewpoint.y + (flags & MiniMap::Zoom2 ? y / 2 : y)));
		map.normalize_coords(f);
		f.field = &map[f];
		Widelands::Map_Index i = Widelands::Map::get_index(f, mapwidth);
		for (uint32_t x = 0; x < rc.w; ++x, pix += sizeof(T)) {
			if (x % 2 || !(flags & MiniMap::Zoom2))
				move_r(mapwidth, f, i);

			if
				(draw_minimap_frameborder<T>
				 (f, ptopleft, pbottomright, mapwidth, mapheight, modx, mody))
			{
				*reinterpret_cast<T *>(pix) = static_cast<T>
					(SDL_MapRGB(&const_cast<SDL_PixelFormat &>(format), 255, 0, 0));
			} else {
				*reinterpret_cast<T *>(pix) = static_cast<T>
					(calc_minimap_color
				 		(format, egbase, f, flags, f.field->get_owned_by(), true));
			}
		}
	} else {
		Widelands::Player::Field const * const player_fields = player->fields();
		for (uint32_t y = 0; y < rc.h; ++y) {
			Uint8 * pix = pixels + (rc.y + y) * pitch + rc.x * sizeof(T);
			Widelands::FCoords f
				(Widelands::Coords
			 		(viewpoint.x, viewpoint.y +
			 		 (flags & MiniMap::Zoom2 ? y / 2 : y)));
			map.normalize_coords(f);
			f.field = &map[f];
			Widelands::Map_Index i = Widelands::Map::get_index(f, mapwidth);
			for (uint32_t x = 0; x < rc.w; ++x, pix += sizeof(T)) {
				if (x % 2 || !(flags & MiniMap::Zoom2))
					move_r(mapwidth, f, i);

				if
					(draw_minimap_frameborder<T>
					 (f, ptopleft, pbottomright, mapwidth, mapheight, modx, mody))
				{
					*reinterpret_cast<T *>(pix) = static_cast<T>
						(SDL_MapRGB
							(&const_cast<SDL_PixelFormat &>(format), 255, 0, 0));
				} else {
					Widelands::Player::Field const & player_field = player_fields[i];
					Widelands::Vision const vision = player_field.vision;

					*reinterpret_cast<T *>(pix) =
						static_cast<T>
						(vision ?
						 calc_minimap_color
						 	(format,
						 	 egbase,
						 	 f,
						 	 flags,
						 	 player_field.owner,
						 	 1 < vision)
						 :
						 0);
				}
			}
		}
	}
}


/*
===============
Draw a minimap into the given rectangle of the bitmap.
viewpt is the field at the top left of the rectangle.
===============
*/
void GameView::draw_minimap
	(Widelands::Editor_Game_Base const &       egbase,
	 Widelands::Player           const * const player,
	 Rect                                const rc,
	 Point                               const viewpt,
	 uint32_t                            const flags)
{
	// First create a temporary SDL Surface to draw the minimap. This Surface is
	// is created in almost display pixel format without alpha channel.
	// Bits per pixels must be 16 or 32 for the minimap code to work
	// TODO: Currently the minimap is redrawn every frame. That is not really
	//       necesary. The created surface could be cached and only redrawn two
	//       or three times per second
	const SDL_PixelFormat & fmt =
		g_gr->get_render_target()->get_surface()->pixelaccess().format();
	SDL_Surface * surface =
		SDL_CreateRGBSurface
			(SDL_SWSURFACE,
			 rc.w,
			 rc.h,
			 fmt.BytesPerPixel == 2 ? 16 : 32,
			 fmt.Rmask,
			 fmt.Gmask,
			 fmt.Bmask,
			 0);

	Rect rc2;
	rc2.x = rc2.y = 0;
	rc2.w = rc.w;
	rc2.h = rc.h;

	SDL_FillRect(surface, 0, SDL_MapRGBA(surface->format, 0, 0, 0, 255));
	SDL_LockSurface(surface);

	Uint8 * const pixels = static_cast<uint8_t *>(surface->pixels);
	Widelands::X_Coordinate const w = egbase.map().get_width();
	switch (surface->format->BytesPerPixel) {
	case sizeof(Uint16):
		draw_minimap_int<Uint16>
			(pixels, surface->pitch, *surface->format,
			 w, egbase, player, rc2, viewpt, flags);
		break;
	case sizeof(Uint32):
		draw_minimap_int<Uint32>
			(pixels, surface->pitch, *surface->format,
			 w, egbase, player, rc2, viewpt, flags);
		break;
	default:
		assert (false);
	}

	SDL_UnlockSurface(surface);

	PictureID picture = g_gr->convert_sdl_surface_to_picture(surface);

	m_surface->blit(Point(rc.x, rc.y), picture, rc2);
}
