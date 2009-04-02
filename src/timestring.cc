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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <cassert>
#include <stdint.h>
#include <ctime>

char timestring_buffer[] =
#ifdef WIN32
"YYYY-MM-DDThh.mm.ss"; //  ':' is not a valid file name character under Windows
#else
"YYYY-MM-DDThh:mm:ss"; //  use proper ISO 8601 format on all other systems
#endif

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
