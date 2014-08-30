/*
 * Copyright (C) 2006-2014 by the Widelands Development Team
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

#include "logic/terrain_affinity.h"

#include <vector>

#include "logic/description_maintainer.h"
#include "logic/field.h"
#include "logic/map.h"
#include "logic/widelands_geometry.h"
#include "logic/world/terrain_description.h"
#include "scripting/lua_table.h"

namespace Widelands {

namespace  {

constexpr double pow2(const double& a) {
	return a * a;
}

}  // namespace

TerrainAffinity::TerrainAffinity(const LuaTable& table, const std::string& immovable_name)
   : preferred_fertility_(table.get_double("preferred_fertility")),
     preferred_humidity_(table.get_double("preferred_humidity")),
     preferred_temperature_(table.get_double("preferred_temperature")),
     pickiness_(table.get_double("pickiness")) {
	if (!(0 <= preferred_fertility_ && preferred_fertility_ <= 1.)) {
		throw game_data_error("%s: preferred_fertility is not in [0, 1].", immovable_name.c_str());
	}
	if (!(0 <= preferred_humidity_ && preferred_humidity_ <= 1.)) {
		throw game_data_error("%s: preferred_humidity is not in [0, 1].", immovable_name.c_str());
	}
	if (!(0 <= pickiness_ && pickiness_ <= 1.)) {
		throw game_data_error("%s: pickiness is not in [0, 1].", immovable_name.c_str());
	}
	if (preferred_temperature_ < 0) {
		throw game_data_error("%s: preferred_temperature is not in Kelvin.", immovable_name.c_str());
	}
}

double TerrainAffinity::preferred_temperature() const {
	return preferred_temperature_;
}

double TerrainAffinity::preferred_fertility() const {
	return preferred_fertility_;
}

double TerrainAffinity::preferred_humidity() const {
	return preferred_humidity_;
}

double TerrainAffinity::pickiness() const {
	return pickiness_;
}

double probability_to_grow(const TerrainAffinity& affinity,
                           const FCoords& fcoords,
                           const Map& map,
                           const DescriptionMaintainer<TerrainDescription>& terrains) {
	double terrain_humidity = 0;
	double terrain_fertility = 0;
	double terrain_temperature = 0;

	const auto average = [&terrain_humidity, &terrain_fertility, &terrain_temperature, &terrains](
	   const int terrain_index) {
		const TerrainDescription& t = terrains.get_unmutable(terrain_index);
		terrain_humidity += t.humidity() / 6.;
		terrain_temperature += t.temperature() / 6.;
		terrain_fertility += t.fertility() / 6.;
	};

	average(fcoords.field->terrain_d());
	average(fcoords.field->terrain_r());
	{
		FCoords tln;
		map.get_tln(fcoords, &tln);
		average(tln.field->terrain_d());
		average(tln.field->terrain_r());
	}

	{
		FCoords trn;
		map.get_trn(fcoords, &trn);
		average(trn.field->terrain_d());
	}

	{
		FCoords ln;
		map.get_ln(fcoords, &ln);
		average(ln.field->terrain_r());
	}

	const double sigma_fertility = (1. - affinity.pickiness()) * 0.25 + 1e-2;
	const double sigma_humidity = (1. - affinity.pickiness()) * 0.25 + 1e-2;
	const double sigma_temperature = (1. - affinity.pickiness()) * 12.5 + 1e-1;

	const double pure_gauss = exp(
	   -pow2(affinity.preferred_fertility() - terrain_fertility) / (2 * pow2(sigma_fertility)) -
	   pow2(affinity.preferred_humidity() - terrain_humidity) / (2 * pow2(sigma_humidity)) -
	   pow2(affinity.preferred_temperature() - terrain_temperature) / (2 * pow2(sigma_temperature)));

	double advanced_gauss = pure_gauss * 1.1 + 0.05;
	if (advanced_gauss > 0.95)
		advanced_gauss = 0.95;

	return advanced_gauss;
}

}  // namespace Widelands