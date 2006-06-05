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

#ifndef __S__S2MAP_H
#define __S__S2MAP_H

#include "map_loader.h"

struct S2MapDescrHeader {
		  char   magic[10]; // "WORLD_V1.0"
		  char 	name[20];
		  short 	w;
		  short	h;
		  char 	uses_world; // 0 = green, 1 =black, 2 = winter
		  char	nplayers;
		  char 	author[26];
		  char 	bulk[2290]; // unknown
} /* size 2352 */;

// MILLIONS of Definitions
// Bobs

// TODO: the following bob types appear in S2 maps but are unknown
//  Somebody who can run Settlers II please check them out
//  11 (0x0B)
//  40 (0x28)
//  41 (0x29)

#define BOB_NONE            0x0

#define BOB_STONE1          0x1
#define BOB_STONE2          0x2
#define BOB_STONE3          0x3
#define BOB_STONE4          0x4
#define BOB_STONE5          0x5
#define BOB_STONE6          0x6

#define BOB_SKELETON1        0x7
#define BOB_SKELETON2        0x8
#define BOB_SKELETON3         0x21

#define BOB_STANDING_STONES1	0x18
#define BOB_STANDING_STONES2	0x19
#define BOB_STANDING_STONES3	0x1a
#define BOB_STANDING_STONES4	0x1b
#define BOB_STANDING_STONES5	0x1c
#define BOB_STANDING_STONES6	0x1d
#define BOB_STANDING_STONES7	0x1e

#define BOB_MUSHROOM1		 0x01
#define BOB_MUSHROOM2		 0x22

#define BOB_PEBBLE1          0x2
#define BOB_PEBBLE2          0x3
#define BOB_PEBBLE3          0x4
#define BOB_PEBBLE4         0x25
#define BOB_PEBBLE5         0x26
#define BOB_PEBBLE6         0x27

#define BOB_DEADTREE1          0x5
#define BOB_DEADTREE2         0x6
#define BOB_DEADTREE3         0x20
#define BOB_DEADTREE4         0x1f

#define BOB_CACTUS1          0xC
#define BOB_CACTUS2          0xD

#define BOB_BUSH1           0x11
#define BOB_BUSH2           0x13
#define BOB_BUSH3           0x10
#define BOB_BUSH4           0x12
#define BOB_BUSH5           0xa

// Settlers 2 has 8 types of trees.
// I assume that different animation states are stored in the map file
// to create the following 32 values. I assume that 4 trees are grouped
// together.
// Unfortunately, I can't verify that (can't run the S2 editor).
// In the end, it doesn't matter much anyway.
#define BOB_TREE1           0x70
#define BOB_TREE2           0x71
#define BOB_TREE3           0x72
#define BOB_TREE4           0x73
#define BOB_TREE5           0x74
#define BOB_TREE6           0x75
#define BOB_TREE7           0x76
#define BOB_TREE8           0x77

#define BOB_TREE9           0xB0
#define BOB_TREE10          0xb1
#define BOB_TREE11          0xB2
#define BOB_TREE12          0xb3
#define BOB_TREE13          0xb4
#define BOB_TREE14          0xb5
#define BOB_TREE15          0xB6
#define BOB_TREE16          0xB7

#define BOB_TREE17          0xf0
#define BOB_TREE18          0xf1
#define BOB_TREE19          0xF2
#define BOB_TREE20          0xF3
#define BOB_TREE21          0xf4
#define BOB_TREE22          0xF5
#define BOB_TREE23          0xf6
#define BOB_TREE24          0xf7

#define BOB_TREE25          0x30
#define BOB_TREE26          0x31
#define BOB_TREE27          0x32
#define BOB_TREE28          0x33
#define BOB_TREE29          0x34
#define BOB_TREE30          0x35
#define BOB_TREE31          0x36
#define BOB_TREE32          0x37

#define BOB_GRASS1			0xe
#define BOB_GRASS2			0x14
#define BOB_GRASS3			0xf


class S2_Map_Loader : public Map_Loader {
   public:
      S2_Map_Loader(const char*, Map*);
      virtual ~S2_Map_Loader();

      virtual int get_type(void) { return Map_Loader::S2ML; }
      virtual int preload_map(bool);
      virtual int load_map_complete(Editor_Game_Base*, bool scenario);

   private:
      char  m_filename[256];

      uchar *load_s2mf_section(FileRead *, int width, int height);
      void  load_s2mf_header();
      void  load_s2mf(Editor_Game_Base*);
};


#endif /* __S__S2MAP_H */
