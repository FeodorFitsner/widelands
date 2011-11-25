/*
 * Copyright (C) 2004, 2006-2011 by the Widelands Development Team
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

#ifndef ITRANSPORT_COST_CALCULATOR_H
#define ITRANSPORT_COST_CALCULATOR_H

#include <boost/noncopyable.hpp>

#include "logic/widelands_geometry.h"

namespace Widelands {

/**
 * This class provides the interface to get cost and cost estimations
 * for certain transport properties (node->node).
 *
 * At the time of this writing, Map implements all of this functionality
 * but most economy code doesn't need all of maps functionality
 */
struct ITransportCostCalculator : boost::noncopyable {
	virtual ~ITransportCostCalculator() {}

	virtual int32_t calc_cost_estimate(Coords, Coords) const = 0;
};

}

#endif
