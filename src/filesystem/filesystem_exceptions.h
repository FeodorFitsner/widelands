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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef FILESYSTEM_EXCEPTIONS_H
#define FILESYSTEM_EXCEPTIONS_H

#include <stdexcept>

/**
 * Generic problem when dealing with a file or directory
 */
struct File_error : public std::runtime_error {
	explicit File_error(const std::string & thrower,
	                    const std::string & filename,
	                    const std::string & message="problem with file/directory")
	throw()
			: std::runtime_error(thrower+": "+message+": "+filename),
			m_thrower(thrower),
			m_filename(filename),
			m_message(message)
	{}

	virtual ~File_error() throw() {}

	std::string m_thrower;
	std::string m_filename;
	std::string m_message;
};

/**
 * A file/directory could not be found. Either it really does not exist or there
 * are problems with the path, e.g. loops or nonexistent path components
 */
class FileNotFound_error : public File_error {
public:
	explicit FileNotFound_error(const std::string & thrower,
	                            const std::string & filename,
	                            const std::string & message="could not find file or directory")
	throw()
			:File_error(thrower, filename, message)
	{}
};

/**
 * The file/directory is of an unexpected type. Reasons can be given via message
 */
class FileType_error : public File_error {
public:
	explicit FileType_error(const std::string & thrower,
	                        const std::string & filename,
	                        const std::string & message="file or directory has wrong type")
	throw()
			:File_error(thrower, filename, message)
	{}
};

/**
 * The operating system denied access to the file/directory in question
 */
class FileAccessDenied_error : public File_error {
public:
	explicit FileAccessDenied_error(const std::string & thrower,
	                                const std::string & filename,
	                                const std::string & message="access denied on file or directory")
	throw()
			:File_error(thrower, filename, message)
	{}
};

#endif
