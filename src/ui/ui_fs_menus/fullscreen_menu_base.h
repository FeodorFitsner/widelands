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

#ifndef __S__MENUECOMMON_H
#define __S__MENUECOMMON_H

#include "types.h"
#include "ui_panel.h"

#define MENU_XRES	800   ///< Fullscreen Menu Width
#define MENU_YRES	600   ///< Fullscreen Menu Height

/// Base class for all Fullscreen menus
/**
 * This class is the base class for a fullscreen menu.
 * A fullscreen menu is a menu which takes the full screen; it has the size
 * MENU_XRES and MENU_YRES and is a modal UI Element
 */
class Fullscreen_Menu_Base : public UIPanel {
	uint	m_pic_background;

public:
	Fullscreen_Menu_Base(const char *bgpic);

	virtual void draw(RenderTarget* dst);
};


#endif // __S__MENUECOMMON_H
