/*
 * Copyright (C) 2012 by the Widelands Development Team
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

#ifndef INTERNET_GAMING_PROTOCOL_H
#define INTERNET_GAMING_PROTOCOL_H

#include <string>






/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * BASIC SETUP VALUES                                                      *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * The current version of the in-game network protocol. Client and metaserver
 * protocol versions must match.
 */
#define INTERNET_GAMING_PROTOCOL_VERSION 0

/**
 * The default timeout time after which the client tries to resend a package or even finally closes the
 * connection to the metaserver, if no answer to a previous package (which requires an answer) was
 * received. In case of a login or reconnect, this is the time to wait for the metaservers answer.
 *
 * value is in milliseconds
 *
 * \todo Should this be resetable by the user?
 */
#define INTERNET_GAMING_TIMEOUT 30000 // 30 seconds

/**
 * The default timeout time after which the client tries to resend a package or even finally closes the
 * connection to the metaserver, if no answer to a previous package (which requires an answer) was
 * received. In case of a login or reconnect, this is the time to wait for the metaservers answer.
 *
 * value is in milliseconds
 *
 * \todo Should this be resetable by the user?
 */
#define INTERNET_GAMING_CLIENT_TIMEOUT 60000 // 60 seconds - some time to reconnect

/**
 * The default number of retries after a timeout after which the client finally closes the
 * connection to the metaserver.
 *
 * \todo Should this be resetable by the user?
 */
#define INTERNET_GAMING_RETRIES 3


/// Metaserver connection details
static const std::string INTERNET_GAMING_METASERVER = "widelands.org";
#define INTERNET_GAMING_PORT 7395


/// The maximum number of clients (players + spectators) per game
#define INTERNET_GAMING_MAX_CLIENTS_PER_GAME 32






/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * CLIENT RIGHTS                  *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/// Client rights
enum {
	INTERNET_CLIENT_UNREGISTERED = 0,
	INTERNET_CLIENT_REGISTERED   = 1,
	INTERNET_CLIENT_SUPERUSER    = 2,
	INTERNET_CLIENT_BOT          = 3
};





/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * COMMUNICATION PROTOCOL BETWEEN CLIENT AND METASERVER                    *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**
 * Below are the command codes used in the internet gaming protocol.
 *
 * The internet gaming protocol is responsible for the communication between the
 * metaserver and the clients.
 *
 * The network stream of the internet gaming protocol is split up into
 * packets (see \ref Deserializer, \ref RecvPacket, \ref SendPacket).
 * Every packet starts with a single-byte command code.
 */

/**
 * Bidirectional command: Terminate the connection with a given reason.
 *
 * Payload is:
 * \li String: reason for disconnect in message code (see internet_gaming_messages.h)
 *
 * Both metaserver and client can send this command, followed by immediately
 * closing the connection. The receiver of this command should just close the connection.
 *
 * Note that either party is allowed to close the connection without sending a \ref IGPCMD_DISCONNECT
 * command first (in any case, this can happen when the program crashes or network connection is lost).
 *
 * \note If you want to change the payload of this command, change it only by appending new items. The
 * reason is that this is the only command that can be sent by the metaserver even when the protocol
 * versions differ.
 */
static const std::string IGPCMD_DISCONNECT = "DISCONNECT";

/**
 * Initiate a connection.
 *
 * The first communication across the network stream is a LOGIN command
 * sent by the client, with the following payload:
 * \li Unsigned8: protocol version
 * \li String:    client name
 * \li String:    build_id of the client
 * \li Unsigned8: whether the client wants to login in to a registered account (0 = false, 1 = true)
 * \li String:    password in clear text - only valid if previous was 1
 *
 * If the metaserver accepts, it replies with a LOGIN command with the following payload:
 * \li String:    client name (might be different to the previously chosen one, if the client did
 *                NOT login to a registered account and either the chosen is registered or already used.)
 * \li String:    welcome message (this might hold information about why the chosen name was changed to
 *                another or e.g. informations like "you've got 1 unread message on widelands.org")
 * \li Unsigned8: clients rights  (see client rights enum above)
 *
 * If no answer is received in \ref INTERNET_GAMING_TIMEOUT ms the client will again try to login
 * \ref INTERNET_GAMING_RETRIES times until it finally bails out something like "server does not answer"
 *
 * For the case, that the metaserver does not accept the login, take a look at \ref IGPCMD_REJECTED
 */
static const std::string IGPCMD_LOGIN = "LOGIN";

/**
 * Reinitiate a connection.
 *
 * Basically the difference to \ref IGPCMD_LOGIN is, that the metaserver allows the user to go on at the
 * place it was before - so if the user was in a running game and only lost connection to the metaserver,
 * but not to the game itself, statistics for that game can still be send and saved.
 * To ensure everything is fine, the login information will still be checked + if a client with the name
 * is still listed as online, the metaserver will ping the client and if the client does not answer intime
 * it will be replaced by the user requesting the relogin
 *
 * sent by the client, with the following payload:
 * \li Unsigned8: protocol version
 * \li String:    client name - the one the metaserver replied at the first login
 * \li String:    build_id of the client
 * \li Unsigned8: whether the client wants to login in to a registered account (0 = false, 1 = true)
 * \li String:    password in clear text - only valid if previous was 1
 *
 * If the metaserver accepts, it replies with a LOGIN command without any payload.
 *
 * If no answer is received in INTERNET_GAMING_TIMEOUT ms the client will try to relogin
 * INTERNET_GAMING_RETRIES times until it finally bails out something like "server does not answer"
 *
 * For the case, that the metaserver does not accept the login, take a look at IGPCMD_REJECTED
 */
static const std::string IGPCMD_RELOGIN = "RELOGIN";

/**
 * If the metaserver does not accept the login for a certain reason (password wrong, user blocked?,
 * whatever...) it replies with a REJECTED command with the following payload:
 * \li String:    Reason why the client was rejected as message code
 */
static const std::string IGPCMD_REJECTED = "REJECTED";

/**
 * This is send by the metaserver to inform the client, about the metaserver time = time(0).
 */
static const std::string IGPCMD_TIME = "TIME";

/**
 * This is send by a superuser client to change the motd. The server has to check the permissions and if those
 * allow a motd change has to change the motd and afterwards to broadcast the new motd to all clients.
 * If the client has no right to change the motd, the server disconnects the client with a permission denied
 * message. It should further log that try to access superuser functionality.
 * \li String: new motd
 */
static const std::string IGPCMD_MOTD = "MOTD";

// in future here should the superuser commands be - but we need an interface in widelands for it anyways, so
// that can wait...

/**
 * Sent by the metaserver without payload. The client must reply with a \ref IGPCMD_PONG command.
 * If the client does not answer on a PING within INTERNET_GAMING_CLIENT_TIMEOUT it gets disconnected.
 */
static const std::string IGPCMD_PING = "PING";

/**
 * Reply to a \ref IGPCMD_PING command, without payload.
 */
static const std::string IGPCMD_PONG = "PONG";

/**
 * Sent by both metaserver and client to exchange chat messages, though with different payloads.
 *
 * The client sends this message to the metaserver with the following payload:
 * \li String: the message
 * \li String: name of user, if private message, else empty string.
 * The metaserver will echo the message if the client is allowed to send chat messages.
 *
 * The metaserver either broadcasts a chat message to all clients or sends it to the pm recipient with the
 * following payload:
 * \li String: sender (may be empty to indicate system messages)
 * \li String: the message
 * \li Unsigned8: whether this is a personal message (0 = false, 1 = true)
 * \li Unsigned8: whether this is a system message, which should even be shown in game (0 = false, 1 = true)
 *
 * \note system messages are the motd (Send by the metaserver to the client, after login (but not relogin)
 *       and after the motd got changed) and announcements by superusers.
 */
static const std::string IGPCMD_CHAT = "CHAT";

/**
 * Sent by the metaserver to inform the client, that the list of games was changed. No payload is sent,
 * as e.g. clients in a game are not really interested about other games and we want to keep traffic
 * as low as possible.
 *
 * To get the new list of games, the client must send \ref IGPCMD_GAMES
 */
static const std::string IGPCMD_GAMES_UPDATE = "GAMES_UPDATE";

/**
 * Sent by the client without payload to ask for the current list of games.
 *
 * Sent by the metaserver with following payload:
 * \li Unsigned8: Number of game packages and for uint8_t i = 0; i < num; ++i {:
 * \li String:    Name of the game
 * \li String:    Widelands version
 * \li Unsigned8: State of the game (1 = connectable, 0 = not connectable)
 * }
 */
static const std::string IGPCMD_GAMES = "GAMES";

/**
 * Sent by the metaserver to inform the client, that the list of clients was changed. No payload is sent,
 * as e.g. clients in a game are not really interested about other clients and we want to keep traffic
 * as low as possible.
 *
 * To get the new list of clients, the client must send \ref IGPCMD_CLIENT
 */
static const std::string IGPCMD_CLIENTS_UPDATE = "CLIENTS_UPDATE";

/**
 * Sent by the client without payload to ask for the current list of clients.
 *
 * Sent by the metaserver with following payload:
 * \li Unsigned8: Number of client packages and for uint8_t i = 0; i < num; ++i {:
 * \li String:    Name of the Client
 * \li String:    Widelands version
 * \li String:    Server the player is connected to, else empty.
 * \li Unsigned8: Clients rights (see client rights enum above)
 * }
 */
static const std::string IGPCMD_CLIENTS = "CLIENTS";

/**
 * Sent by the client to announce the startup of a game with following payload:
 * \li String:    name
 * \li Unsigned8: number of maximal clients
 * \note build_id is not necessary, as this is in every way the build_id of the hosting client.
 *
 * Sent by the metaserver to acknowledge the startup of a new game without payload. The metaserver will
 * list the new game, but set it as not connectable and recheck the connectability for
 * INTERNET_GAMING_TIMEOUT ms.
 * If the game gets connectable in time, the metaserver lists the game as connectable, else it removes the
 * game from the list of games and sends a \ref IGPCMD_GAME_NOT_CONNECTABLE to the hosting client.
 */
static const std::string IGPCMD_GAME_OPEN = "GAME_OPEN";

/**
 * Sent by the client to initialize the connection to a game with following payload:
 * \li String:    name of the game the client wants to connect to
 * \note the client will wait for the metaserver answer, as it needs the ip adress. if the answer is
 *       not received at time it will retry until the maximum numbers of retries is reached and the
 *       client finally closes the connection.
 *
 * Sent by the metaserver to acknowledge the connection request and to submit the ip of the game
 * \li String:    ip of the game.
 * \note as soon as this message is sent, the metaserver will list the client as connected to the game.
 */
static const std::string IGPCMD_GAME_CONNECT = "GAME_CONNECT";

/**
 * Sent by the client to close the connection to a game without payload, as the client can
 * only be on one game at time.
 * This is the case in *every* way a client leaves a game. No matter if a game was played or not or whether
 * the client is the host or not.
 *
 * Sent by the metaserver to acknowledge the close of connection request
 * \note as soon as this message is sent, the metaserver will list the client as not connected to any
 *       game.
 * \note if the client that sends this message is the host of the game, the game will be
 *       removed from list as well. However other clients connected to that game should send the
 *       \ref IGPCMD_GAME_DISCONNECT themselves.
 */
static const std::string IGPCMD_GAME_DISCONNECT = "GAME_DISCONNECT";

/**
 * Sent by the game hosting client to announce the start of the game. No payload.
 * \note the hosting client will wait for the metaserver answer,to ensure the game is listed. If even
 *       retries are not answered, the connection to the metaserver will be closed and a message shall be
 *       send in the newly started game.
 *
 * Sent by the metaserver to acknowledge the start without payload.
 */
static const std::string IGPCMD_GAME_START = "GAME_START";

/**
 * Sent by every participating player of a game to announce the end of the game and to send the statistics.
 * Payload is:
 * \li String:     name of the map
 * \li String:     names of the winners seperated with spaces
 * \li String:     informative string about the win condition.
 * \li Unsigned32: in game time until end
 *
 * \note this does not end the physical game and thus the metaserver should not remove the game from
 *       the list. The clients might want to play on, so...
 * \note the client will wait for the metaserver answer,to ensure the game is listed. If even retries
 *       are not answered, the connection to the metaserver will be closed and a message shall be send
 *       to the client in the still open game.
 *
 * Sent by the metaserver to acknowledge that it received the data.
 */
static const std::string IGPCMD_GAME_END = "GAME_END";

/// Sent by the metaserver to tell a game hosting client, that the game is not connectable. No payload.
static const std::string IGPCMD_GAME_NOT_CONNECTABLE = "GAME_NOT_CONNECTABLE";


#endif
