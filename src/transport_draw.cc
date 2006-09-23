/*
 * Copyright (C) 2002-2004, 2006 by the Widelands Development Team
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

#include "editor_game_base.h"
#include "rendertarget.h"
#include "transport.h"

void Flag::draw
(const Editor_Game_Base & game,
 RenderTarget & dst,
 const FCoords coords,
 const Point pos)
{
	static struct { int x, y; } ware_offsets[8] = {
		{ -5,  1 },
		{ -1,  3 },
		{  3,  3 },
		{  7,  1 },
		{ -6, -3 },
		{ -1, -2 },
		{  3, -2 },
		{  8, -3 }
	};
	
	dst.drawanim
		(pos.x, pos.y, m_anim, game.get_gametime() - m_animstart, get_owner());
	
	const uint item_filled = m_item_filled;
	for (uint i = 0; i < item_filled; ++i) {// draw wares
		Point warepos = pos;
		if (i < 8) {
			warepos.x += ware_offsets[i].x;
			warepos.y += ware_offsets[i].y;
		} else
			warepos.y -= 6 + (i - 8) * 3;
		dst.drawanim
			(warepos.x, warepos.y,
			 m_items[i].item->get_ware_descr()->get_animation("idle"),
			 0,
			 get_owner());
	}
}

/** The road is drawn by the terrain renderer via marked fields. */
void Road::draw
(const Editor_Game_Base &, RenderTarget &, const FCoords, const Point)
{}


