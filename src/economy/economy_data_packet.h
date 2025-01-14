/*
 * Copyright (C) 2004, 2006-2010 by the Widelands Development Team
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

#ifndef WL_ECONOMY_ECONOMY_DATA_PACKET_H
#define WL_ECONOMY_ECONOMY_DATA_PACKET_H

class FileRead;
class FileWrite;

namespace Widelands {
class Economy;
class Game;
class MapObjectLoader;
struct MapObjectSaver;

class EconomyDataPacket {
	public:
		EconomyDataPacket(Economy * e) : m_eco(e) {}

		void read(FileRead &);
		void write(FileWrite &);

	private:
		Economy * m_eco;
};

}

#endif  // end of include guard: WL_ECONOMY_ECONOMY_DATA_PACKET_H
