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

#include "journal.h"

#include "log.h"
#include "io/filesystem/filesystem.h"
#include "machdep.h"

#include <cassert>

/**
 * Write a signed 8bit value to the recording file.
 * \param v The character to be written
 */
void Journal::write(int8_t const v) {
	m_recordstream.write(reinterpret_cast<const char *>(&v), sizeof(v));
}

/// \overload
void Journal::write(uint8_t const v) {
	m_recordstream.write(reinterpret_cast<const char *>(&v), sizeof(v));
}

/// \overload
void Journal::write(int16_t v) {
	v = Little16(v);
	m_recordstream.write(reinterpret_cast<const char *>(&v), sizeof(v));
}
/// \overload
void Journal::write(uint16_t v) {
	v = Little16(v);
	m_recordstream.write(reinterpret_cast<const char *>(&v), sizeof(v));
}

/// \overload
void Journal::write(int32_t v) {
	v = Little32(v);
	m_recordstream.write(reinterpret_cast<const char *>(&v), sizeof(v));
}

/// \overload
void Journal::write(uint32_t v) {
	v = Little32(v);
	m_recordstream.write(reinterpret_cast<const char *>(&v), sizeof(v));
}

/// \overload
/// SDLKey is an enum, and enums are implemented as int32_t. Consequently,
/// SDLKey changes size on 64bit machines :-(
/// So we force it to be 32bit, discarding the higher 32 bits (hopefully no-one
/// will have so many keys).
///
/// On 32bit systems, it does not matter whether this method or
/// \ref write(Uint32 v) actually gets used.
///
/// \sa write(SDLMod v)
/// \sa read(SDLMod &v)
void Journal::write(SDLKey v)
{
	m_recordstream.write(reinterpret_cast<char *>(&v), sizeof(v));
}

/**
 * \overload
 * \sa write(SDLKey v)
 */
void Journal::write(SDLMod v)
{
	m_recordstream.write(reinterpret_cast<char *>(&v), sizeof(v));
}

/**
 * Write a signed char value to the recording file.
 * \param v Reference where the read character will be stored.
 */
void Journal::read (int8_t  & v)
{
	m_playbackstream.read(reinterpret_cast<char *>(&v), sizeof(uint8_t));
}

/**
 * \overload
 */
void Journal::read(uint8_t  & v)
{
	m_playbackstream.read(reinterpret_cast<char *>(&v), sizeof(uint8_t));
}

/**
 * \overload
 */
void Journal::read (int16_t & v) {
	m_playbackstream.read(reinterpret_cast<char *>(&v), sizeof(int16_t));
	v = Little16(v);
}

/**
 * \overload
 */
void Journal::read(uint16_t & v) {
	m_playbackstream.read(reinterpret_cast<char *>(&v), sizeof(uint16_t));
	v = Little16(v);
}

/**
 * \overload
 */
void Journal::read (int32_t & v) {
	m_playbackstream.read(reinterpret_cast<char *>(&v), sizeof(int32_t));
	v = Little32(v);
}

/**
 * \overload
 */
void Journal::read(uint32_t & v) {
	m_playbackstream.read(reinterpret_cast<char *>(&v), sizeof(uint32_t));
	v = Little32(v);
}

/**
 * \overload
 * \sa read(SDLKey v)
 */
void Journal::read(SDLKey & v)
{
	//Look at read(SDLKey v) before changing code here!
	//Additional reminder: SDLKey is an enum which are signed int32_t !

	Uint32 vv;

	m_playbackstream.read(reinterpret_cast<char *>(&vv), sizeof(Uint32));
	v = static_cast<SDLKey>(Little32(vv));
}

/**
 * \overload
 * \sa read(SDLKey v)
 */
void Journal::read(SDLMod & v)
{
	//Look at read(SDLMod v) before changing code here!
	//Additional reminder: SDLKey is an enum which are signed int32_t !

	Uint32 vv;

	m_playbackstream.read(reinterpret_cast<char *>(&vv), sizeof(Uint32));
	v = static_cast<SDLMod>(Little32(vv));
}

/**
 * \todo Document me
 */
void Journal::ensure_code(uint8_t code)
{
	uint8_t filecode;

	read(filecode);
	if (filecode != code) {
		throw BadRecord_error(m_playbackname, filecode, code);
	}
}

/**
 * Standard ctor
 */
Journal::Journal()
:
m_recordname(""), m_playbackname(""),
m_record(false),  m_playback(false)
{
	m_recordstream.exceptions
		(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);

	m_playbackstream.exceptions
		(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);
}

/**
 * Close any open journal files
 */
Journal::~Journal()
{
	stop_recording();
	stop_playback();
}

/**
 * Start recording events handed to us
 * \param filename File the events should be written to
 * \todo set the filename somewhere else
 */
void Journal::start_recording(std::string const & filename)
{
	assert(!m_recordstream.is_open());

	//TODO: m_recordname = FileSystem::FS_CanonicalizeName(filename);
	m_recordname = filename;
	if (m_recordname.empty())
		assert(false); //TODO: barf in a controlled way

	try {
		m_recordstream.open
			(m_recordname.c_str(), std::ios::binary|std::ios::trunc);
		write(RFC_MAGIC);
		m_recordstream << std::flush;
		m_record = true;
		log("Recording into %s\n", m_recordname.c_str());
	}
	catch (std::ofstream::failure e) {
		//TODO: use exception mask to find out what happened
		//TODO: there should be a messagebox to tell the user.
		log
			("Problem while opening record file %s for writing.\n",
			 m_recordname.c_str());
		stop_recording();
		throw Journalfile_error(m_recordname);
	}
}

/**
 * Stop recording events.
 * It's safe to call this even if recording has not been
 * started yet.
 */
void Journal::stop_recording()
{
	m_record = false;

	if (m_recordstream.is_open()) {
		m_recordstream<<std::flush;
		m_recordstream.close();
	}
}

/**
 * Start playing back events
 * \param filename File to get events from
 * \todo set the filename somewhere else
 */
void Journal::start_playback(std::string const & filename)
{
	assert(!m_playbackstream.is_open());

	//TODO: m_playbackname = FileSystem::FS_CanonicalizeName(filename);
	m_playbackname = filename;
	if (m_playbackname.empty())
		assert(false); //TODO: barf in a controlled way

	try {
		uint32_t magic;

		m_playbackstream.open(m_playbackname.c_str(), std::ios::binary);
		read(magic);
		if (magic != RFC_MAGIC)
			throw BadMagic_error(m_playbackname);
		m_playback = true;
		log("Playing back from %s\n", m_playbackname.c_str());
	}
	catch (std::ifstream::failure e) {
		//TODO: use exception mask to find out what happened
		//TODO: there should be a messagebox to tell the user.
		log
			("ERROR: problem while opening playback file for reading. Playback "
			 "deactivated.\n");
		stop_playback();
		throw Journalfile_error(m_recordname);
	}
}

/**
 * Stop playing back events.
 * It's safe to call this even if playback has not been
 * started yet.
 */
void Journal::stop_playback()
{
	m_playback = false;

	if (m_playbackstream.is_open()) {
		m_playbackstream.close();
	}
}

/**
 * Record an event into the playback file. This entails serializing the
 * event and writing it out.
 *
 * \param e The event to be recorded
 */
void Journal::record_event(SDL_Event const & e)
{
	if (!m_record)
		return;

	try {
		//Note: the following lines are *inside* the switch on purpose:
		//   write(RFC_EVENT);
		//   m_recordstream<<std::flush;
		//If they were outside, they'd get executed on every mainloop
		//iteration, which would yield a) huge files and b) lots of
		//completely unnecessary overhad.
		switch (e.type) {
		case SDL_KEYDOWN:
			write(static_cast<uint8_t>(RFC_EVENT));
			write(static_cast<uint8_t>(RFC_KEYDOWN));
			write(e.key.keysym.mod);
			write(e.key.keysym.sym);
			write(e.key.keysym.unicode);
			m_recordstream << std::flush;
			break;
		case SDL_KEYUP:
			write(static_cast<uint8_t>(RFC_EVENT));
			write(static_cast<uint8_t>(RFC_KEYUP));
			write(e.key.keysym.mod);
			write(e.key.keysym.sym);
			write(e.key.keysym.unicode);
			m_recordstream << std::flush;
			break;
		case SDL_MOUSEBUTTONDOWN:
			write(static_cast<uint8_t>(RFC_EVENT));
			write(static_cast<uint8_t>(RFC_MOUSEBUTTONDOWN));
			write(e.button.button);
			write(e.button.x);
			write(e.button.y);
			write(e.button.state);
			m_recordstream << std::flush;
			break;
		case SDL_MOUSEBUTTONUP:
			write(static_cast<uint8_t>(RFC_EVENT));
			write(static_cast<uint8_t>(RFC_MOUSEBUTTONUP));
			write(e.button.button);
			write(e.button.x);
			write(e.button.y);
			write(e.button.state);
			m_recordstream << std::flush;
			break;
		case SDL_MOUSEMOTION:
			write(static_cast<uint8_t>(RFC_EVENT));
			write(static_cast<uint8_t>(RFC_MOUSEMOTION));
			write(e.motion.state);
			write(e.motion.x);
			write(e.motion.y);
			write(e.motion.xrel);
			write(e.motion.yrel);
			m_recordstream << std::flush;
			break;
		case SDL_QUIT:
			write(static_cast<uint8_t>(RFC_EVENT));
			write(static_cast<uint8_t>(RFC_QUIT));
			m_recordstream << std::flush;
			break;
		default:
			// can't really do anything useful with this event
			break;
		}
	}
	catch (std::ofstream::failure const &) {
		//TODO: use exception mask to find out what happened
		//TODO: there should be a messagebox to tell the user.
		log("Failed to write to record file. Recording deactivated.\n");
		stop_recording();
		throw Journalfile_error(m_recordname);
	}
}

/**
 * Get an event from the playback file. This entails creating an empty
 * event with sensible default values (not all parameters get recorded)
 * and deserializing the event record.
 *
 * \param e The event being returned
 */
bool Journal::read_event(SDL_Event & e)
{
	if (!m_playback)
		return false;

	try {
		bool haveevent = false;

		uint8_t recordtype;
		read(recordtype);
		switch (recordtype) {
		case RFC_EVENT:
			uint8_t eventtype;
			read(eventtype);
			switch (eventtype) {
			case RFC_KEYDOWN:
				e.type = SDL_KEYDOWN;
				read(e.key.keysym.mod);
				read(e.key.keysym.sym);
				read(e.key.keysym.unicode);
				break;
			case RFC_KEYUP:
				e.type = SDL_KEYUP;
				read(e.key.keysym.mod);
				read(e.key.keysym.sym);
				read(e.key.keysym.unicode);
				break;
			case RFC_MOUSEBUTTONDOWN:
				e.type = SDL_MOUSEBUTTONDOWN;
				read(e.button.button);
				read(e.button.x);
				read(e.button.y);
				read(e.button.state);
				break;
			case RFC_MOUSEBUTTONUP:
				e.type = SDL_MOUSEBUTTONUP;
				read(e.button.button);
				read(e.button.x);
				read(e.button.y);
				read(e.button.state);
				break;
			case RFC_MOUSEMOTION:
				e.type = SDL_MOUSEMOTION;
				read(e.motion.state);
				read(e.motion.x);
				read(e.motion.y);
				read(e.motion.xrel);
				read(e.motion.yrel);
				break;
			case RFC_QUIT:
				e.type = SDL_QUIT;
				break;
			default:
				throw BadEvent_error(m_playbackname, eventtype);
			}

			haveevent = true;
			break;
		case RFC_ENDEVENTS:
			//Do nothing
			break;
		default:
			throw BadRecord_error(m_playbackname, recordtype, RFC_INVALID);
			break;
		}

		return haveevent;
	} catch (std::ifstream::failure const &) {
		//TODO: use exception mask to find out what happened
		//TODO: there should be a messagebox to tell the user.
		log("Failed to read from journal file. Playback deactivated.\n");
		stop_playback();
		throw Journalfile_error(m_playbackname);
	}
}

/**
 * Do the right thing with timestamps.
 * All timestamps handled with \ref WLApplication::get_time() pass through here.
 * If necessary, they will be recorded. On playback, they will be modified to
 * show the recorded time instead of the current time.
 */
void Journal::timestamp_handler(uint32_t & stamp)
{
	if (m_record) {
		write(static_cast<uint8_t>(RFC_GETTIME));
		write(stamp);
	}

	if (m_playback) {
		ensure_code(static_cast<uint8_t>(RFC_GETTIME));
		read(stamp);
	}
}

/**
 * \todo document me
 */
void Journal::set_idle_mark()
{
	write(static_cast<uint8_t>(RFC_ENDEVENTS));
}
