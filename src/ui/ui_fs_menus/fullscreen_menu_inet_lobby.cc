/*
 * Copyright (C) 2004, 2006-2008 by the Widelands Development Team
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

#include "fullscreen_menu_inet_lobby.h"

#include "constants.h"
#include "game_server_connection.h"
#include "game_server_proto_packet_hello.h"
#include "game_server_proto_packet_chatmessage.h"
#include "game_server_proto_packet_connect.h"
#include "game_server_proto_packet_getuserinfo.h"
#include "i18n.h"

#include "ui_button.h"
#include "ui_editbox.h"
#include "ui_listselect.h"
#include "ui_modal_messagebox.h"
#include "ui_multilinetextarea.h"
#include "ui_textarea.h"

/*
 * Static callback functions for various packets
 */
static void user_entered(std::string name, std::string room, bool enters, void* data) {
	static_cast<Fullscreen_Menu_InetLobby *>(data)->user_entered(name, room, enters);
}
static void server_message(std::string str, void* data) {
	static_cast<Fullscreen_Menu_InetLobby*>(data)->server_message(str);
}
static void room_info(std::vector< std::string > users, void* data) {
	static_cast<Fullscreen_Menu_InetLobby *>(data)->room_info(users);
}
static void user_info(std::string user, std::string game, std::string room, void* data) {
	static_cast<Fullscreen_Menu_InetLobby *>(data)->user_info(user, game, room);
}
static void chat_message(std::string user, std::string msg, uint8_t is_action, void* data) {
	static_cast<Fullscreen_Menu_InetLobby*>(data)
		->chat_message(user, msg, is_action);
}
static void critical_error(std::string str, void* data) {
	static_cast<Fullscreen_Menu_InetLobby *>(data)->critical_error(str);
}
static void disconnect(void* data) {
	static_cast<Fullscreen_Menu_InetLobby *>(data)->disconnect();
}

/*
 * Init this, send a hello packet
 */
Fullscreen_Menu_InetLobby::Fullscreen_Menu_InetLobby(Game_Server_Connection* gsc)
	:Fullscreen_Menu_Base("singleplmenu.jpg") // change this
{
	m_disconnect_expected = false;

	UI::Textarea* title= new UI::Textarea(this, MENU_XRES/2, 10, _("Lobby"), Align_HCenter);
	title->set_font(UI_FONT_BIG, UI_FONT_CLR_FG);

	m_chatarea = new UI::Multiline_Textarea
		(this,
		 5, 40,
		 static_cast<int32_t>(get_inner_w() * 0.75) - 5, get_inner_h() - 40 - 50,
		 0, Align_Left, 1);
	m_chatarea->set_scrollmode(UI::Multiline_Textarea::ScrollLog);

	m_chatbox =
		new UI::Edit_Box
		(this,
		 5, get_inner_h() - 45,
		 static_cast<int32_t>(get_inner_w() * 0.75) - 5, 25,
		 0, 1);
	m_chatbox->changed.set(this, &Fullscreen_Menu_InetLobby::changed);

	m_userlist =
		new UI::Listselect<void *>
		(this,
		 static_cast<int32_t>(get_inner_w()-get_inner_w() * 0.25 +  5),
		 40,
		 static_cast<int32_t>(get_inner_w()               * 0.25 - 10),
		 get_inner_h() - 40 - 50,
		 Align_Left);

	new UI::Button<Fullscreen_Menu_InetLobby>
		(this,
		 get_inner_w() - get_inner_w() / 4 + 5, get_inner_h() - 45, 50, 25,
		 0,
		 &Fullscreen_Menu_InetLobby::clicked_back, this,
		 _("Back"));

	m_gsc = gsc;
	gsc->set_server_message_handler(&::server_message, this);
	gsc->set_user_entered_handler(&::user_entered, this);
	gsc->set_get_room_info_handler(&::room_info, this);
	gsc->set_get_user_info_handler(&::user_info, this);
	gsc->set_chat_message_handler(&::chat_message, this);
	gsc->set_critical_error_handler(&::critical_error, this);
	gsc->set_disconnect_handler(&::disconnect, this);

	//  send welcome packet(s)
	m_gsc->send(new Game_Server_Protocol_Packet_Connect());
	m_gsc->send(new Game_Server_Protocol_Packet_Hello(gsc->get_username()));
}

Fullscreen_Menu_InetLobby::~Fullscreen_Menu_InetLobby() {}

/*
 * Check if there is network data for us
 */
void Fullscreen_Menu_InetLobby::think() {
	m_gsc->handle_data();
}

/*
 * The editbox has changed, this is to send something over the net
 */
void Fullscreen_Menu_InetLobby::changed() {
	std::string text = m_chatbox->get_text();
	m_chatbox->set_text("");

	//  TODO check here if this is a Server message
	Game_Server_Protocol_Packet_ChatMessage* cm = new Game_Server_Protocol_Packet_ChatMessage(0, text);
	m_gsc->send(cm);
}

/*
 * A button has been clicked
 */
void Fullscreen_Menu_InetLobby::clicked_back() {
	//  TODO shut down connection gracefully
	end_modal(0);
}

/*
 * a server message has reached us. This is text for the user
 * but not written by any other user, but send by the
 * server (as response to a packet mostly)
 */
void Fullscreen_Menu_InetLobby::server_message(std::string str) {
	m_chatarea->set_text((m_chatarea->get_text() + str).c_str());
}

/*
 * A room info request has returned
 */
void Fullscreen_Menu_InetLobby::room_info(std::vector<std::string > users) {
	m_userlist->clear(); //  We clear the list, and refill it.

	for (uint32_t i = 0; i < users.size(); ++i)
		if (users[i] != m_gsc->get_username()) {
			std::string name = users[i];
			m_userlist->add(name.c_str(), 0);
			// Get User Informations
			Game_Server_Protocol_Packet_GetUserInfo *gui = new Game_Server_Protocol_Packet_GetUserInfo(users[i]);
			m_gsc->send(gui);
		}
	m_userlist->sort();
}

/*
 * A user info request has returned
 */
void Fullscreen_Menu_InetLobby::user_info(std::string user, std::string game, std::string room) {
	char buffer[1024];

	snprintf
		(buffer, sizeof(buffer),
		 _("User %s is using \"%s\" in room \"%s\"\n").c_str(),
		 user.c_str(), game.c_str(), room.c_str());
	server_message(buffer);
}

/*
 * A chat message has arrived
 */
void Fullscreen_Menu_InetLobby::chat_message(std::string user, std::string msg, bool is_action) {
	std::string buffer;

	if (is_action) {
		buffer  = "** ";
		buffer += user;
		buffer += ' ';
		buffer += msg;
		buffer += '\n';
		server_message(buffer.c_str());
		return;
	}

	std::string text = m_chatbox->get_text();
	buffer  = user;
	buffer += ": ";
	buffer += msg;
	buffer += '\n';
	m_chatarea->set_text((m_chatarea->get_text() + buffer).c_str());
}


/*
 * A User entered the room
 */
void Fullscreen_Menu_InetLobby::user_entered(std::string gname, std::string groom, bool enters) {
	std::string name = gname;

	if (groom == m_gsc->get_room()) {
		if (enters) m_userlist->add   (name.c_str(), 0);
		else        m_userlist->remove(name.c_str());
	}

	m_userlist->sort();

	std::string str =
		enters ?
		_("User %s has entered the room %s !\n")
		:
		_("User %s has left the room %s !\n");
	server_message(str);

	m_gsc->send(new Game_Server_Protocol_Packet_GetUserInfo(gname));
}

/*
 * A critical connection error has occured. It
 * is likely that the connection is terminated, but
 * we only have to care for showing the user the
 * message
 */
void Fullscreen_Menu_InetLobby::critical_error(std::string str) {
	UI::Modal_Message_Box mmb
		(this, _("Critical Connection Error!"), str, UI::Modal_Message_Box::OK);
	mmb.run();
	m_disconnect_expected = true;
}

/*
 * A disconnect occured
 */
void Fullscreen_Menu_InetLobby::disconnect() {
	if (not m_disconnect_expected) {
		UI::Modal_Message_Box mmb
			(this,
			 _("Critical Connection Error!"),
			 _("Server disconnected unexpectedly!"),
			 UI::Modal_Message_Box::OK);
		mmb.run();
	}
	end_modal(0);
	return;
}
