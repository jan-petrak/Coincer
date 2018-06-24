/*
 *  Coincer
 *  Copyright (C) 2017  Coincer Developers
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

#ifndef P2P_H
#define P2P_H

#include <event2/event.h>
#include <netinet/in.h>

#include "filing.h"
#include "linkedlist.h"
#include "neighbours.h"

/* default port for TCP listening */
#define	DEFAULT_PORT		31070
/* after TIMEOUT_TIME seconds invoke timeout callback */
#define	TIMEOUT_TIME 		30

/**
 * Event loop works with the data stored in an instance of this struct.
 */
typedef struct s_global_state {
	/**< For holding and polling events. */
	struct event_base *event_loop;
	/**< Holder of paths to needed files/dirs. */
	filepaths_t filepaths;
	/**< Linked list of our neighbours. */
	linkedlist_t neighbours;
	/**< Linked list of some peers in the network. */
	linkedlist_t peers;
	/**< Peers that didn't accept/reject us yet. */
	linkedlist_t pending_neighbours;
} global_state_t;

int listen_init(struct evconnlistener	**listener,
		global_state_t		*global_state);

void add_more_connections(global_state_t *global_state, size_t conns_amount);

int connect_to_addr(global_state_t		*global_state,
		    const struct in6_addr	*addr);

void ask_for_peers(neighbour_t *neighbour);

#endif /* P2P_H */

