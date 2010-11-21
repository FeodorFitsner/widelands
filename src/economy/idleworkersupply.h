/*
 * Copyright (C) 2002-2004, 2006-2008 by the Widelands Development Team
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

#ifndef IDLEWORKERSUPPLY_H
#define IDLEWORKERSUPPLY_H

#include "supply.h"

#ifdef _MSC_VER
#define __attribute__(x)
#endif

namespace Widelands {
class Worker;
struct Economy;

struct IdleWorkerSupply : public Supply {
	IdleWorkerSupply(Worker &);
	~IdleWorkerSupply();

	void set_economy(Economy *);
	virtual PlayerImmovable * get_position(Game &);

	virtual bool is_active() const throw ();
	virtual bool has_storage() const throw ();
	virtual void get_ware_type(bool & isworker, Ware_Index & ware) const;
	virtual void send_to_storage(Game &, Warehouse * wh);

	virtual uint32_t nr_supplies(Game const &, Request const &) const;
	virtual WareInstance & launch_item(Game &, Request const &)
		__attribute__ ((noreturn));
	virtual Worker & launch_worker(Game &, Request const &);

private:
	Worker  & m_worker;
	Economy * m_economy;
};

}

#endif
