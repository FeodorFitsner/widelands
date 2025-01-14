/*
 * Copyright (C) 2006-2009 by the Widelands Development Team
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

#ifndef WL_IO_FILESYSTEM_DISK_FILESYSTEM_H
#define WL_IO_FILESYSTEM_DISK_FILESYSTEM_H

#include <cstring>
#include <string>

#include "io/filesystem/filesystem.h"

// TODO(unknown): const correctness
class RealFSImpl : public FileSystem {
public:
	RealFSImpl(const std::string & Directory);

	std::set<std::string> list_directory(const std::string& path) override;

	bool is_writable() const override;
	bool file_is_writeable(const std::string & path);
	bool file_exists (const std::string & path) override;
	bool is_directory(const std::string & path) override;
	void ensure_directory_exists(const std::string & fs_dirname) override;
	void make_directory        (const std::string & fs_dirname) override;

	void * load(const std::string & fname, size_t & length) override;


	void write(const std::string & fname, void const * data, int32_t length, bool append);
	void write(const std::string & fname, void const * data, int32_t length) override
		{write(fname, data, length, false);}

	StreamRead  * open_stream_read (const std::string & fname) override;
	StreamWrite * open_stream_write(const std::string & fname) override;

	FileSystem * make_sub_file_system(const std::string & fs_dirname) override;
	FileSystem * create_sub_file_system(const std::string & fs_dirname, Type) override;
	void fs_unlink(const std::string & file) override;
	void fs_rename(const std::string & old_name, const std::string & new_name) override;

	std::string get_basename() override {return m_directory;}
	unsigned long long disk_space() override;

private:
	void m_unlink_directory(const std::string & file);
	void m_unlink_file     (const std::string & file);

	std::string m_directory;
};

#endif  // end of include guard: WL_IO_FILESYSTEM_DISK_FILESYSTEM_H
