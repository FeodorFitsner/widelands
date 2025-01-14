/*
 * Copyright (C) 2002, 2006-2013 by the Widelands Development Team
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

#include "profile/profile.h"

#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

#include <boost/algorithm/string.hpp>

#include "base/i18n.h"
#include "base/log.h"
#include "base/wexception.h"
#include "build_info.h"
#include "io/fileread.h"
#include "io/filewrite.h"


#define TRUE_WORDS 7
static char const * trueWords[TRUE_WORDS] =
{
	"true",
	/** TRANSLATORS: A variant of the commandline parameter "true" value */
	/** TRANSLATORS: Needs to be consistent with the translations in widelands-console */
	_("true"),
	"yes",
	/** TRANSLATORS: A variant of the commandline parameter "true" value */
	/** TRANSLATORS: Needs to be consistent with the translations in widelands-console */
	_("yes"),
	"on",
	/** TRANSLATORS: A variant of the commandline parameter "true" value */
	/** TRANSLATORS: Needs to be consistent with the translations in widelands-console */
	_("on"),
	"1"
};

#define FALSE_WORDS 7
static char const * falseWords[FALSE_WORDS] =
{
	"false",
	/** TRANSLATORS: A variant of the commandline parameter "false" value */
	/** TRANSLATORS: Needs to be consistent with the translations in widelands-console */
	_("false"),
	"no",
	/** TRANSLATORS: A variant of the commandline parameter "false" value */
	/** TRANSLATORS: Needs to be consistent with the translations in widelands-console */
	_("no"),
	"off",
	/** TRANSLATORS: A variant of the commandline parameter "false" value */
	/** TRANSLATORS: Needs to be consistent with the translations in widelands-console */
	_("off"),
	"0"
};

Profile g_options(Profile::err_log);

Section::Value::Value(const string & nname, const char * const nval) :
	m_used(false),
	m_name(nname)
{
	set_string(nval);
}

Section::Value::Value(const Section::Value & o) :
	m_used(o.m_used),
	m_name(o.m_name)
{
	set_string(o.m_value.get());
}

Section::Value::Value(Section::Value && o)
	: Value()
{
	using std::swap;
	swap(*this, o);
}

Section::Value & Section::Value::operator= (Section::Value other)
{
	using std::swap;
	swap(*this, other);
	return *this;
}

Section::Value & Section::Value::operator= (Section::Value && other)
{
	using std::swap;
	swap(*this, other);
	return *this;
}

bool Section::Value::is_used() const
{
	return m_used;
}

void Section::Value::mark_used()
{
	m_used = true;
}

int32_t Section::Value::get_int() const
{
	char * endp;
	long int const i = strtol(m_value.get(), &endp, 0);
	if (*endp)
		throw wexception("%s: '%s' is not an integer", get_name(), get_string());
	int32_t const result = i;
	if (i != result)
		throw wexception("%s: '%s' is out of range",   get_name(), get_string());

	return result;
}


uint32_t Section::Value::get_natural() const
{
	char * endp;
	long long int i = strtoll(m_value.get(), &endp, 0);
	if (*endp || i < 0)
		throw wexception("%s: '%s' is not natural", get_name(), get_string());
	return i;
}


uint32_t Section::Value::get_positive() const
{
	char * endp;
	long long int i = strtoll(m_value.get(), &endp, 0);
	if (*endp || i < 1)
		throw wexception("%s: '%s' is not positive", get_name(), get_string());
	return i;
}


bool Section::Value::get_bool() const
{
	for (int32_t i = 0; i < TRUE_WORDS; ++i)
		if (boost::iequals(m_value.get(), trueWords[i]))
			return true;
	for (int32_t i = 0; i < FALSE_WORDS; ++i)
		if (boost::iequals(m_value.get(), falseWords[i]))
			return false;

	throw wexception("%s: '%s' is not a boolean value", get_name(), get_string());
}


Point Section::Value::get_point() const
{
	char * endp = m_value.get();
	long int const x = strtol(endp, &endp, 0);
	long int const y = strtol(endp, &endp, 0);
	if (*endp)
		throw wexception("%s: '%s' is not a Point", get_name(), get_string());

	return Point(x, y);
}

void Section::Value::set_string(char const * const value)
{
	using std::copy;

	const auto len = strlen(value) + 1;
	m_value.reset(new char[len]);
	copy(value, value + len, m_value.get());
}

void swap(Section::Value & first, Section::Value & second)
{
	using std::swap;

	swap(first.m_name,  second.m_name);
	swap(first.m_value, second.m_value);
	swap(first.m_used,  second.m_used);
}


/*
==============================================================================

Section

==============================================================================
*/

char const * Section::get_name() const {
	return m_section_name.c_str();
}
void Section::set_name(const std::string& name) {
	m_section_name = name;
}

Section::Section(Profile * const prof, const std::string & name) :
m_profile(prof), m_used(false), m_section_name(name) {}

/** Section::is_used()
 *
 */
bool Section::is_used() const
{
	return m_used;
}

/** Section::mark_used()
 *
 */
void Section::mark_used()
{
	m_used = true;
}

/** Section::check_used()
 *
 * Print a warning for every unused value.
 */
void Section::check_used() const
{
	for (const Value& temp_value : m_values) {
		if (!temp_value.is_used()) {
			m_profile->error
				("Section [%s], key '%s' not used (did you spell the name "
				 "correctly?)",
				 get_name(), temp_value.get_name());
		}
	}
}


bool Section::has_val(char const * const name) const
{
	for (const Value& temp_value : m_values) {
		if (boost::iequals(temp_value.get_name(), name)) {
			return true;
		}
	}
	return false;
}

/**
 * Returns the Value associated with the given keyname.
 *
 * Args: name  name of the key
 *
 * Returns: Pointer to the Value struct; 0 if the key doesn't exist.
 */
Section::Value * Section::get_val(char const * const name)
{
	for (Value& value : m_values) {
		if (boost::iequals(value.get_name(), name)) {
			value.mark_used();
			return &value;
		}
	}
	return nullptr;
}

/**
 * Returns the first unused value associated with the given keyname.
 *
 * Args: name  name of the key; can be 0 to find any key
 *
 * Returns: Pointer to the Value struct; 0 if no more key-value pairs are found
 */
Section::Value * Section::get_next_val(char const * const name)
{
	for (Value& value : m_values) {
		if (!value.is_used()) {
			if (!name || boost::iequals(value.get_name(), name)) {
				value.mark_used();
				return &value;
			}
		}
	}
	return nullptr;
}

Section::Value & Section::create_val
	(char const * const name, char const * const value)
{
	for (Value& temp_value : m_values) {
		if (boost::iequals(temp_value.get_name(), name)) {
			temp_value.set_string(value);
			return temp_value;
		}
	}
	return create_val_duplicate(name, value);
}

Section::Value & Section::create_val_duplicate
	(char const * const name, char const * const value)
{
	m_values.emplace_back(name, value);
	return m_values.back();
}

/**
 * Return the integer value of the given key or throw an exception if a
 * problem arises.
 */
int32_t Section::get_safe_int(char const * const name)
{
	Value * const v = get_val(name);
	if (!v)
		throw wexception("[%s]: missing integer key '%s'", get_name(), name);
	return v->get_int();
}


/// Return the natural value of the given key or throw an exception.
uint32_t Section::get_safe_natural(char const * const name)
{
	if (Value * const v = get_val(name))
		return v->get_natural();
	else
		throw wexception("[%s]: missing natural key '%s'", get_name(), name);
}


/// Return the positive value of the given key or throw an exception.
uint32_t Section::get_safe_positive(char const * const name)
{
	if (Value * const v = get_val(name))
		return v->get_positive();
	else
		throw wexception("[%s]: missing positive key '%s'", get_name(), name);
}


/**
 * Return the boolean value of the given key or throw an exception if a
 * problem arises.
 */
bool Section::get_safe_bool(char const * const name)
{
	Value * const v = get_val(name);
	if (!v)
		throw wexception("[%s]: missing boolean key '%s'", get_name(), name);
	return v->get_bool();
}

/**
 * Return the key value as a plain string or throw an exception if the key
 * doesn't exist
 */
char const * Section::get_safe_string(char const * const name)
{
	Value * const v = get_val(name);
	if (!v)
		throw wexception("[%s]: missing key '%s'", get_name(), name);
	return v->get_string();
}

/**
 * Return the key value as a plain string or throw an exception if the key
 * does not exist.
 */
const char * Section::get_safe_string(const std::string & name)
{
	return get_safe_string(name.c_str());
}

/**
 * Returns the integer value of the given key. Falls back to a default value
 * if the key is not found.
 *
 * Args: name  name of the key
 *       def   fallback value
 *
 * Returns: the integer value of the key
 */
int32_t Section::get_int(char const * const name, int32_t const def)
{
	Value * const v = get_val(name);
	if (!v)
		return def;

	try {
		return v->get_int();
	} catch (const std::exception & e) {
		m_profile->error("%s", e.what());
	}

	return def;
}


uint32_t Section::get_natural(char const * const name, uint32_t const def)
{
	if (Value * const v = get_val(name))
		try {
			return v->get_natural();
		} catch (const std::exception & e) {
			m_profile->error("%s", e.what());
			return def;
		}
	else
		return def;

}


uint32_t Section::get_positive(char const * const name, uint32_t const def)
{
	if (Value * const v = get_val(name)) {
		try {
			return v->get_positive();
		} catch (const std::exception & e) {
			m_profile->error("%s", e.what());
			return def;
		}
	}

	return def;
}


/**
 * Returns the boolean value of the given key. Falls back to a default value
 * if the key is not found.
 *
 * Args: name  name of the key
 *       def   fallback value
 *
 * Returns: the boolean value of the key
 */
bool Section::get_bool(char const * const name, bool const def)
{
	Value * const v = get_val(name);
	if (!v)
		return def;

	try {
		return v->get_bool();
	} catch (const std::exception & e) {
		m_profile->error("%s", e.what());
	}

	return def;
}

/**
 * Returns the value of the given key. Falls back to a default value if the
 * key is not found.
 *
 * Args: name  name of the key
 *       def   fallback value
 *
 * Returns: the string associated with the key; never returns 0 if the key
 *          has been found
 */
char const * Section::get_string
	(char const * const name, char const * const def)
{
	Value const * const v = get_val(name);
	return v ? v->get_string() : def;
}

Point Section::get_point(const char * const name, const Point def)
{
	Value const * const v = get_val(name);
	return v ? v->get_point() : def;
}


/**
 * Retrieve the next unused key with the given name as a boolean value.
 *
 * Args: name   name of the key, can be 0 to find all unused keys
 *       value  value of the key is stored here
 *
 * Returns: the name of the key, or 0 if none has been found
 */
char const * Section::get_next_bool
	(char const * const name, bool * const value)
{
	Value * const v = get_next_val(name);
	if (!v)
		return nullptr;

	if (value)
		*value = v->get_bool();
	return v->get_name();
}


/**
 * Modifies/Creates the given key.
 */
void Section::set_int(char const * const name, int32_t const value)
{
	set_string(name, std::to_string(value));
}


void Section::set_string(char const * const name, char const * string)
{
	create_val(name, string).mark_used();
}

void Section::set_string_duplicate
	(char const * const name, char const * const string)
{
	create_val_duplicate(name, string).mark_used();
}

/*
==============================================================================

Profile

==============================================================================
*/

/** Profile::Profile(int32_t error_level = err_throw)
 *
 * Create an empty profile
 */
Profile::Profile(int32_t error_level)
{
	m_error_level = error_level;
}

/**
 * Parses an ini-style file into sections and key-value pairs.
 * If global_section is not null, keys outside of sections are put into a
 * section
 * of that name.
 */
Profile::Profile
	(char const * const filename,
	 char const * const global_section,
	 int32_t      const error_level)
	: m_filename(filename), m_error_level(error_level)
{
	read(filename, global_section);
}

Profile::Profile
	(char const * const filename,
	 char const * const global_section,
	 const std::string & textdomain,
	 int32_t      const error_level)
	: m_filename(filename), m_error_level(error_level)
{
	i18n::Textdomain td(textdomain);
	read(filename, global_section);
}

/**
 * Output an error message.
 * Depending on the error level, it is thrown as a wexception, logged or
 * ignored.
 */
void Profile::error(char const * const fmt, ...) const
{
	if (m_error_level == err_ignore)
		return;

	char buffer[256];
	va_list va;

	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	if (m_error_level == err_log)
		log("[%s] %s\n", m_filename.c_str(), buffer);
	else
		throw wexception("[%s] %s", m_filename.c_str(), buffer);
}

/** Profile::check_used()
 *
 * Signal an error if a section or key hasn't been used.
 */
void Profile::check_used() const
{
	for (const Section& temp_section : m_sections) {
		if (!temp_section.is_used()) {
			error
				("Section [%s] not used (did you spell the name correctly?)",
				 temp_section.get_name());
		}
		else {
			temp_section.check_used();
		}
	}
}

/**
 * Retrieve the first section of the given name and mark it used.
 *
 * @param name name of the section
 *
 * @return pointer to the section (or 0 if the section doesn't exist)
 */
Section * Profile::get_section(const std::string & name)
{
	for (Section& temp_section : m_sections) {
		if (boost::iequals(temp_section.get_name(), name.c_str())) {
			temp_section.mark_used();
			return &temp_section;
		}
	}
	return nullptr;
}

/**
 * Safely get a section of the given name.
 * If the section doesn't exist, an exception is thrown.
 */
Section & Profile::get_safe_section(const std::string & name)
{
	if (Section * const s = get_section(name.c_str()))
		return *s;
	else
		throw wexception
			("in \"%s\" section [%s] not found", m_filename.c_str(), name.c_str());
}

/**
 * Safely get a section of the given name.
 * If the section doesn't exist, it is created.
 * Similar to create_section(), but the section is marked as used.
 */
Section & Profile::pull_section(char const * const name)
{
	Section & s = create_section(name);
	s.mark_used();
	return s;
}

/**
 * Retrieve the next unused section of the given name and mark it used.
 *
 * Args: name  name of the section; can be 0 to retrieve any unused section
 *
 * Returns: pointer to the section (or 0 if the section doesn't exist)
 */
Section * Profile::get_next_section(char const * const name)
{
	for (Section& section : m_sections) {
		if (!section.is_used()) {
			if (!name || boost::iequals(section.get_name(), name)) {
				section.mark_used();
				return &section;
			}
		}
	}
	return nullptr;
}


Section & Profile::create_section          (char const * const name)
{
	for (Section& section : m_sections) {
		if (boost::iequals(section.get_name(), name)) {
			return section;
		}
	}
	return create_section_duplicate(name);
}


Section & Profile::create_section_duplicate(char const * const name)
{
	m_sections.push_back(Section(this, name));
	return m_sections.back();
}


inline char * skipwhite(char * p)
{
	while (*p && isspace(*p))
		++p;
	return p;
}

inline void rtrim(char * const str)
{
	for (char * p = strchr(str, '\0'); str < p; --p)
		if (!isspace(p[-1])) {
			*p = 0;
			break;
		}
}

inline void killcomments(char * p)
{
	while (*p) {
		if (p[0] == '#') {
			p[0] = '\0';
			break;
		}
		++p;
	}
}


/**
 * Parses an ini-style file into sections and key values. If a section or
 * key name occurs multiple times, an additional entry is created.
 *
 * Args: filename  name of the source file
 */
void Profile::read
	(char const * const filename,
	 char const * const global_section,
	 FileSystem & fs)
{
	uint32_t linenr = 0;
	try {
		FileRead fr;
		fr.open(fs, filename);

		char    * p = nullptr;
		Section * s = nullptr;

		bool reading_multiline = 0;
		std::string data;
		char * key = nullptr;
		bool translate_line = false;
		while (char * line = fr.read_line()) {
			++linenr;

			if (!reading_multiline)
				p = line;

			p = skipwhite(p);
			if (!p[0] || p[0] == '#')
				continue;

			if (p[0] == '[') {
				++p;
				if (char * const closing = strchr(p, ']'))
					*closing = '\0';
				else
					throw wexception("missing ']' after \"%s\"", p);
				s = &create_section_duplicate(p);
			} else {
				char * tail = nullptr;
				translate_line = false;
				if (reading_multiline) {
					// Note: comments are killed by walking backwards into the string
					rtrim(p);
					while (*line != '\'' && *line != '"') {
						if (*line == 0)
							throw wexception("runaway multiline string");
						if (*line == '_')
							translate_line = true;
						++line;
					}

					// skip " or '
					++line;

					for
						(char * eot = line + strlen(line) - 1;
						 *eot != '"' && *eot != '\'';
						 --eot)
						*eot = 0;
					// NOTE: we leave the last '"' and do not remove them
					tail = line;
				} else {
					tail = strchr(p, '=');
					if (!tail)
						throw wexception("invalid syntax: %s", line);
					*tail++ = '\0';
					key = p;
					if (*tail == '_') {
						tail += 1; // skip =_, which is only used for translations
						translate_line = true;
					}
					tail = skipwhite(tail);
					killcomments(tail);
					rtrim(tail);
					rtrim(p);

					// first, check for multiline string
					if
						((tail[0] == '\'' || tail[0] == '"')
						 &&
						 (tail[1] == '\'' || tail[1] == '"'))
					{
						reading_multiline = true;
						tail += 2;
					}

					// then remove surrounding '' or ""
					if (tail[0] == '\'' || tail[0] == '"')
						++tail;
				}
				if (tail) {
					char * const eot = tail + strlen(tail) - 1;
					if (*eot == '\'' || *eot == '"') {
						*eot = '\0';
						if (*tail) {
							char * const eot2 = tail + strlen(tail) - 1;
							if (*eot2 == '\'' || *eot2 == '"') {
								reading_multiline = false;
								*eot2 = '\0';
							}
						}
					}

					// ready to insert
					if (!s) {
						if (global_section)
							s = &create_section_duplicate(global_section);
						else
							throw wexception("key %s outside section", p);
					}

					if (translate_line && *tail) {
						data += i18n::translate(tail);
					} else {
						data += tail;
					}
					if (s && ! reading_multiline) {
						s->create_val_duplicate(key, data.c_str());
						data.clear();
					}
				} else
					throw wexception("syntax error");
			}
		}
	}
	catch (const FileNotFoundError &) {
		//It's no problem if the config file does not exist. (It'll get
		//written on exit anyway)
		log("There's no configuration file, using default values.\n");
	}
	catch (const std::exception & e) {
		error("%s:%u: %s", filename, linenr, e.what());
	}

	//  Make sure that the requested global section exists, even if it is empty.
	if (global_section && !get_section(global_section))
		create_section_duplicate(global_section);
}

/**
 * Writes all sections out to the given file.
 * If used_only is true, only used sections and keys are written to the file.
 */
void Profile::write
	(char const * const filename, bool const used_only, FileSystem & fs)
{
	FileWrite fw;

	fw.print_f
		("# Automatically created by Widelands %s (%s)\n",
		 build_id().c_str(), build_type().c_str());

	for (const Section& temp_section : m_sections) {
		if (used_only && !temp_section.is_used())
			continue;

		fw.print_f("\n[%s]\n", temp_section.get_name());

		for (const Section::Value& temp_value : temp_section.m_values) {
			if (used_only && !temp_value.is_used())
				continue;

			char const * const str = temp_value.get_string();

			if (*str) {
				uint32_t spaces = strlen(temp_value.get_name());
				bool multiline = false;

				for (uint32_t i = 0; i < strlen(str); ++i) {
					if (str[i] == '\n') {
						multiline = true;
					}
				}

				// Try to avoid _every_ possible way of
				// getting inconsistent data
				std::string tempstr("");

				if (multiline)
					// Show WL that a multilined text starts
					tempstr += "\"";

				for (char const * it = str; *it; ++it)
					// No speach marks - they would break the format
					switch (*it) {
					case '"':
						tempstr += "''";
						break;
					case '\n':
						// Convert the newlines to WL format.
						tempstr += " \"\n";
						for (uint32_t j = 0; j < spaces + 1; ++j)
							tempstr += ' ';
						tempstr += " \"";
						break;
					default:
						tempstr += *it;
						break;
					}

				if (multiline)
					// End of multilined text.
					tempstr += '"';

				fw.print_f("%s=\"%s\"\n", temp_value.get_name(), tempstr.c_str());
			} else
				fw.print_f("%s=\n", temp_value.get_name());
		}
	}

	fw.write(fs, filename);
}
