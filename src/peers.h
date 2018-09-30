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

#include <sodium.h>
#include <stdint.h>
#include <time.h>

#include "crypto.h"
#include "linkedlist.h"

/** After this many seconds we consider a nonce to be stale. */
#define NONCE_STALE_TIME 60

/** If set, the identity will be removed ASAP. */
#define IDENTITY_TMP 0x01

/** Nonce representation. */
typedef struct s_nonce {
	/** A number with maximum size of (2^64 - 1) representing a nonce. */
	uint64_t value;
	/** Creation timestamp. */
	time_t creation;
} nonce_t;

/** Peer representation. */
typedef struct s_peer {
	/** Peer's public key. */
	unsigned char identifier[crypto_box_PUBLICKEYBYTES];
	/** A sorted list of nonces tied to this peer. */
	linkedlist_t nonces;
	/** The last known 'announcement of presence' nonce of this peer. */
	nonce_t presence_nonce;
	/** The peer's node in the list of peers. */
	linkedlist_node_t *node;
} peer_t;

/** Representation of one of our identities. */
typedef struct s_identity {
	/** A keypair including a public key as our identifier. */
	keypair_t keypair;
	/** Flags of this identity. */
	int flags;
	/** The time of the last p2p.route.adv of this identity.*/
	time_t last_adv;
	/** The last used nonce with this identity. */
	uint64_t nonce_value;
} identity_t;

int identifier_empty(const unsigned char id[crypto_box_PUBLICKEYBYTES]);

identity_t *identity_find(const linkedlist_t	*identities,
			  const unsigned char	*identifier);
void identity_flags_set(identity_t *identity, int flags);
void identity_flags_unset(identity_t *identity, int flags);
identity_t *identity_generate(int flags);

int nonce_is_stale(const nonce_t *nonce, const time_t current_time);
int nonce_store(linkedlist_t *nonces, uint64_t value);

nonce_t *nonces_find(const linkedlist_t *nonces, uint64_t value);
nonce_t *nonces_get_newest(const linkedlist_t *nonces);
nonce_t *nonces_get_oldest(const linkedlist_t *nonces);
void nonces_remove_stale(linkedlist_t *nonces);

void peer_clear(peer_t *peer);
void peer_delete(peer_t *peer);
peer_t *peer_find(const linkedlist_t	*peers,
		  const unsigned char	*identifier);
peer_t *peer_store(linkedlist_t		*peers,
		   const unsigned char	*identifier);

void presence_nonce_store(peer_t *peer, uint64_t value);

#endif /* PEERS_H */
