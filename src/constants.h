/*
 * Copyright (C) 2002-2003, 2006-2011 by the Widelands Development Team
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

#ifndef CONSTANTS_H
#define CONSTANTS_H

/**
 * \file constants.h
 * \brief Global compile time configuration and important constants
 *
 * Changes have wide impact on recompile time.
 * Lots more are scattered through header files
 */

/// \name Textures
/// Textures have a fixed size and are squares.
/// TEXTURE_HEIGHT is just defined for easier understanding of the code.
//@{
#define TEXTURE_WIDTH 64
#define TEXTURE_HEIGHT TEXTURE_WIDTH
//@}

#define XRES 800 ///< Fullscreen Menu Width
#define YRES 600 ///< Fullscreen Menu Height

/// \name Fonts
/// Font constants, defined including size
//@{
#define UI_FONT_NAME_SERIF      "DejaVuSerif.ttf"
#define UI_FONT_NAME_SANS       "DejaVuSans.ttf"
#define UI_FONT_NAME_WIDELANDS  "Widelands/Widelands.ttf"

#define UI_FONT_NAME            UI_FONT_NAME_SERIF
#define UI_FONT_NAME_NO_EXT     "DejaVuSerif"
#define UI_FONT_SIZE_BIG        22
#define UI_FONT_SIZE_SMALL      14
#define UI_FONT_SIZE_ULTRASMALL 10

#define UI_FONT_BIG             UI_FONT_NAME, UI_FONT_SIZE_BIG
#define UI_FONT_SMALL           UI_FONT_NAME, UI_FONT_SIZE_SMALL
#define UI_FONT_ULTRASMALL      UI_FONT_NAME, UI_FONT_SIZE_ULTRASMALL

#define UI_FONT_TOOLTIP         UI_FONT_SMALL
#define PROSA_FONT              UI_FONT_NAME_SERIF, 18
//@}

/// \name Font colors
/// A background color is not explicitly defined
//@{

/// Global UI font color
#define UI_FONT_CLR_FG       RGBColor(255, 255,   0)
#define UI_FONT_CLR_BG       RGBColor(107,  87,  55)
#define UI_FONT_CLR_DISABLED RGBColor(127, 127, 127)
#define UI_FONT_CLR_WARNING  RGBColor(255,  22,  22)

/// Prosa font color
#define PROSA_FONT_CLR_FG    RGBColor(255, 255,   0)

/// Colors for good/ok/bad
#define UI_FONT_CLR_BAD_HEX   "ff0000"
#define UI_FONT_CLR_OK_HEX   "ffff00"
#define UI_FONT_CLR_GOOD_HEX   "325b1f"
//@}

/** \name Text colors
 * User interface text color constants
 *
 * Defined as "\<fontcolor\>, \<background color\>".
 * The background colors are chosen to match the user interface
 * backgrounds.
 */
//@{
#define UI_FONT_BIG_CLR     UI_FONT_CLR_FG, UI_FONT_CLR_BG
/// small is used for ultrasmall, too
#define UI_FONT_SMALL_CLR   UI_FONT_CLR_FG, UI_FONT_CLR_BG
#define UI_FONT_TOOLTIP_CLR RGBColor(255, 255, 0)
//@}

/// the actual game logic doesn't know about frames
/// (it works with millisecond-precise timing)
/// FRAME_LENGTH is just the default animation speed
#define FRAME_LENGTH 250

/// Networking
//@{
#define WIDELANDS_LAN_DISCOVERY_PORT 7394
#define WIDELANDS_LAN_PROMOTION_PORT 7395
#define WIDELANDS_PORT               7396
//@}

/// Constants for user-defined SDL events that get handled by SDL's mainloop
//@{
enum {
	CHANGE_MUSIC
};
//@}

/**
 * C++ is really bad at integer types. For example this constant is not
 * recognized as a valid value of type Workarea_Info::size_type without a cast.
 */
#define NUMBER_OF_WORKAREA_PICS static_cast<Workarea_Info::size_type>(3)

#endif
