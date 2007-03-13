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

#include <assert.h>
#include <errno.h>
#include "disk_filesystem.h"
#include <sys/stat.h>
#include "wexception.h"
#include "zip_filesystem.h"

#ifdef __WIN32__
#include "windows.h"
#else
#include <glob.h>
#endif

/**
 * Initialize the real file-system
 */
RealFSImpl::RealFSImpl(const std::string Directory)
		: m_directory(Directory)
{
	// TODO: check OS permissions on whether the directory is writable!
#ifdef __WIN32__
	m_root=Directory;
#else
	m_root = "";
	m_root=FS_CanonicalizeName( Directory );
#endif
}

/**
 * Cleanup code
 */
RealFSImpl::~RealFSImpl()
{
}

void RealFSImpl::listSubdirs() const
{
	printf("%s\n", m_directory.c_str());
}

/** RealFSImpl::IsWritable()
 *
 * Return true if this directory is writable.
 */
const bool RealFSImpl::IsWritable() const
{
	return true; // should be checked in constructor

	//fweber: no, should be checked here, because the ondisk state might have
	//changed since then
}

/**
 * Returns the number of files found, and stores the filenames (without the
 * pathname) in the results. There doesn't seem to be an even remotely
 * cross-platform way of doing this
 */
// note: the Win32 version may be broken, feel free to fix it
const int RealFSImpl::FindFiles(std::string path,
										  const std::string pattern,
										  filenameset_t *results, uint depth)
#ifdef _WIN32
{
	std::string buf;
	struct _finddata_t c_file;
	long hFile;
	int count;

	if (path.size())
		buf = m_directory + '\\' + path + '\\' + pattern;
	else
		buf = m_directory + '\\' + pattern;

	count = 0;

	hFile = _findfirst(buf.c_str(), &c_file);
	if (hFile == -1)
		return 0;

	do {
		results->insert(std::string(path)+'/'+c_file.name);
	} while(_findnext(hFile, &c_file) == 0);

	_findclose(hFile);

	return results->size();
}
#else
{
	std::string buf;
	glob_t gl;
	int i, count;
	int ofs;

	if (path.size()) {
		buf = m_directory + '/' + path + '/' + pattern;
		ofs = m_directory.length()+1;
	} else {
		buf = m_directory + '/' + pattern;
		ofs = m_directory.length()+1;
	}

	if (glob(buf.c_str(), 0, NULL, &gl))
		return 0;

	count = gl.gl_pathc;

	for(i = 0; i < count; i++) {
		results->insert(&gl.gl_pathv[i][ofs]);
	}

	globfree(&gl);

	return count;
}
#endif

/**
 * Returns true if the given file exists, and false if it doesn't.
 * Also returns false if the pathname is invalid (obviously, because the file
 * \e can't exist then)
 * \todo Can this be rewritten to just using exceptions? Should it?
 */
const bool RealFSImpl::FileExists(const std::string path)
{
	struct stat st;

	if (stat(FS_CanonicalizeName(path).c_str(), &st) == -1)
		return false;

	return true;
}

/**
 * Returns true if the given file is a directory, and false if it isn't.
 * Also returns false if the pathname is invalid (obviously, because the file
 * \e can't exist then)
 */
const bool RealFSImpl::IsDirectory(const std::string path)
{
	struct stat st;

	if(!FileExists(path))
		return false;
	if (stat(FS_CanonicalizeName(path).c_str(), &st) == -1)
		return false;

	return S_ISDIR(st.st_mode);
}

/**
 * Create a sub filesystem out of this filesystem
 */
FileSystem* RealFSImpl::MakeSubFileSystem(const std::string path)
{
	assert( FileExists( path )); //TODO: throw an exception instead
	std::string fullname;

	fullname=FS_CanonicalizeName(path);
	//printf("RealFSImpl MakeSubFileSystem path %s fullname %s\n", path.c_str(), fullname.c_str());

	if( IsDirectory( path )) {
		return new RealFSImpl( fullname );
	} else {
		FileSystem* s =  new ZipFilesystem( fullname );
		return s;
	}
}

/**
 * Create a sub filesystem out of this filesystem
 */
FileSystem* RealFSImpl::CreateSubFileSystem(const std::string path,
      const Type fs)
{
	if( FileExists( path ))
		throw wexception( "Path %s already exists. Can't create a filesystem from it!\n", path.c_str());

	std::string fullname;

	fullname=FS_CanonicalizeName(path);

	if( fs == FileSystem::DIR ) {
		EnsureDirectoryExists( path );
		return new RealFSImpl( fullname );
	} else {
		FileSystem* s =  new ZipFilesystem( fullname );
		return s;
	}
}

/**
 * Remove a number of files
 */
void RealFSImpl::Unlink(const std::string file)
{
	if( !FileExists( file ))
		return;

	if(IsDirectory( file ))
		m_unlink_directory( file );
	else
		m_unlink_file( file );
}

/**
 * remove directory or file
 */
void RealFSImpl::m_unlink_file(const std::string file)
{
	assert( FileExists( file ));  //TODO: throw an exception instead
	assert(!IsDirectory( file )); //TODO: throw an exception instead

	std::string fullname;

	fullname=FS_CanonicalizeName(file);

#ifndef __WIN32__
	unlink( fullname.c_str());
#else
	DeleteFile( fullname.c_str() );
#endif
}

void RealFSImpl::m_unlink_directory(const std::string file)
{
	assert( FileExists( file ));
	assert( IsDirectory( file ));

	filenameset_t files;

	FindFiles( file, "*", &files );

	for(filenameset_t::iterator pname = files.begin(); pname != files.end(); pname++) {
		std::string filename = FS_Filename( (*pname).c_str());
		if( filename == ".svn" ) // HACK: ignore SVN directory for this might be a campaign directory or similar
			continue;
		if( filename == ".." )
			continue;
		if( filename == "." )
			continue;

		if( IsDirectory( *pname ) )
			m_unlink_directory( *pname );
		else
			m_unlink_file( *pname );
	}

	// NOTE: this might fail if this directory contains CVS dir,
	// so no error checking here
	std::string fullname;

	fullname=FS_CanonicalizeName(file);
#ifndef __WIN32__
	rmdir( fullname.c_str() );
#else
	RemoveDirectory( fullname.c_str());
#endif
}

/**
 * Create this directory if it doesn't exist, throws an error
 * if the dir can't be created or if a file with this name exists
 */
void RealFSImpl::EnsureDirectoryExists(const std::string dirname)
{
	if(FileExists(dirname)) {
		if(IsDirectory(dirname)) return; // ok, dir is already there
	}
	MakeDirectory(dirname);
}

/**
 * Create this directory, throw an error if it already exists or
 * if a file is in the way or if the creation fails.
 *
 * Pleas note, this function does not honor parents,
 * MakeDirectory("onedir/otherdir/onemoredir") will fail
 * if either ondir or otherdir is missing
 */
void RealFSImpl::MakeDirectory(const std::string dirname)
{
	if(FileExists(dirname))
		throw wexception("A File with the name %s already exists\n", dirname.c_str());

	std::string fullname;

	fullname=FS_CanonicalizeName(dirname);

	int retval=0;
#ifdef WIN32
	retval=mkdir(fullname.c_str());
#else
	retval=mkdir(fullname.c_str(), 0x1FF);
#endif
	if(retval==-1)
		throw wexception("Couldn't create directory %s: %s\n", dirname.c_str(), strerror(errno));
}

/**
 * Read the given file into alloced memory; called by FileRead::Open.
 * Throws an exception if the file couldn't be opened.
 */
void *RealFSImpl::Load(const std::string fname, int * const length)
{
	std::string fullname;
	FILE *file=0;
	void *data=0;
	int size;

	fullname =FS_CanonicalizeName(fname);

	file = 0;
	data = 0;

	try
	{
		//debug info
		//printf("------------------------------------------\n");
		//printf("RealFSImpl::Load():\n");
		//printf("     fname       = %s\n", fname.c_str());
		//printf("     m_directory = %s\n", m_directory.c_str());
		//printf("     fullname    = %s\n", fullname.c_str());
		//printf("------------------------------------------\n");

		file = fopen(fullname.c_str(), "rb");
		if (!file)
			throw wexception("Couldn't open %s (%s)", fname.c_str(), fullname.c_str());

		// determine the size of the file (rather quirky, but it doesn't require
		// potentially unportable functions)
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);

		// allocate a buffer and read the entire file into it
		data = malloc(size + 1); //  FIXME memory leak!
		if (fread(data, size, 1, file) != 1)
			throw wexception("Read failed for %s (%s)", fname.c_str(), fullname.c_str());
		((char *)data)[size] = 0;

		fclose(file);
		file = 0;
	}
	catch(...)
	{
		if (file)
			fclose(file);
		if (data) {
			free(data);
			data = 0;
		}
		throw;
	}

	if (length)
		*length = size;

	return data;
}

/**
 * Write the given block of memory to the repository.
 * Throws an exception if it fails.
 */
void RealFSImpl::Write(const std::string fname, const void * const data,
                       const int length)
{
	std::string fullname;
	FILE *f;
	int c;

	fullname=FS_CanonicalizeName(fname);

	f = fopen(fullname.c_str(), "wb");
	if (!f)
		throw wexception("Couldn't open %s (%s) for writing", fname.c_str(), fullname.c_str());

	c = fwrite(data, length, 1, f);
	fclose(f);

	if (c != 1)
		throw wexception("Write to %s (%s) failed", fname.c_str(), fullname.c_str());
}
