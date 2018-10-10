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

#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <event2/event.h>

#include "linkedlist.h"
#include "paths.h"
#include "peers.h"

/**
 * Event loop works with the data stored in an instance of this struct.
 */
typedef struct s_global_state {
	/** For holding and polling events. */
	struct event_base *event_loop;
	/** A list of registered events of the event loop. */
	linkedlist_t events;
	/** Holder of paths to needed files/dirs. */
	filepaths_t filepaths;
	/** Linked list of some known hosts in the network. */
	linkedlist_t hosts;
	/** List of our identities. */
	linkedlist_t identities;
	/** Linked list of recently received message traces (their hash and
	 * a pointer to a neighbour who's sent us the message). */
	linkedlist_t message_traces;
	/** Linked list of our neighbours. */
	linkedlist_t neighbours;
	/** Known peers. */
	linkedlist_t peers;
	/** Hosts that didn't accept/reject us yet. */
	linkedlist_t pending_neighbours;
	/** We listen on this port. */
	unsigned short port;
	/** Routes to hosts. */
	linkedlist_t routing_table;
	/** Our true identity. */
	identity_t *true_identity;
} global_state_t;

void global_state_clear(global_state_t *global_state);
int global_state_init(global_state_t *global_state);

#endif /* GLOBAL_STATE_H */
