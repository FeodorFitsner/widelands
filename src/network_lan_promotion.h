/*
 * Copyright (C) 2004 by the Widelands Development Team
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

#ifndef __NETWORK_LAN_PROMOTION_H__
#define __NETWORK_LAN_PROMOTION_H__

#include <list>
#include "network_system.h"

#define LAN_PROMOTION_PROTOCOL_VERSION	1

#define LAN_GAME_CLOSED		0
#define LAN_GAME_OPEN		1

struct LAN_Game_Info {
    char		magic[6];
    unsigned char	version;
    unsigned char	state;
    
    char		gameversion[32];
    char		hostname[32];
    char		map[32];
};

struct LAN_Open_Game {
    in_addr_t		address;
    in_port_t		port;
    LAN_Game_Info	info;
};

class LAN_Base {
    protected:
	LAN_Base ();
	~LAN_Base ();
	
	void bind (unsigned short);
	
	bool avail ();
    
	ssize_t recv (void*, size_t, sockaddr_in*);
	
	void send (const void*, size_t, const sockaddr_in*);
	void broadcast (const void*, size_t, unsigned short);
    
    private:
	int			sock;
	
	std::list<in_addr_t>	broadcast_addresses;
};

class LAN_Game_Promoter:LAN_Base {
    public:
	LAN_Game_Promoter ();
	~LAN_Game_Promoter ();
	
	void run ();
	
	void set_map (const char*);
    
    private:
	LAN_Game_Info	gameinfo;
	bool		needupdate;
};

class LAN_Game_Finder:LAN_Base {
    public:
	enum {
	    GameOpened,
	    GameClosed,
	    GameUpdated
	};
	
	LAN_Game_Finder ();
	~LAN_Game_Finder ();
	
	void run ();
	
	void set_callback (void(*)(int, const LAN_Open_Game*, void*), void*);
    
    private:
	std::list<LAN_Open_Game*>	opengames;
	
	void (*callback) (int, const LAN_Open_Game*, void*);
	void*				userdata;
};

#endif

