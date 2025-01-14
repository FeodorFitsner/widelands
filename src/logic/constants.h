/*
 * Copyright (C) 2006-2015 by the Widelands Development Team
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

#ifndef WL_LOGIC_CONSTANTS_H
#define WL_LOGIC_CONSTANTS_H

/// Maximum numbers of players in a game. The game logic code reserves 5 bits
/// for player numbers, so it can keep track of 32 different player numbers, of
/// which the value 0 means neutral and the values 1 .. 31 can be used as the
/// numbers for actual players. So the upper limit of this value is 31.
#define MAX_PLAYERS 8

/// How often are statistics to be sampled.
#define STATISTICS_SAMPLE_TIME 30000

#endif  // end of include guard: WL_LOGIC_CONSTANTS_H
