/*
 * Copyright (C) 2002 by Holger Rapp 
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

#ifndef __S__MAPVIEW_H
#define __S__MAPVIEW_H

#include "map.h"
#include "graphic.h"
#include "ui.h"

/* class Map_View
 *
 * this implements a view of a map. it's used
 * to render a valid map on the screen
 *
 * Depends: class Map
 * 			g_gr
 */
class Map_View : public Panel {
public:
	Map_View(Panel *parent, int x, int y, uint w, uint h, Map *m);
	~Map_View();

	// Function to set the viewpoint
	void set_viewpoint(uint,  uint);
	void set_rel_viewpoint(int x, int y) { set_viewpoint(vpx+x,  vpy+y); }

	// Drawing
	void draw(Bitmap *bmp, int ofsx, int ofsx);

	// Event handling
	void handle_mouseclick(uint btn, bool down, uint x, uint y);
	void handle_mousemove(uint x, uint y, int xdiff, int ydiff, uint btns);

private:
	Map* map;
	int vpx, vpy;
	bool dragging;

	void draw_field(Bitmap *, Field*);
	void draw_polygon(Bitmap *, Field*, Field*, Field*, Pic*);
};


#endif /* __S__MAPVIEW_H */
