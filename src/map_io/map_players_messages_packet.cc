/*
 * Copyright (C) 2010-2013 by the Widelands Development Team
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

#include "map_io/map_players_messages_packet.h"

#include "logic/game_data_error.h"
#include "logic/player.h"
#include "map_io/coords_profile.h"
#include "map_io/map_object_loader.h"
#include "map_io/map_object_saver.h"
#include "profile/profile.h"

namespace Widelands {

#define CURRENT_PACKET_VERSION 1
#define PLAYERDIRNAME_TEMPLATE "player/%u"
#define FILENAME_TEMPLATE PLAYERDIRNAME_TEMPLATE "/messages"
#define FILENAME_SIZE 19

void MapPlayersMessagesPacket::Read
	(FileSystem & fs, Editor_Game_Base & egbase, bool, MapObjectLoader & mol)

{
	uint32_t      const gametime   = egbase.get_gametime ();
	const Map   &       map        = egbase.map          ();
	Extent        const extent     = map   .extent       ();
	Player_Number const nr_players = map   .get_nrplayers();
	iterate_players_existing(p, nr_players, egbase, player)
		try {
			char filename[FILENAME_SIZE];
			snprintf(filename, sizeof(filename), FILENAME_TEMPLATE, p);
			Profile prof;
			try {prof.read(filename, nullptr, fs);} catch (...) {continue;}
			prof.get_safe_section("global").get_positive
				("packet_version", CURRENT_PACKET_VERSION);
			MessageQueue & messages = player->messages();

			{
				MessageQueue::const_iterator const begin = messages.begin();
				if (begin != messages.end()) {
					log
						("ERROR: The message queue for player %u contains a message "
						 "before any messages have been loade into it. This is a bug "
						 "in the savegame loading code. It created a new message and "
						 "added it to the queue. This is only allowed during "
						 "simulation, not at load. The following messge will be "
						 "removed when the queue is reset:\n"
						 "\tsender  : %s\n"
						 "\ttitle   : %s\n"
						 "\tsent    : %u\n"
						 "\tduration: %u\n"
						 "\tposition: (%i, %i)\n"
						 "\tstatus  : %u\n"
						 "\tbody    : %s\n",
						 p,
						 begin->second->sender  ().c_str(),
						 begin->second->title   ().c_str(),
						 begin->second->sent    (),
						 begin->second->duration(),
						 begin->second->position().x, begin->second->position().y,
						 begin->second->status  (),
						 begin->second->body    ().c_str());
					messages.clear();
				}
			}

			uint32_t previous_message_sent = 0;
			while (Section * const s = prof.get_next_section())
				try {
					uint32_t const sent    = s->get_safe_int("sent");
					if (sent < previous_message_sent)
						throw GameDataError
							(
							 "messages are not ordered: sent at %u but previous "
							 "message sent at %u",
							 sent, previous_message_sent);
					if (gametime < sent)
						throw GameDataError
							(
							 "message is sent in the future: sent at %u but "
							 "gametime is only %u",
							 sent, gametime);
					uint32_t duration = Forever(); //  default duration
					if (Section::Value const * const dv = s->get_val("duration")) {
						duration = dv->get_positive();
						if (duration == Forever())
							throw GameDataError
								(
								 "the value %u is not allowed as duration; it is "
								 "a special value meaning forever, which is the "
								 "default; omit the duration key to make the "
								 "message exist forever",
								 Forever());
						if (sent + duration < sent)
							throw GameDataError
								(
								 "duration %u is too large; causes numeric "
								 "overflow when added to sent time %u",
								 duration, sent);
						if (sent + duration < gametime)
							throw GameDataError
								(
								 "message should have expired at %u; sent at %u "
								 "with duration %u but gametime is already %u",
								 sent + duration, sent, duration, gametime);
					}
					Message::Status status = Message::Archived; //  default status
					if (char const * const status_string = s->get_string("status")) {
						try {
							if      (!strcmp(status_string, "new"))
								status = Message::New;
							else if (!strcmp(status_string, "read"))
								status = Message::Read;
							else
								throw GameDataError
									("expected %s but found \"%s\"",
									 "{new|read}", status_string);
						} catch (const WException & e) {
							throw GameDataError("status: %s", e.what());
						}
					}
					Serial serial = s->get_int("serial", 0);
					if (serial > 0) {
						assert(mol.is_object_known(serial));
						MapObject & mo = mol.get<MapObject>(serial);
						assert(mol.is_object_loaded(mo));
						serial = mo.serial();
					}

					messages.add_message
						(*new Message
						 	(s->get_string     ("sender", ""),
						 	 sent,
						 	 duration,
						 	 s->get_name       (),
						 	 s->get_safe_string("body"),
							 get_coords("position", extent, Coords::Null(), s),
							 serial,
						 	 status));
					//  Expiration is scheduled for all messages (with finite
					//  duration) after the command queue has been loaded (in
					//  GameLoader::load_game).
					previous_message_sent = sent;
				} catch (const WException & e) {
					throw GameDataError
						("\"%s\": %s", s->get_name(), e.what());
				}
				prof.check_used();
		} catch (const WException & e) {
			throw GameDataError
				("messages for player %u: %s", p, e.what());
		}
}

void MapPlayersMessagesPacket::Write
	(FileSystem & fs, Editor_Game_Base & egbase, MapObjectSaver & mos)
{
	fs.EnsureDirectoryExists("player");
	Player_Number const nr_players = egbase.map().get_nrplayers();
	iterate_players_existing_const(p, nr_players, egbase, player) {
		Profile prof;
		prof.create_section("global").set_int
			("packet_version", CURRENT_PACKET_VERSION);
		const MessageQueue & messages = player->messages();
		Map_Message_Saver & message_saver = mos.message_savers[p - 1];
		for (const std::pair<Message_Id, Message *>& temp_message : messages) {
			message_saver.add         (temp_message.first);
			const Message & message = *temp_message.second;
			assert(message.sent() <= static_cast<uint32_t>(egbase.get_gametime()));
			assert
				(message.duration() == Forever() ||
				 message.sent() < message.sent() + message.duration());
			if
				(message.duration() != Forever() &&
				 message.sent() + message.duration()
				 <
				 static_cast<uint32_t>(egbase.get_gametime()))
				log
					("ERROR: Trying to save a message that should have expired:\n"
					 "\tsent = %u, duration = %u, expiry = %u, gametime = %u\n"
					 "\tsender = \"%s\"\n"
					 "\ttitle: %s\n"
					 "\tbody: %s\n"
					 "\tposition: (%i, %i)\n"
					 "\tstatus: %s\n",
					 message.sent(), message.duration(),
					 message.sent() + message.duration(), egbase.get_gametime(),
					 message.sender().c_str(), message.title().c_str(),
					 message.body().c_str(),
					 message.position().x, message.position().y,
					 message.status() == Message::New      ? "new"      :
					 message.status() == Message::Read     ? "read"     :
					 message.status() == Message::Archived ? "archived" : "ERROR");
			assert
				(message.duration() == Forever() ||
				 static_cast<uint32_t>(egbase.get_gametime())
				 <=
				 message.sent() + message.duration());
			Section & s = prof.create_section_duplicate(message.title().c_str());
			if (message.sender().size())
				s.set_string("sender",    message.sender  ());
			s.set_int      ("sent",      message.sent    ());
			{
				Duration const duration = message.duration();
				if (duration != Forever()) //  The default duration. Do not write.
					s.set_int("duration",  duration);
			}
			s.set_string   ("body",      message.body    ());
			if (Coords const c =         message.position())
				set_coords("position",  c, &s);
			switch (message.status()) {
			case Message::New:
				s.set_string("status",    "new");
				break;
			case Message::Read:
				s.set_string("status",    "read");
				break;
			case Message::Archived: //  The default status. Do not write.
				break;
			default:
				assert(false);
			}
			if (message.serial()) {
				const MapObject* mo = egbase.objects().get_object(message.serial());
				uint32_t fileindex = mos.get_object_file_index_or_zero(mo);
				s.set_int       ("serial",    fileindex);
			}
		}
		char filename[FILENAME_SIZE];
		snprintf(filename, sizeof(filename), PLAYERDIRNAME_TEMPLATE, p);
		fs.EnsureDirectoryExists(filename);
		snprintf(filename, sizeof(filename),      FILENAME_TEMPLATE, p);
		prof.write(filename, false, fs);
	}
}

}
