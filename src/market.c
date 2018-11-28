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

#include <stdlib.h>
#include <string.h>

#include "crypto.h"
#include "linkedlist.h"
#include "log.h"
#include "market.h"
#include "peers.h"

order_t *order_create(linkedlist_t *orders, int flags, void *owner)
{
	order_t *order;

	order = (order_t *) malloc(sizeof(order_t));
	if (!order) {
		log_error("Creating a new order");
		return NULL;
	}

	if (!(order->node = linkedlist_append(orders, order))) {
		free(order);
		log_error("Storing a new order");
		return NULL;
	}

	/* TODO: Create order ID */

	linkedlist_init(&order->blacklist);
	order->flags = flags;
	if (flags & ORDER_FOREIGN) {
		order->owner.cp = (peer_t *) owner;
	} else {
		order->owner.me = (identity_t *) owner;
	}

	return order;
}

void order_clear(order_t *order)
{
	linkedlist_destroy(&order->blacklist, NULL);
}

order_t *order_find(const linkedlist_t	*orders,
		    const unsigned char	*order_id)
{
	const linkedlist_node_t *current = linkedlist_get_first(orders);
	order_t *order;

	while (current) {
		order = (order_t *) current->data;

		if (!memcmp(order->id, order_id, SHA3_256_SIZE)) {
			return order;
		}

		current = linkedlist_get_next(orders, current);
	}

	return NULL;
}

int order_blacklist_append(linkedlist_t		*blacklist,
			   const unsigned char	*identifier)
{
	unsigned char *id;

	id = (unsigned char *) malloc(SHA3_256_SIZE * sizeof(unsigned char));
	if (!id) {
		log_error("Allocating order's blacklist node data");
		return 1;
	}

	if (!linkedlist_append(blacklist, id)) {
		log_error("Appending order's blacklist node");
		free(id);
		return 1;
	}

	memcpy(id, identifier, SHA3_256_SIZE);

	return 0;
}

unsigned char *order_blacklist_find(const linkedlist_t	*blacklist,
				    const unsigned char	*identifier)
{
	const linkedlist_node_t *current = linkedlist_get_first(blacklist);
	unsigned char *id;

	while (current) {
		id = (unsigned char *) current->data;

		if (!memcmp(id, identifier, SHA3_256_SIZE)) {
			return id;
		}

		current = linkedlist_get_next(blacklist, current);
	}

	return NULL;
}

void order_flags_set(order_t *order, int flags)
{
	order->flags |= flags;
}

void order_flags_unset(order_t *order, int flags)
{
	order->flags &= ~flags;
}
