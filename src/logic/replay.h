/*
 * Copyright (C) 2007-2009,2013 by the Widelands Development Team
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

#ifndef WL_LOGIC_REPLAY_H
#define WL_LOGIC_REPLAY_H

/**
 * Allow players to watch previous game in a platform-independent way.
 * Also useful as a debugging aid.
 *
 * A game replay consists of a savegame plus a log-file of subsequent
 * playercommands.
 */

#include <cstring>
#include <string>

#include <stdint.h>

#define REPLAY_DIR "replays"
#define REPLAY_SUFFIX ".wrpl"

struct Md5Checksum;

class StreamRead;
class StreamWrite;

namespace Widelands {
struct Command;
class Game;
class PlayerCommand;

/**
 * Read game replays from disk.
 */
class ReplayReader {
public:
	ReplayReader(Game & game, const std::string & filename);
	~ReplayReader();

	Command * get_next_command(uint32_t time);
	bool end_of_replay();

private:
	StreamRead * m_cmdlog;

	uint32_t m_replaytime;
};

/**
 * Write game replays to disk.
 */
class ReplayWriter {
public:
	ReplayWriter(Game &, const std::string & filename);
	~ReplayWriter();

	void send_player_command(PlayerCommand *);
	void send_sync(const Md5Checksum &);

private:
	Game        & m_game;
	StreamWrite * m_cmdlog;
	std::string m_filename;
};

}

#endif  // end of include guard: WL_LOGIC_REPLAY_H
