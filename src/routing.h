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

#ifndef ROUTING_H
#define ROUTING_H

#include <sodium.h>
#include <stdint.h>
#include <time.h>

#include "daemon_messages.h"
#include "global_state.h"
#include "linkedlist.h"
#include "neighbours.h"
#include "peers.h"

/** After this many seconds we consider a message trace to be stale. */
#define MESSAGE_TRACE_STALE_TIME 60
/** After this many seconds we consider a route to be stale. */
#define ROUTE_STALE_TIME	 60

/** A message trace representation. */
typedef struct s_message_trace {
	/** Message's nonce value. */
	uint64_t nonce_value;
	/** Neighbour who's sent us the message */
	const neighbour_t *sender;
	/** Creation timestamp. */
	time_t creation;
} message_trace_t;

/** Data type for a routing table. */
typedef struct s_route {
	/** Destination peer. */
	peer_t *destination;
	/** List of possible next hops (pointers to our neighbours)
	 *  to the destination. Naturally sorted by delay viability. */
	linkedlist_t next_hops;
	/** The time of the last destination's announcement of presence or
	 *  the time of this route's creation. */
	time_t last_update;
	/** The route's node in the routing table. */
	linkedlist_node_t *node;
} route_t;

int message_forward(const message_t	*message,
		    const neighbour_t	*sender,
		    global_state_t	*global_state);

int message_trace_is_stale(const message_trace_t *msg_trace,
			   const time_t		 *current_time);

route_t *route_add(linkedlist_t		*routing_table,
		   peer_t		*dest,
		   neighbour_t		*next_hop);
void route_clear(route_t *route);
void route_delete(linkedlist_t *routing_table, const unsigned char *dest_id);
route_t *route_find(const linkedlist_t	*routing_table,
		    const unsigned char	*dest_id);
int route_is_stale(const route_t *route, const time_t *current_time);
int route_next_hop_add(route_t *route, neighbour_t *next_hop);
void route_next_hop_remove(route_t *route, neighbour_t *next_hop);
int route_reset(route_t *route, neighbour_t *next_hop);

int routing_loop_detect(const linkedlist_t	*msg_traces,
			const neighbour_t	*neighbour,
			uint64_t		nonce_value);
void routing_loop_remove(linkedlist_t		*routing_table,
			 linkedlist_t		*neighbours,
			 linkedlist_t		*identities,
			 const unsigned char	*dest_id);

void routing_table_remove_next_hop(linkedlist_t	*routing_table,
				   neighbour_t	*next_hop);

int send_p2p_bye(linkedlist_t *neighbours, identity_t *identity);
int send_p2p_hello(neighbour_t *dest, unsigned short port);
int send_p2p_peers_adv(neighbour_t *dest, const linkedlist_t *hosts);
int send_p2p_peers_sol(neighbour_t *dest);
int send_p2p_ping(neighbour_t *dest);
int send_p2p_pong(neighbour_t *dest);
int send_p2p_route_adv(linkedlist_t *neighbours, identity_t *identity);
int send_p2p_route_sol(linkedlist_t	   *neighbours,
		       identity_t	   *identity,
		       const unsigned char *target);

#endif /* ROUTING_H */
