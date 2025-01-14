/*
 * Copyright (C) 2006-2007 by the Widelands Development Team
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

#ifndef WL_IO_FILESYSTEM_FILESYSTEM_EXCEPTIONS_H
#define WL_IO_FILESYSTEM_FILESYSTEM_EXCEPTIONS_H

#include <stdexcept>
#include <string>

/**
 * Generic problem when dealing with a file or directory
 */
struct FileError : public std::runtime_error {
	explicit FileError
		(const std::string & thrower,
		 const std::string & filename,
		 const std::string & message = "problem with file/directory")

		:
		std::runtime_error(thrower + ": " + message + ": " + filename),
		m_thrower         (thrower),
		m_filename        (filename),
		m_message         (message)
	{}

	std::string m_thrower;
	std::string m_filename;
	std::string m_message;
};

/**
 * A file/directory could not be found. Either it really does not exist or there
 * are problems with the path, e.g. loops or nonexistent path components
 */
struct FileNotFoundError : public FileError {
	explicit FileNotFoundError
		(const std::string & thrower,
		 const std::string & filename,
		 const std::string & message = "could not find file or directory")

		: FileError(thrower, filename, message)
	{}
};

/**
 * The file/directory is of an unexpected type. Reasons can be given via message
 */
struct FileTypeError : public FileError {
	explicit FileTypeError
		(const std::string & thrower,
		 const std::string & filename,
		 const std::string & message = "file or directory has wrong type")

		: FileError(thrower, filename, message)
	{}
};

/**
 * The operating system denied access to the file/directory in question
 */
struct FileAccessDeniedError : public FileError {
	explicit FileAccessDeniedError
		(const std::string & thrower,
		 const std::string & filename,
		 const std::string & message = "access denied on file or directory")

		: FileError(thrower, filename, message)
	{}
};

/**
 * The directory cannot be created
 */

struct DirectoryCannotCreateError : public FileError {
	explicit DirectoryCannotCreateError
		(const std::string & thrower,
		 const std::string & dirname,
		 const std::string & message = "cannot create directory")

		: FileError(thrower, dirname, message)
	{}
};
#endif  // end of include guard: WL_IO_FILESYSTEM_FILESYSTEM_EXCEPTIONS_H
