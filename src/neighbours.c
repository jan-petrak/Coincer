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

#include <assert.h>
#include <event2/bufferevent.h>
#include <netinet/in.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypto.h"
#include "linkedlist.h"
#include "log.h"
#include "neighbours.h"
#include "peers.h"

/**
 * Add new neighbour into neighbours.
 *
 * @param	neighbours	Linked list of our neighbours.
 * @param	addr		Binary ip address stored in 16 bytes.
 * @param	bev		Neighbour's bufferevent.
 *
 * @return	neighbour_t	Newly added neighbour.
 * @return	NULL		If the neighbour already exists or allocation
 *				failure occured.
 */
neighbour_t *add_new_neighbour(linkedlist_t		*neighbours,
			       const struct in6_addr	*addr,
			       struct bufferevent	*bev)
{
	neighbour_t *new_neighbour;

	/* create new neighbour */
	new_neighbour = (neighbour_t *) malloc(sizeof(neighbour_t));
	/* allocation failure */
	if (!new_neighbour) {
		log_error("add_new_neighbour - new_neighbour malloc");
                return NULL;
	}

	/* don't add duplicates */
	if (find_neighbour(neighbours, addr, compare_neighbour_addrs) ||
	    find_neighbour(neighbours, bev,  compare_neighbour_bufferevents)) {
		free(new_neighbour);
		return NULL;
	}

	/* initialize new neighbour */
	memcpy(&new_neighbour->addr, addr, sizeof(struct in6_addr));
	new_neighbour->buffer_event = bev;
	new_neighbour->client = NULL;
	new_neighbour->failed_pings = 0;
	new_neighbour->flags = 0x0;
	new_neighbour->host = NULL;
	/* create our pseudonym */
	generate_keypair(&new_neighbour->my_pseudonym.keypair);
	new_neighbour->my_pseudonym.nonce_value = get_random_uint64_t() >> 1;
	/* initialize neighbour's pseudonym */
	memset(new_neighbour->pseudonym.identifier,
	       0x0,
	       crypto_box_PUBLICKEYBYTES);
	linkedlist_init(&new_neighbour->pseudonym.nonces);
	memset(&new_neighbour->pseudonym.presence_nonce,
	       0x0,
	       sizeof(nonce_t));

	if (!(new_neighbour->node = linkedlist_append(neighbours,
						      new_neighbour))) {
		clear_neighbour(new_neighbour);
		free(new_neighbour);
		return NULL;
	}

	return new_neighbour;
}

/**
 * Safely delete all neighbour's data.
 *
 * @param	neighbour	Delete this neighbour's data.
 */
void clear_neighbour(neighbour_t *neighbour)
{
	bufferevent_free(neighbour->buffer_event);
	if (neighbour->client) {
		free(neighbour->client);
	}
	peer_clear(&neighbour->pseudonym);
}

/**
 * Comparing function between a neighbour's address and an address.
 *
 * @param	neighbour	Use address of this neighbour.
 * @param	addr		Compare to this address.
 *
 * @return	0		The addresses equal.
 * @return	<0		'addr' is greater.
 * @return	>0		Neighbour's addr is greater.
 */
int compare_neighbour_addrs(const neighbour_t		*neighbour,
			    const struct in6_addr	*addr)
{
	return memcmp(&neighbour->addr, addr, sizeof(neighbour->addr));
}

/**
 * Comparing function between a neighbour's bufferevent and a bufferevent.
 *
 * @param	neighbour	Use bufferevent of this neighbour.
 * @param	bev		Compare to this bufferevent.
 *
 * @return	0		The bufferevents equal.
 * @return	1		The bufferevents do not equal.
 */
int compare_neighbour_bufferevents(const neighbour_t		*neighbour,
				   const struct bufferevent	*bev)
{
	return neighbour->buffer_event != bev;
}

/**
 * Comparing function between our neighbour-specific pseudonym and a public key.
 *
 * @param	neighbour	Use this neighbour.
 * @param	public_key	Compare to this public key.
 *
 * @return	0		The public keys equal.
 * @return	<0		'public_key' is greater.
 * @return	>0		The pseudonym's public key is greater.
 */
int compare_neighbour_my_pseudonyms(const neighbour_t *neighbour,
				    unsigned char     *public_key)
{
	return memcmp(neighbour->my_pseudonym.keypair.public_key,
		      public_key,
		      crypto_box_PUBLICKEYBYTES);
}

/**
 * Comparing function between neighbour's pseudonym and a public key.
 *
 * @param	neighbour	Use this neighbour.
 * @param	public_key	Compare to this public key.
 *
 * @return	0		The public keys equal.
 * @return	<0		'public_key' is greater.
 * @return	>0		The pseudonym's public key is greater.
 */
int compare_neighbour_pseudonyms(const neighbour_t *neighbour,
				 unsigned char	   *public_key)
{
	return memcmp(neighbour->pseudonym.identifier,
		      public_key,
		      crypto_box_PUBLICKEYBYTES);
}

/**
 * Fetch pointers to neighbours with specific flags set, into an array
 * that is being allocated in here.
 *
 * @param	neighbours	Our neighbours.
 * @param	output		Output array of pointers to satisfying
 *				neighbours. If set to NULL, function just
 *				returns the number of them.
 * @param	flags		Choose neighbours based on these flags.
 *				Fetches output with all neighbours if set to 0.
 *
 * @return	>=0		The number of satisfying neighbours.
 * @return	-1		Allocation failure.
 */
int fetch_specific_neighbours(const linkedlist_t	*neighbours,
			      neighbour_t		***output,
			      int			flags)
{
	linkedlist_node_t	*current_node;
	neighbour_t		*current_neighbour;
	size_t			n = 0;

	if (output) {
		*output = (neighbour_t **) malloc(linkedlist_size(neighbours) *
						  sizeof(neighbour_t *));
		if (!*output) {
			log_error("Fetching specific neighbours");
			return -1;
		}
	}

	current_node = linkedlist_get_first(neighbours);
	while (current_node) {
		current_neighbour = (neighbour_t *) current_node->data;
		/* if all specified flags are being set on this neighbour */
		if ((current_neighbour->flags & flags) == flags) {
			if (output) {
				(*output)[n++] = current_neighbour;
			} else {
				n++;
			}
		}
		current_node = linkedlist_get_next(neighbours, current_node);
	}

	return n;
}

/**
 * Find the first neighbour that returns 0 when a comparing function
 * is applied on them.
 *
 * @param	neighbours	Our neighbours.
 * @param	attribute	Input parameter for the comparing function.
 * @param	comp_func	The comparing function.
 *
 * @return	neighbour_t	Requested neighbour.
 * @return	NULL		If not found.
 */
neighbour_t *find_neighbour(const linkedlist_t	*neighbours,
			    const void		*attribute,
			    int (*cmp_func) (const neighbour_t	*neighbour,
					     const void		*attribute))
{
	const linkedlist_node_t *current_node;
	neighbour_t		*current_neighbour;

	current_node = linkedlist_get_first(neighbours);
	while (current_node) {
		current_neighbour = (neighbour_t *) current_node->data;

		/* cmp_func returns 0 when current_neighbour's attribute
		 * is equal to 'attribute' from find_neighbour param */
		if (!cmp_func(current_neighbour, attribute)) {
			return current_neighbour;
		}

		current_node = linkedlist_get_next(neighbours, current_node);
	}
	/* neighbour not found */
	return NULL;
}

/**
 * Set flags on given neighbour.
 *
 * @param	neighbour	Set flags on this neighbour.
 * @param	flags		Set these flags on the neighbour.
 */
void set_neighbour_flags(neighbour_t *neighbour, int flags)
{
	neighbour->flags |= flags;
}

/**
 * Unset flags on given neighbour.
 *
 * @param	neighbour	Unset flags on this neighbour.
 * @param	flags		Unset these flags on the neighbour.
 */
void unset_neighbour_flags(neighbour_t *neighbour, int flags)
{
	neighbour->flags &= ~flags;
}
