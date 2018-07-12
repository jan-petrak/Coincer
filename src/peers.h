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

/** A peer is not available if they is already our neighbour,
 *  pending to become one, or if we are unnable to connect to them.
 *  1 if available, 0 if not.
 */
#define PEER_AVAILABLE	0x01

/* IPv6 addresses of peers guaranteed to be in the network */
static const unsigned char DEFAULT_PEERS[DEFAULT_PEERS_SIZE][16] = {
/* TODO: Replace with real default peers */
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xff, 0xff,
		192, 168, 0, 124
	},
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xff, 0xff,
		192, 168, 0, 125
	}
};

/** Peer info holder. */
typedef struct s_peer {
	/** Binary IPv6 address. */
	struct in6_addr	addr;
	/** A set of flags for this peer. */
	int flags;
	/* TODO: add uptime */
} peer_t;

void clear_peers(linkedlist_t *peers);

int fetch_peers(const char *peers_path, linkedlist_t *peers);

int fetch_specific_peers(const linkedlist_t	*peers,
			 peer_t			***output,
			 int			flags);

peer_t *find_peer_by_addr(const linkedlist_t	*peers,
			  const struct in6_addr	*addr);

void peers_to_str(const linkedlist_t *peers, char *output);

void reset_peers_availability(linkedlist_t *peers);

peer_t *save_peer(linkedlist_t *peers, const struct in6_addr *addr);

void set_peer_flags(peer_t *peer, int flags);

void shuffle_peers_arr(peer_t **peers, size_t peers_size);

int store_peers(const char *peers_path, const linkedlist_t *peers);

void unset_peer_flags(peer_t *peer, int flags);

#endif /* PEERS_H */
