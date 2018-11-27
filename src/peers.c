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

#include <sodium.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crypto.h"
#include "linkedlist.h"
#include "log.h"
#include "peers.h"

/**
 * Determine whether an identifier is empty (if all its bytes are set to 0x0).
 *
 * @param	id		Check this identifier.
 *
 * @return	1		The identifier is empty.
 * @return	0		The identifier is not empty.
 */
int identifier_empty(const unsigned char id[PUBLIC_KEY_SIZE])
{
	return id[0] == 0x0 && !memcmp(id, id + 1, PUBLIC_KEY_SIZE - 1);
}

/**
 * Find the identity for an identifier.
 *
 * @param	identities		List of our identities.
 * @param	identifier		Search for this identifier.
 *
 * @return	identity_t		Corresponding identity for the
 *					identifier.
 * @return	NULL			The identifier doesn't belong to any
 *					of our identities.
 */
identity_t *identity_find(const linkedlist_t	*identities,
			  const unsigned char	*identifier)
{
	identity_t		*identity;
	const linkedlist_node_t	*node;

	node = linkedlist_get_first(identities);
	while (node) {
		identity = (identity_t *) node->data;

		/* if the identifier represents one of our identities */
		if (!memcmp(identity->keypair.public_key,
			    identifier,
			    PUBLIC_KEY_SIZE)) {
			return identity;
		}

		node = linkedlist_get_next(identities, node);
	}

	return NULL;
}

/**
 * Set flags on given identity.
 *
 * @param	identity	Set flags on this identity.
 * @param	flags		Set these flags on the identity.
 */
void identity_flags_set(identity_t *identity, int flags)
{
	identity->flags |= flags;
}

/**
 * Unset flags on given identity.
 *
 * @param	identity	Unset flags on this identity.
 * @param	flags		Unset these flags on the identity.
 */
void identity_flags_unset(identity_t *identity, int flags)
{
	identity->flags &= ~flags;
}

/**
 * Generate an identity, including its initial nonce value.
 *
 * @param	flags		The flags of the identity.
 *
 * @return	identity_t	Dynamically allocated identity.
 * @return	NULL		Allocation failure.
 */
identity_t *identity_generate(int flags)
{
	identity_t *identity;

	identity = (identity_t *) malloc(sizeof(identity_t));
	if (!identity) {
		log_error("Creating a new identity");
		return NULL;
	}

	generate_keypair(&identity->keypair);
	identity->nonce_value = get_random_uint64_t();
	identity->last_adv = 0;
	identity->flags = 0x0;
	identity_flags_set(identity, flags);

	return identity;
}

/**
 * Nonce staleness predicate.
 *
 * @param	nonce		Check staleness of this nonce.
 * @param	current_time	The time to compare to.
 *
 * @return	1		The nonce is stale.
 * @return	0		The nonce is not stale.
 */
int nonce_is_stale(const nonce_t *nonce, const time_t current_time)
{
	return difftime(current_time, nonce->creation) >= NONCE_STALE_TIME;
}

/**
 * Store a nonce into ascendingly sorted (by nonce value) container of nonces.
 *
 * @param	nonces		The nonces container.
 * @param	value		The nonce's value.
 *
 * @return	0		Successfully stored.
 * @return	1		Failure.
 */
int nonce_store(linkedlist_t *nonces, uint64_t value)
{
	linkedlist_node_t	*cur_node;
	nonce_t			*cur_nonce;
	nonce_t			*nonce;

	nonce = (nonce_t *) malloc(sizeof(nonce_t));
	if (!nonce) {
		log_error("Creating a nonce");
		return 1;
	}

	nonce->value = value;
	nonce->creation = time(NULL);

	/* place the nonce into appropriate position */
	cur_node = linkedlist_get_last(nonces);
	while (cur_node) {
		cur_nonce = (nonce_t *) cur_node->data;
		/* appropriate position found */
		if (value > cur_nonce->value) {
			if (cur_node != linkedlist_get_last(nonces)) {
				nonce->creation = cur_nonce->creation;
			}

			if (!linkedlist_insert_after(nonces, cur_node, nonce)) {
				log_error("Storing a nonce");
				free(nonce);
				return 1;
			}

			return 0;
		}

		cur_node = linkedlist_get_prev(nonces, cur_node);
	}

	/* at this point we know the nonce should be placed at the beginning
	 * of the container; do that only if the container is empty */
	if (!linkedlist_empty(nonces) ||
	    !linkedlist_insert_after(nonces, &nonces->first, nonce)) {
		log_error("Storing a nonce");
		free(nonce);
		return 1;
	}

	return 0;
}

/**
 * Find nonce by its value in a list of nonces.
 *
 * @param	nonces		Choose the nonce from these nonces.
 * @param	value		Nonce's value.
 *
 * @return	nonce_t		Requested nonce.
 * @return	NULL		The nonce is absent.
 */
nonce_t *nonces_find(const linkedlist_t *nonces, uint64_t value)
{
	const linkedlist_node_t	*node;
	nonce_t			*nonce;

	node = linkedlist_get_first(nonces);
	while (node) {
		nonce = (nonce_t *) node->data;
		if (nonce->value == value) {
			return nonce;
		/* the nonces are ascendingly sorted */
		} else if (nonce->value > value) {
			return NULL;
		}
		node = linkedlist_get_next(nonces, node);
	}

	return NULL;
}

/**
 * Get the newest nonce (the nonce with the highest value) from a list
 * of nonces.
 *
 * @param	nonces		Use these nonces.
 *
 * @return	nonce_t		The last nonce.
 * @return	NULL		The nonces are empty.
 */
nonce_t *nonces_get_newest(const linkedlist_t *nonces)
{
	linkedlist_node_t *node;
	if ((node = linkedlist_get_last(nonces))) {
		return node->data;
	}
	return NULL;
}

/**
 * Get the oldest nonce (the nonce with the lowest value) from a list of nonces.
 *
 * @param	nonces		Use these nonces.
 *
 * @return	nonce_t		The first nonce.
 * @return	NULL		The nonces are empty.
 */
nonce_t *nonces_get_oldest(const linkedlist_t *nonces)
{
	linkedlist_node_t *node;
	if ((node = linkedlist_get_first(nonces))) {
		return node->data;
	}
	return NULL;
}

/**
 * Remove stale nonces from a list of nonces.
 *
 * @param	nonces		Use these nonces.
 */
void nonces_remove_stale(linkedlist_t *nonces)
{
	time_t			current_time;
	linkedlist_node_t	*node;
	nonce_t			*nonce;

	current_time = time(NULL);

	node = linkedlist_get_last(nonces);
	/* skip the last nonce, as we always want at least one to be stored;
	 * if the 'node' is NULL, linkedlist_get_prev returns NULL */
	node = linkedlist_get_prev(nonces, node);

	while (node && node != &nonces->first) {
		/* we are going to process this nonce */
		nonce = (nonce_t *) node->data;
		/* and move to previous node */
		node = linkedlist_get_prev(nonces, node);

		if (nonce_is_stale(nonce, current_time)) {
			/* remove the nonce's node; note that it is
			 * safe to assume 'node' isn't NULL, thanks to
			 * the usage of the linkedlist stub nodes */
			linkedlist_delete(node->next);
		}
		/* 'node' has already been moved to previous */
	}
}

/**
 * Safely delete peer's variables.
 *
 * @param	peer	Clear this peer.
 */
void peer_clear(peer_t *peer)
{
	linkedlist_destroy(&peer->nonces, NULL);
}

/**
 * Delete a peer from a list of peers.
 *
 * @param	peer	Delete this peer.
 */
void peer_delete(peer_t *peer)
{
	linkedlist_delete_safely(peer->node, peer_clear);
}

/**
 * Find a peer by its identifier in a list of peers.
 *
 * @param	peers		Use these peers.
 * @param	identifier	Peer's identifier.
 *
 * @param	peer_t		The requested peer.
 * @param	NULL		Unknown peer.
 */
peer_t *peer_find(const linkedlist_t	*peers,
		  const unsigned char	*identifier)
{
	const linkedlist_node_t	*node;
	peer_t			*peer;

	node = linkedlist_get_first(peers);
	while (node) {
		peer = (peer_t *) node->data;
		if (!memcmp(peer->identifier, identifier, PUBLIC_KEY_SIZE)) {
			return peer;
		}
		node = linkedlist_get_next(peers, node);
	}

	return NULL;
}

/**
 * Store a new peer in a list of known peers.
 *
 * @param	peers		Store into this list.
 * @param	identifier	The new peer's identifier.
 *
 * @return	peer_t		Newly stored peer.
 * @return	NULL		Failure.
 */
peer_t *peer_store(linkedlist_t		*peers,
		   const unsigned char	*identifier)
{
	peer_t *new_peer;

	new_peer = (peer_t *) malloc(sizeof(peer_t));
	if (!new_peer) {
		log_error("Allocating a new peer");
		return NULL;
	}

	linkedlist_init(&new_peer->nonces);
	memcpy(new_peer->identifier, identifier, PUBLIC_KEY_SIZE);
	memset(&new_peer->presence_nonce, 0x0, sizeof(nonce_t));

	if (!(new_peer->node = linkedlist_append(peers, new_peer))) {
		log_error("Storing a new peer");
		peer_clear(new_peer);
		free(new_peer);
		return NULL;
	}

	return new_peer;
}

/**
 * Store a presence nonce of a peer.
 *
 * @param	peer	Use this peer.
 * @param	value	The nonce's value.
 */
void presence_nonce_store(peer_t *peer, uint64_t value)
{
	peer->presence_nonce.value    = value;
	peer->presence_nonce.creation = time(NULL);
}
