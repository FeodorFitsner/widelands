/*
 * Copyright (C) 2008-2009 by the Widelands Development Team
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

#include "findimmovable.h"

#include "attackable.h"
#include "immovable.h"
#include "militarysite.h"
#include "upcast.h"

namespace Widelands {

struct FindImmovableAlwaysTrueImpl {
	bool accept(const BaseImmovable &) const {return true;}
};

const FindImmovable & FindImmovableAlwaysTrue()
{
	static FindImmovable alwaystrue = FindImmovableAlwaysTrueImpl();
	return alwaystrue;
}

bool FindImmovableSize              ::accept(const BaseImmovable & imm) const {
	int32_t const size = imm.get_size();
	return m_min <= size && size <= m_max;
}

bool FindImmovableType              ::accept(const BaseImmovable & imm) const {
	return m_type == imm.get_type();
}

bool FindImmovableAttribute         ::accept(const BaseImmovable & imm) const {
	return imm.has_attribute(m_attrib);
}

bool FindImmovablePlayerImmovable   ::accept(const BaseImmovable & imm) const {
	return dynamic_cast<PlayerImmovable const *>(&imm);
}

bool FindImmovablePlayerMilitarySite::accept(const BaseImmovable & imm) const {
	if (upcast(MilitarySite const, ms, &imm))
		return &ms->owner() == &player;
	return false;
}

bool FindImmovableAttackable        ::accept(const BaseImmovable & imm) const {
	return dynamic_cast<Attackable const *>(&imm);
}

bool FindImmovableByDescr::accept(const BaseImmovable & baseimm) const {
	if (upcast(const Immovable, imm, &baseimm)) {
		if (&imm->descr() == &descr)
			return true;
	}
	return false;
}

}
