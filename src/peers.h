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

#ifndef PEERS_H
#define PEERS_H

#include <arpa/inet.h>
#include <stdint.h>

#include "linkedlist.h"

/* number of peers guaranteed to be in the network */
#define	DEFAULT_PEERS_SIZE	2
/* maximum number of peers we store */
#define	MAX_PEERS_SIZE		50

/* IPv6 addresses of peers guaranteed to be in the network */
static const unsigned char DEFAULT_PEERS[DEFAULT_PEERS_SIZE][16] = {
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 255, 255,
		147, 251, 210, 17
	},
	{
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 255, 255,
		147, 251, 210, 23
	}
};

/** Peer info holder. */
typedef struct s_peer {
	/** Binary IPv6 address. */
	struct in6_addr	addr;
	/** A peer is not available if they is already our neighbour,
	 *  pending to become one, or if we are unnable to connect to them.
	 *  1 if available, 0 if not.
	 */
	short is_available;
	/* TODO: add uptime */
} peer_t;

linkedlist_node_t *add_peer(linkedlist_t *peers, const struct in6_addr *addr);

void clear_peers(linkedlist_t *peers);

size_t fetch_available_peers(const linkedlist_t *peers,
			     peer_t *available_peers[MAX_PEERS_SIZE]);

int fetch_peers(const char *peers_path, linkedlist_t *peers);

peer_t *find_peer_by_addr(const linkedlist_t	*peers,
			  const struct in6_addr	*addr);

void peers_to_str(const linkedlist_t *peers, char *output);

void reset_peers_availability(linkedlist_t *peers);

void shuffle_peers_arr(peer_t *peers[MAX_PEERS_SIZE], size_t peers_size);

int store_peers(const char *peers_path, const linkedlist_t *peers);

#endif /* PEERS_H */
