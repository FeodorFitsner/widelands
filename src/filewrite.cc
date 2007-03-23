/*
 * Copyright (C) 2006 by the Widelands Development Team
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

#include "filesystem.h"
#include "filewrite.h"

#include <stdarg.h>

FileWrite::FileWrite() : data(0), length(0), maxsize(0), filepos(0) {}

FileWrite::~FileWrite() {if (data) Clear();}

void FileWrite::Clear()
{
	if (data)
		free(data);

	data = 0;
	length = 0;
	maxsize = 0;
	filepos = 0;
}

void FileWrite::Write(FileSystem & fs, const char * const filename)
{
	fs.Write(filename, data, length);

	Clear();
}

void FileWrite::Data(const void *buf, const size_t size, const Pos pos) {
	assert(data || !length);

	Pos i = pos;
	if (pos == NoPos()) {
		i = filepos;
		filepos += size;
	}

	if (i+size > length) {
		if (i+size > maxsize) {
			maxsize += 4096;
			if (i+size > maxsize)
				maxsize = i+size;

			data = realloc(data, maxsize);
		}

		length = i+size;
	}

	memcpy((char*)data + i, buf, size);
}

void FileWrite::Printf(const char *fmt, ...)
{
	char buf[2048];
	va_list va;
	int i;

	va_start(va, fmt);
	i = vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);

	if (i < 0) throw Buffer_Overflow();

	Data(buf, i);
}
