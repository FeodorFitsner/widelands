/*
 * Copyright (C) 2007 by the Widelands Development Team
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

#include "streamread.h"
#include "wexception.h"

StreamRead::~StreamRead()
{
}

/**
 * Read a number of bytes from the stream.
 *
 * If the requested number of bytes couldn't be read, this function
 * fails by throwing an exception.
 */
void StreamRead::DataComplete(void* const data, const size_t size)
{
	size_t read = Data(data, size);

	if (read != size)
		throw wexception("Stream ended unexpectedly (%u bytes read, %u expected)", read, size);
}

Sint8 StreamRead::Signed8()
{
	Sint8 x;

	DataComplete(&x, 1);

	return x;
}

Uint8 StreamRead::Unsigned8()
{
	Uint8 x;

	DataComplete(&x, 1);

	return x;
}

Sint16 StreamRead::Signed16()
{
	Sint16 x;

	DataComplete(&x, 2);

	return Little16(x);
}

Uint16 StreamRead::Unsigned16()
{
	Uint16 x;

	DataComplete(&x, 2);

	return Little16(x);
}

Sint32 StreamRead::Signed32()
{
	Sint32 x;

	DataComplete(&x, 4);

	return Little32(x);
}

Uint32 StreamRead::Unsigned32()
{
	Uint32 x;

	DataComplete(&x, 4);

	return Little32(x);
}

std::string StreamRead::String()
{
	std::string x;
	char ch;

	for (;;) {
		DataComplete(&ch, 1);

		if (ch == 0)
			break;

		x += ch;
	}

	return x;
}

