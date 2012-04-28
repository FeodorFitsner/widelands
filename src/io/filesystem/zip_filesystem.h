/*
 * Copyright (C) 2002-2005, 2007-2009 by the Widelands Development Team
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

#ifndef ZIP_FILESYSTEM_H
#define ZIP_FILESYSTEM_H

#include "filesystem.h"
#include "unzip.h"
#include "zip.h"

#include <string>
#include <cstring>

#ifdef _MSC_VER
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

struct ZipFilesystem : public FileSystem {
	ZipFilesystem(std::string const &);
	virtual ~ZipFilesystem();

	virtual bool IsWritable() const;

	virtual int32_t FindFiles
		(std::string const & path,
		 std::string const & pattern,
		 filenameset_t     * results,
		 uint32_t            depth = 0);

	virtual bool IsDirectory(std::string const & path);
	virtual bool FileExists (std::string const & path);

	virtual void * Load(std::string const & fname, size_t & length);
	virtual void * fastLoad
		(std::string const & fname, size_t & length, bool & fast);

	virtual void Write
		(std::string const & fname, void const * data, int32_t length);
	virtual void EnsureDirectoryExists(std::string const & dirname);
	virtual void   MakeDirectory      (std::string const & dirname);

	virtual StreamRead  * OpenStreamRead
		(const std::string & fname);
	virtual StreamWrite * OpenStreamWrite
		(const std::string & fname);

	virtual FileSystem &   MakeSubFileSystem(std::string const & dirname);
	virtual FileSystem & CreateSubFileSystem
		(std::string const & dirname, Type);
	virtual void Unlink(std::string const & filename);
	virtual void Rename(std::string const &, std::string const &);

	virtual unsigned long long DiskSpace();

public:
	static FileSystem * CreateFromDirectory(std::string const & directory);

	virtual std::string getBasename() {return m_zipfilename;};

private:
	void m_OpenUnzip();
	void m_OpenZip();
	void m_Close();
	std::string strip_basename(std::string);

private:
	enum State {
		STATE_IDLE,
		STATE_ZIPPING,
		STATE_UNZIPPPING
	};

	State       m_state;
	zipFile     m_zipfile;
	unzFile     m_unzipfile;
	/// if true data is in a directory named as the zipfile. This is set by
	/// strip_basename()
	bool        m_oldzip;
	std::string m_zipfilename;
	std::string m_basenamezip;
	std::string m_basename;

};

#endif
