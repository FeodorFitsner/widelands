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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef WIDELANDS_GAMEVIEW_H
#define WIDELANDS_GAMEVIEW_H

namespace Widelands {
	struct Player;
	class Editor_Game_Base;
};

class Vertex;
class Texture;

#include "graphic/rendertarget.h"

/**
 * This class does the game rendering. The class is initialized from a
 * RenderTarget and can draw views of the game and the minimap. The game
 * can be rendered to the full screen or to a subwindow. This depends on
 * the RenderTarget from which it was initialized.
 *
 * @todo GameView mixes opengl and software rendering. This should be split
 * properly to have different classes for OpenGL and Software rendering.
 * Perhaps creating a GameRenderManager where different rendering classes can
 * be registered so it is easiely possible to create different game rendering?
 */
class GameView : public RenderTarget
{
public:
	GameView(RenderTarget & rt):
		RenderTarget(rt) {}
	virtual ~GameView() {}

	/**
	 * Renders the map from a player's point of view into the current drawing
	 * window.
	 *
	 * Will call the function below when player.see_all().
	 *
	 * viewofs is the offset of the upper left corner of the window into the map,
	 * in pixels.
	 */
	void rendermap
		(Widelands::Editor_Game_Base const &       egbase,
		 Widelands::Player           const &       player,
		 Point                                     viewofs);

	/**
	 * Same as above but not from a player's point of view. Used in game when
	 * rendering for a player that sees all and the editor.
	 */
	void rendermap
		(Widelands::Editor_Game_Base const & egbase,
		 Point                               viewofs);

	/**
	 * Render the minimap. If player is not 0, it renders from that player's
	 * point of view.
	 */
	void renderminimap
		(Widelands::Editor_Game_Base const & egbase,
		 Widelands::Player           const * player,
		 Point                               viewpoint,
		 uint32_t                            flags);

protected:
	/**
	 * Helper function to draw two terrain triangles. This is called from the
	 * rendermap() functions.
	 */
	virtual void draw_field
		(Rect          & subwin,
		 Vertex  const &  f_vert,
		 Vertex  const &  r_vert,
		 Vertex  const & bl_vert,
		 Vertex  const & br_vert,
		 uint8_t         roads,
		 Texture const & tr_d_texture,
		 Texture const &  l_r_texture,
		 Texture const &  f_d_texture,
		 Texture const &  f_r_texture);

	/// A helper function to draw the minimap. This is called from
	/// renderminimap().
	void draw_minimap
		(Widelands::Editor_Game_Base const &,
		 Widelands::Player           const *,
		 Rect                                rc,
		 Point                               viewpt,
		 uint32_t                            flags);

	/**
	 * This is called before the view of the game is rendered.
	 */
	virtual void rendermap_init() {}

	/**
	* This is called after the view of the game is rendered.
	*/
	virtual void rendermap_deint() {}
};

#endif //WIDELANDS_GAMEVIEW_H
