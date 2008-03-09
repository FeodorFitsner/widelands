/*
 * Copyright (C) 2008 by the Widelands Development Team
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

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <algorithm>
#include <vector>

#include "widelands_geometry.h"

namespace Widelands {

class PlayerImmovable;

/*

Sender/Receiver infrastructure

Used by computer players to be notified of events, but could/should be
extended to a nofication system that sends player information.

*/

template<typename T>
class NoteReceiver;

template<typename T>
class NoteSender {
	friend class NoteReceiver<T>;

	typedef std::vector<NoteReceiver<T>*> Links;

public:
	~NoteSender() {
		while (m_links.size())
			m_links[m_links.size()-1]->disconnect(*this);
	}

protected:
	void send(const T& note) {
		for (typename Links::iterator it = m_links.begin(); it != m_links.end(); ++it)
			(*it)->receive(note);
	}

private:
	Links m_links;
};

template<typename T>
class NoteReceiver {
	friend class NoteSender<T>;

	typedef std::vector<NoteSender<T>*> Links;

public:
	virtual ~NoteReceiver() {
		while (m_links.size())
			disconnect(*m_links[m_links.size()-1]);
	}

	void connect(NoteSender<T>& sender) {
		if (std::find(m_links.begin(), m_links.end(), &sender) != m_links.end())
			return;
		m_links.push_back(&sender);
		sender.m_links.push_back(this);
	}

	void disconnect(NoteSender<T>& sender) {
		typename NoteSender<T>::Links::iterator oit = std::find(sender.m_links.begin(), sender.m_links.end(), this);
		if (oit != sender.m_links.end())
			sender.m_links.erase(oit);
		typename Links::iterator it = std::find(m_links.begin(), m_links.end(), &sender);
		if (it != m_links.end())
			m_links.erase(it);
	}

	virtual void receive(const T& note) = 0;

private:
	Links m_links;
};


enum losegain_t {LOSE=0, GAIN};

struct NoteImmovable {
	PlayerImmovable* pi;
	losegain_t lg;

	NoteImmovable(PlayerImmovable* _pi, losegain_t _lg)
		: pi(_pi), lg(_lg) {}
};

struct NoteField {
	FCoords fc;
	losegain_t lg;

	NoteField(const FCoords& _fc, losegain_t _lg)
		: fc(_fc), lg(_lg) {}
};

}

#endif // NOTIFICATION_H
