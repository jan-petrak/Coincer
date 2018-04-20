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

#include "linkedlist.h"
#include "neighbours.h"

/* number of peers guaranteed to be in the network */
#define	DEFAULT_PEERS_SIZE	3
/* default port for TCP listening */
#define	DEFAULT_PORT		31070
/* minimum number of peers we need to be connected to */
#define	MINIMUM_NEIGHBOURS	2
/* after TIMEOUT_TIME seconds invoke timeout callback */
#define	TIMEOUT_TIME 		30

/* IPv6 addresses of peers guaranteed to be in the network */
static const unsigned char DEFAULT_PEERS[DEFAULT_PEERS_SIZE][16] = {
	/* TODO: Replace with the actual default peer IPs */
	{
		(int)0, (int)0, (int)0, (int)0,
		(int)0, (int)0, (int)0, (int)0,
		(int)0, (int)0, (int)0, (int)0,
		(int)0, (int)0, (int)0, (int)1
	},
	{
		(int)0, (int)0, (int)0, (int)0,
		(int)0, (int)0, (int)0, (int)0,
		(int)0, (int)0, (int)255, (int)255,
		(int)192, (int)168, (int)0, (int)125
	},
	{
		(int)0, (int)0, (int)0, (int)0,
		(int)0, (int)0, (int)0, (int)0,
		(int)0, (int)0, (int)255, (int)255,
		(int)192, (int)168, (int)0, (int)130
	}

};

/**
 * Event loop works with the data stored in an instance of this struct.
 */
typedef struct s_global_state {
	struct event_base *event_loop; /**< For holding and polling events. */
	linkedlist_t neighbours; /**< Linked list of our neighbours. */
} global_state_t;

int listen_init(struct evconnlistener	**listener,
		global_state_t		*global_state);

int joining_init(global_state_t *global_state);

neighbour_t *connect_to_peer(global_state_t		*global_state,
			     const unsigned char	*ip_addr);
#endif /* P2P_H */
