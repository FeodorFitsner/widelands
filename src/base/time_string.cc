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

#include "base/time_string.h"

#include <algorithm>
#include <cassert>
#include <ctime>
#include <string>

#include <boost/format.hpp>
#include <boost/regex.hpp>
#include <stdint.h>

#include "base/i18n.h"

namespace  {
char timestring_buffer[] = "YYYY-MM-DDThh.mm.ss"; //  ':' is not a valid file name character for FAT FS
char gamestringbuffer[] = "000:00:00";
}  // namespace

char * timestring() {
	time_t t;
	time(&t);
	struct tm const * const t_components = localtime(&t);
	{
		uint16_t year = 1900 + t_components->tm_year; //  years start at 1900
		assert(year < 9999);
		timestring_buffer [0] = '0' + year / 1000;
		year %= 1000;
		timestring_buffer [1] = '0' + year /  100;
		year %=  100;
		timestring_buffer [2] = '0' + year /   10;
		year %=   10;
		timestring_buffer [3] = '0' + year;
	}
	{
		uint16_t mon = 1 + t_components->tm_mon; //  months start at 0
		assert(mon  <=   99);
		timestring_buffer [5] = '0' + mon  /   10;
		mon  %=   10;
		timestring_buffer [6] = '0' + mon;
	}
	{
		uint16_t mday = t_components->tm_mday;
		assert(mday <=   99);
		timestring_buffer [8] = '0' + mday /   10;
		mday %=   10;
		timestring_buffer [9] = '0' + mday;
	}
	{
		uint16_t hour = t_components->tm_hour;
		assert(hour <=   99);
		timestring_buffer[11] = '0' + hour /   10;
		hour %=   10;
		timestring_buffer[12] = '0' + hour;
	}
	{
		uint16_t min = t_components->tm_min;
		assert(min  <=   99);
		timestring_buffer[14] = '0' + min  /   10;
		min  %=   10;
		timestring_buffer[15] = '0' + min;
	}
	{
		uint16_t sec = t_components->tm_sec;
		assert(sec  <=   99);
		timestring_buffer[17] = '0' + sec  /   10;
		sec  %=   10;
		timestring_buffer[18] = '0' + sec;
	}
	return timestring_buffer;
}

std::string localize_month(int8_t month) {
	switch (month) {
		case 1:
			/** TRANSLATORS: January. Keep this short if you can. */
			return _("Jan");
		case 2:
			/** TRANSLATORS: February. Keep this short if you can. */
			return _("Feb");
		case 3:
			/** TRANSLATORS: March. Keep this short if you can. */
			return _("Mar");
		case 4:
			/** TRANSLATORS: April. Keep this short if you can. */
			return _("Apr");
		case 5:
			/** TRANSLATORS: May. Keep this short if you can. */
			return _("May");
		case 6:
			/** TRANSLATORS: June. Keep this short if you can. */
			return _("Jun");
		case 7:
			/** TRANSLATORS: July. Keep this short if you can. */
			return _("Jul");
		case 8:
			/** TRANSLATORS: August. Keep this short if you can. */
			return _("Aug");
		case 9:
			/** TRANSLATORS: September. Keep this short if you can. */
			return _("Sep");
		case 10:
			/** TRANSLATORS: October. Keep this short if you can. */
			return _("Oct");
		case 11:
			/** TRANSLATORS: November. Keep this short if you can. */
			return _("Nov");
		case 12:
			/** TRANSLATORS: December. Keep this short if you can. */
			return _("Dec");
		default:
			return std::to_string(month);
	}
}

char * gametimestring_leading_zeros(uint32_t gametime)
{
	uint32_t time = gametime / 1000;
	gamestringbuffer[8] = '0' +  time        % 10;
	gamestringbuffer[7] = '0' + (time /= 10) %  6;
	gamestringbuffer[5] = '0' + (time /=  6) % 10;
	gamestringbuffer[4] = '0' + (time /= 10) %  6;
	gamestringbuffer[2] = '0' + (time /=  6) % 10;
	gamestringbuffer[1] = '0' + (time /= 10) % 10;
	gamestringbuffer[0] = '0' + (time /= 10);
	return gamestringbuffer;
}

std::string gametimestring(uint32_t gametime)
{
	// update buffer
	std::string result = gametimestring_leading_zeros(gametime);

	// remove leading 0s
	int i = 0;
	while (result.at(i) == '0') ++i;
	if (result.at(i) == ':') ++i;

	return result.substr(i);
}
