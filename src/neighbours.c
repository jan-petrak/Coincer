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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linkedlist.h"
#include "log.h"
#include "neighbours.h"

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
	if (new_neighbour == NULL) {
		log_error("add_new_neighbour - new_neighbour malloc");
                return NULL;
	}

	/* don't add duplicates */
	if (find_neighbour_by_addr(neighbours, addr) ||
	    find_neighbour(neighbours, bev)) {
		free(new_neighbour);
		return NULL;
	}

	/* initialize new neighbour */
	memcpy(&new_neighbour->addr, addr, 16);
	new_neighbour->failed_pings = 0;
	new_neighbour->buffer_event = bev;

	/* add new neighbour into linked list; NULL if the allocation failed */
	if (linkedlist_append(neighbours, new_neighbour) == NULL) {
		free(new_neighbour);
		return NULL;
	}

	return new_neighbour;
}

/**
 * Delete all neighbours.
 *
 * @param	neighbours	Linked list of our neighbours.
 */
void clear_neighbours(linkedlist_t *neighbours)
{
	neighbour_t		*current;
	linkedlist_node_t	*current_node;

	current_node = linkedlist_get_first(neighbours);
	/* safely delete data from the linked list nodes */
	while (current_node != NULL) {
		/* load current node's data into 'current' */
		current = (neighbour_t *) current_node->data;

		/* deallocate neighbour's bufferevent */
		bufferevent_free(current->buffer_event);

		/* get next node in the linked list */
		current_node = linkedlist_get_next(neighbours, current_node);
	}

	/* safely delete all nodes and their data in the linked list */
	linkedlist_destroy(neighbours);
}

/**
 * Delete neighbour from neighbours.
 *
 * @param	neighbours	Linked list of our neighbours.
 * @param	bev		Neighbour's bufferevent.
 */
void delete_neighbour(linkedlist_t *neighbours, struct bufferevent *bev)
{
	neighbour_t		*neighbour;
	linkedlist_node_t	*neighbour_node;

	neighbour = find_neighbour(neighbours, bev);
	/* no neighbour with bufferevent 'bev' => nothing to delete */
	if (neighbour == NULL) {
		return;
	}

	neighbour_node = linkedlist_find(neighbours, neighbour);
	/* since neighbour was found, neighbour_node should never be NULL */
	assert(neighbour_node != NULL);

	/* free neighbour's bufferevent */
	bufferevent_free(bev);
	/* delete the neighbour from the linked list */
	linkedlist_delete(neighbour_node);
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

	if (output != NULL) {
		*output = (neighbour_t **) malloc(linkedlist_size(neighbours) *
						  sizeof(neighbour_t *));
		if (*output == NULL) {
			log_error("Fetching specific neighbours");
			return -1;
		}
	}

	current_node = linkedlist_get_first(neighbours);
	while (current_node != NULL) {
		current_neighbour = (neighbour_t *) current_node->data;
		/* if all specified flags are being set on this neighbour */
		if ((current_neighbour->flags & flags) == flags) {
			if (output != NULL) {
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
 * Find neighbour in neighbours based on their bufferevent.
 *
 * @param	neighbours	Our neighbours.
 * @param	bev		Neighbour's bufferevent.
 *
 * @return	neighbour_t	Requested neighbour.
 * @return	NULL		If not found.
 */
neighbour_t *find_neighbour(const linkedlist_t		*neighbours,
                            const struct bufferevent	*bev)
{
	/* start the search from the first linked list node */
	const linkedlist_node_t *current = linkedlist_get_first(neighbours);

	while (current != NULL) {
		/* data of the 'current' node; struct s_neighbour */
		neighbour_t *current_data = (neighbour_t *) current->data;

		/* bufferevents equal => neighbour found */
		if (current_data->buffer_event == bev) {
			/* return node's data; struct s_neighbour */
			return current_data;
		}
		/* get next node in the linked list */
		current = linkedlist_get_next(neighbours, current);
	}
	/* neighbour not found */
	return NULL;
}

/**
 * Find neighbour in neighbours based on their addr.
 *
 * @param	neighbours	Our neighbours.
 * @param	addr		Binary ip address stored in 16 bytes.
 *
 * @return	neighbour_t	Requested neighbour.
 * @return	NULL		If not found.
 */
neighbour_t *find_neighbour_by_addr(const linkedlist_t		*neighbours,
                                    const struct in6_addr	*addr)
{
	/* start the search from the first linked list node */
	const linkedlist_node_t *current = linkedlist_get_first(neighbours);

	while (current != NULL) {
		/* data of the 'current' node; struct s_neighbour */
		neighbour_t *current_data = (neighbour_t *) current->data;

		/* ip addresses match => neighbour found */
		if (memcmp(&current_data->addr, addr, 16) == 0) {
			/* return node's data; struct s_neighbour */
			return current_data;
		}
		/* get next node in the linked list */
		current = linkedlist_get_next(neighbours, current);
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
