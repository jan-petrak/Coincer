/*
 *  Coincer
 *  Copyright (C) 2017-2018  Coincer Developers
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DAEMON_MESSAGES_H
#define DAEMON_MESSAGES_H

#include <sodium.h>
#include <stdint.h>

#include "linkedlist.h"

/** Current version of coincer daemon protocol. */
#define PROTOCOL_VERSION 1

/** Types of daemon messages. */
enum message_type {
	P2P_BYE, /**< Peer departure announcement. */
	P2P_HELLO, /**< Initial message between two neighbours. */
	P2P_PEERS_ADV, /**< Known hosts addresses advertisement.*/
	P2P_PEERS_SOL, /**< Known hosts addresses solicitation. */
	P2P_PING, /**< Test of connectivity between two neighbours. */
	P2P_PONG, /**< Response to a p2p.ping. */
	P2P_ROUTE_ADV, /**< Peer reachability advertisement. */
	P2P_ROUTE_SOL /**< Peer reachability solicitation. */
};

/**
 * p2p.hello data holder.
 */
typedef struct s_p2p_hello {
	/** Peer's software info, the format is like BIP14: /Coincer:1.0.0/. */
	char *client;
	/** Peer's listening port. */
	unsigned short port;
} p2p_hello_t;

/**
 * p2p.peers.adv data holder.
 */
typedef struct s_p2p_peers_adv {
	/** List of some hosts in readable format: [ [ addr, port ], ... ]. */
	char *addresses;
} p2p_peers_adv_t;

/**
 * p2p.route.sol data holder.
 */
typedef struct s_p2p_route_sol {
	/** The peer's identifier. */
	unsigned char target[crypto_box_PUBLICKEYBYTES];
} p2p_route_sol_t;

/**
 * Internal message body representation.
 */
typedef struct s_message_body {
	unsigned char	  to[crypto_box_PUBLICKEYBYTES]; /**< Destination. */
	enum message_type type; /**< Message type. */
	void		  *data; /**< Message content. */
	uint64_t	  nonce; /**< Message's nonce. */
} message_body_t;

/**
 * Internal message representation.
 */
typedef struct s_message {
	int		version; /**< Protocol version. */
	unsigned char	from[crypto_box_PUBLICKEYBYTES]; /**< Sender. */
	message_body_t	body; /**< Message body. */
	unsigned char	sig[crypto_sign_BYTES]; /**< Signature. */
} message_t;

int create_p2p_bye(message_t *message);

int create_p2p_hello(message_t *message, unsigned short port);

int create_p2p_peers_adv(message_t *message, const linkedlist_t *hosts);

int create_p2p_peers_sol(message_t *message);

int create_p2p_ping(message_t *message);

int create_p2p_pong(message_t *message);

int create_p2p_route_adv(message_t *message);

int create_p2p_route_sol(message_t *msg_body, const unsigned char *target);

void message_delete(message_t *message);

#endif /* DAEMON_MESSAGES_H */
