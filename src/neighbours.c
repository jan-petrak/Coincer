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

#include <event2/bufferevent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linkedlist.h"
#include "neighbours.h"

/**
 * Initialize the linked list of neighbours to default values.
 *
 * @param	neighbours	Linked list to be initialized.
 */
void neighbours_init(linkedlist_t *neighbours)
{
	linkedlist_init(neighbours);
}

/** 
 * Find neighbour in neighbours based on their bufferevent.
 *
 * @param 	neighbours	Linked list of our neighbours.
 * @param	bev		Neighbour's bufferevent.
 *
 * @return 	Desired neighbour.
 * @return 	NULL if not found.
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
 * Find neighbour in neighbours based on their ip_addr.
 *
 * @param 	neighbours	Linked list of our neighbours.
 * @param	ip_addr		Binary ip address stored in 16 bytes.
 *
 * @return	Desired neighbour.
 * @return	NULL if not found.
 */
neighbour_t *find_neighbour_by_ip(const linkedlist_t	*neighbours,
                                  const unsigned char	*ip_addr)
{
	/* start the search from the first linked list node */
	const linkedlist_node_t *current = linkedlist_get_first(neighbours);

	while (current != NULL) {
		/* data of the 'current' node; struct s_neighbour */
		neighbour_t *current_data = (neighbour_t *) current->data;

		/* ip addresses match => neighbour found */
		if (memcmp(current_data->ip_addr, ip_addr, 16) == 0) {
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
 * Add new neighbour into neighbours.
 *
 * @param 	neighbours	Linked list of our neighbours.
 * @param	ip_addr		Binary ip address stored in 16 bytes.
 * @param	bev		Neighbour's bufferevent.
 *
 * @return	Newly added neighbour.
 * @return	NULL if neighbour already exists or allocation failure occured.
 */
neighbour_t *add_new_neighbour(linkedlist_t		*neighbours,
			       const unsigned char	*ip_addr,
			       struct bufferevent	*bev)
{
	/* create new neighbour */
	neighbour_t *new_neighbour;
	new_neighbour = (neighbour_t *) malloc(sizeof(neighbour_t));

	/* allocation failure */
	if (new_neighbour == NULL) {
		/* TODO: Allocation failure handling */
		perror("malloc for a new neighbour");
                return NULL;
	}

	/* don't add duplicates */
	if (find_neighbour_by_ip(neighbours, ip_addr)) {
		return NULL;
	}

	/* initialize new neighbour */
	memcpy(new_neighbour->ip_addr, ip_addr, 16);
	new_neighbour->failed_pings = 0;
	new_neighbour->buffer_event = bev;

	/* add new neighbour into linked list; NULL if allocation failed */
	if (linkedlist_append(neighbours, new_neighbour) == NULL) {
		/* TODO: Allocation failure handling */
		perror("malloc for a new neighbour of linkedlist node");
		return NULL;
	}

	return new_neighbour;
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
	if (neighbour_node == NULL) {
		/* TODO: Error handling */
		perror("delete_neighbour");
		return;
	}

	/* free neighbour's bufferevent */
	bufferevent_free(bev);

	/* free the linked list node's data; neighbour_t * */
	free(neighbour_node->data);

	/* delete the neighbour from the linked list */
	linkedlist_delete(neighbours, neighbour_node);
}

/**
 * Delete all neighbours.
 *
 * @param	neighbours	Linked list of our neighbours.
 */
void clear_neighbours(linkedlist_t *neighbours)
{
	linkedlist_node_t *current_node = linkedlist_get_first(neighbours);

	/* safely delete data from the linked list nodes */
	while (current_node != NULL) {
		neighbour_t *current;

		/* load current node's data into 'current' */
		current = (neighbour_t *) current_node->data;

		/* deallocate neighbour's bufferevent */
		bufferevent_free(current->buffer_event);

		/* deallocate data */
		free(current_node->data);
		current_node->data = NULL;

		/* get next node in the linked list */
		current_node = linkedlist_get_next(neighbours, current_node);
	}

	/* safely delete all nodes in the linked list */
	linkedlist_destroy(neighbours);
}
