/*
 * Copyright (C) 2002, 2006, 2008-2010 by the Widelands Development Team
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
// this originally comes from Return to the Shadows (http://www.rtts.org/)
// files.cc: provides all the OS abstraction to access files

#include "filesystem.h"

#ifdef USE_DATAFILE
#include "datafile.h"
#endif
#include "disk_filesystem.h"
#include "layered_filesystem.h"
#include "zip_exceptions.h"
#include "zip_filesystem.h"

#include <config.h>

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <list>

#ifdef WIN32
#include "log.h"
#include <windows.h>
#include <io.h>
#else
#include <glob.h>
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <unistd.h>

#ifdef WIN32
#define stat _stat
#endif

#ifdef _MSC_VER
#define S_ISDIR(x) ((x&_S_IFDIR)?1:0)
#define S_ISREG(x) ((x&_S_IFREG)?1:0)
#endif

FileSystem::FileSystem()
{
	m_root = "";
#ifdef WIN32
	m_filesep = '\\';
#else
	m_filesep = '/';
#endif
}


/**
 * \param path A file or directory name
 * \return True if ref path is absolute, false otherwise
 */
bool FileSystem::pathIsAbsolute(std::string const & path) const {
	std::string::size_type const path_size = path  .size();
	std::string::size_type const root_size = m_root.size();

#ifdef WIN32
	// Handle drive letters on windows
	if (path.size() > 3 && path[1] == ':' && path[2] == '\\')
		return true;
#endif
	if (path_size < root_size)
		return false;

	if (path_size == root_size)
		return path == m_root;

	if (path.compare(0, m_root.size(), m_root))
		return false;

	assert(root_size < path_size); //  Otherwise an invalid read happens below.
	if (path[root_size] != m_filesep)
		return false;

	return true;
}


/**
 * Fix a path that might come from another OS.
 * This function is used to make sure that paths send via network are usable
 * on locale OS.
 */
std::string FileSystem::fixCrossFile(std::string const & path) const {
	uint32_t path_size = path.size();
	std::string fixedPath(path);
	std::string temp;
	for (uint32_t i = 0; i < path_size; ++i) {
		temp = path.at(i);
#ifdef WIN32
		if (temp == "/")
#else
		if (temp == "\\")
#endif
			fixedPath.at(i) = m_filesep;
	}
	return fixedPath;
}

/**
 * \return The process' current working directory
 */
std::string FileSystem::getWorkingDirectory() const {
#ifndef WIN32
	char cwd[PATH_MAX + 1];
	char * const result = getcwd(cwd, PATH_MAX);
	if (! result)
		throw File_error("FileSystem::getWorkingDirectory()", "widelands", "can not run getcwd");

	return std::string(cwd);
#else
	char filename[_MAX_PATH + 1];
	GetModuleFileName(0, filename, _MAX_PATH);
	std::string exedir(filename);
	exedir = exedir.substr(0, exedir.rfind('\\'));
	return exedir;
#endif
}


/// \todo Write homedir detection for non-getenv-systems
std::string FileSystem::GetHomedir()
{
	std::string homedir;
#ifdef WIN32
	// trying to get it compatible to ALL windows versions...
	// Could anybody please hit the Megasoft devs for not keeping
	// their own "standards"?
#define TRY_USE_AS_HOMEDIR(name)                                              \
   homedir = getenv(name);                                                    \
   if (homedir.size() and check_writeable_for_data(homedir.c_str()))          \
      return homedir;                                                         \

	TRY_USE_AS_HOMEDIR("USERPROFILE");
	TRY_USE_AS_HOMEDIR("HOMEPATH");
	TRY_USE_AS_HOMEDIR("HOME");
	TRY_USE_AS_HOMEDIR("APPDATA");

	log("None of the directories was useable - falling back to \".\"\n");
#else
#ifdef HAS_GETENV
	if (char const * const h = getenv("HOME"))
		homedir = h;
#endif
#endif

	if (homedir.empty()) {
		printf
			("\nWARNING: either we can not detect your home directory "
			 "or you do not have one! Please contact the developers.\n\n");

		//TODO: is it really a good idea to set homedir to "." then ??

		printf("Instead of your home directory, '.' will be used.\n\n");
		homedir = ".";
	}

	return homedir;
}

/**
 * Split a string into components separated by a certain character.
 *
 * \param path The path to parse
 * \param filesep The file path separator used by the native filesystem
 * \param components The output iterator to place the path nodes into
 */
template<typename Inserter>
static void FS_Tokenize
	(std::string const & path, char const filesep, Inserter components)
{
	std::string::size_type pos;  //  start of token
	std::string::size_type pos2; //  next filesep character

	//extract the first path component
	if (path.find(filesep) == 0) //is this an absolute path?
		pos = 1;
	else //relative path
		pos = 0;
	pos2 = path.find(filesep, pos);
	//'current' token is now between pos and pos2

	//split path into it's components
	while (pos2 != std::string::npos) {
		if (pos != pos2)
		{
			std::string node = path.substr(pos, pos2 - pos);
			*components++ = node;
		}
		pos = pos2 + 1;
		pos2 = path.find(filesep, pos);
	}

	//extract the last component (most probably a filename)
	std::string node = path.substr(pos);
	if (!node.empty())
		*components++ = node;
}

/**
 * Transform any valid, unique pathname into a well-formed absolute path
 *
 * \todo Enable non-Unix paths
 */
std::string FileSystem::FS_CanonicalizeName(std::string path) const {
	std::list<std::string> components;
	std::list<std::string>::iterator i;

#ifdef WIN32
	// remove all slashes with backslashes so following can work.
	for (uint32_t j = 0; j < path.size(); ++j) {
		if (path[j] == '/')
			path[j] = '\\';
	}
#endif

	FS_Tokenize(path, m_filesep, std::inserter(components, components.begin()));

	//tilde expansion
	if (!components.empty() && *components.begin() == "~") {
		components.erase(components.begin());
		FS_Tokenize
			(GetHomedir(),
			 m_filesep,
			 std::inserter(components, components.begin()));
	} else if (!pathIsAbsolute(path))
		//  make relative paths absolute (so that "../../foo" can work)
		FS_Tokenize
			(m_root.empty() ? getWorkingDirectory() : m_root, m_filesep,
			 std::inserter(components, components.begin()));

	//clean up the path
	for (i = components.begin(); i != components.end();) {
		char const * str = i->c_str();
		if (*str == '.') {
			++str;

			//remove single dot
			if (*str == '\0') {
				i = components.erase(i);
				continue;
			}
			//remove double dot and the preceding component (if any)
			else if (*str == '.' && *(str + 1) == '\0') {
				if (i != components.begin()) {
#ifdef WIN32
					// On windows don't remove driveletter in this error condition
					if (--i != components.begin())
						i = components.erase(i);
					else ++i;
#else
					i = components.erase(--i);
#endif
				}
				i = components.erase(i);
				continue;
			}
		}

		++i;
	}

	std::string canonpath;
	canonpath.reserve(path.length());
#ifndef WIN32
	for (i = components.begin(); i != components.end(); ++i) {
		canonpath.push_back('/');
		canonpath += *i;
	}
#else
	for (i = components.begin(); i != components.end(); ++i) {
		canonpath += *i;
		canonpath += '\\';
	}

	if (canonpath.size()) canonpath.erase(canonpath.end() - 1); //remove trailing slash
#endif

	//debug info
	//printf("canonpath = %s\n", canonpath.c_str());

	return canonpath;
}

/**
 * Returns the filename of this path, everything after the last
 * / or \  (or the whole string)
 */
char const * FileSystem::FS_Filename(char const * p) {
	for (char const * result = p;; ++p)
		if      (*p == '\0')
			return result;
		else if (*p == '/' || *p == '\\')
			result = p + 1;
}

char const * FileSystem::FS_Filename(char const * p, char const * & extension)
{
	extension = 0;
	for (char const * result = p;; ++p)
		if (*p == '\0') {
			if (not extension)
				extension = p;
			return result;
		} else if (*p == '/' || *p == '\\') {
			extension = 0;
			result = p + 1;
		} else if (*p == '.')
			extension = p;
}

std::string FileSystem::FS_FilenameWoExt(char const * const p)
{
	char const * extension;
	std::string fname(p ? FileSystem::FS_Filename(p, extension) : "");
	return
		extension ? fname.substr(0, fname.length() - strlen(extension)) : fname;
}

/// Create a filesystem from a zipfile or a real directory
/// \todo Catch FileType_error in all users
/// \todo Check for existence before doing anything with the file/dir
/// \todo Catch FileNotFound_error in all users
/// \throw FileNotFound_error if root does not exist, is some kind of special
/// file, loops around (via symlinks) or is too long for the OS/filesystem.
/// \throw FileAccessDenied_error if the OS denies access (of course ;-)
/// \throw FileTypeError if root is neither a directory or regular file
/// \todo throw FileTypeError if root is not a zipfile (exception from
/// ZipFilesystem)
FileSystem & FileSystem::Create(std::string const & root)
throw (FileType_error, FileNotFound_error, FileAccessDenied_error)
{
	struct stat statinfo;

	if (stat(root.c_str(), &statinfo) == -1) {
		if
			(errno == EBADF   ||
			 errno == ENOENT  ||
			 errno == ENOTDIR ||
#ifdef ELOOP
			 errno == ELOOP   || //MinGW does not support ELOOP (yet)
#endif
			 errno == ENAMETOOLONG)
		{
			throw FileNotFound_error("FileSystem::Create", root);
		}
		if (errno == EACCES)
			throw FileAccessDenied_error("FileSystem::Create", root);
	}

	if (S_ISDIR(statinfo.st_mode)) {
		return *new RealFSImpl(root);
	}
	if (S_ISREG(statinfo.st_mode)) { //TODO: ensure root is a zipfile
		return *new ZipFilesystem(root);
	}

	throw FileType_error
		("FileSystem::Create", root,
		 "cannot create virtual filesystem from file or directory");
}

#ifdef WIN32
/// hack that is unfortunately needed for windows to check whether Widelands
/// can write in the directory
bool FileSystem::check_writeable_for_data(char const * const path)
{
	RealFSImpl fs(path);
	if (fs.IsDirectory(".widelands"))
		return true;
	try {
		// throws an exception if not writable
		fs.EnsureDirectoryExists(".widelands");
		fs.Unlink(".widelands");
		return true;
	} catch (...) {
		log("Directory %s is not writeable - next try\n", path);
		return false;
	}
}
#endif
