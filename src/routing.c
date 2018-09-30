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

#define _BSD_SOURCE /* usleep */

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "autoconfig.h"
#include "crypto.h"
#include "daemon_messages.h"
#include "global_state.h"
#include "json_parser.h"
#include "linkedlist.h"
#include "log.h"
#include "neighbours.h"
#include "peers.h"
#include "routing.h"

/**
 * Simple helper to determine the destination visibility of a message.
 */
enum dest_visibility {
	DV_SHOWN, /**< The message will include the destination's ID. */
	DV_HIDDEN /**< The message won't include the destination's ID. */
};

static int message_send_n2n(message_t *message, neighbour_t *dest);
static int message_send_p2p(const message_t	*message,
			    const linkedlist_t	*routing_table);
static int message_send_to_neighbour(const message_t	*message,
				     neighbour_t	*dest);

static int message_trace_store(linkedlist_t		*msg_traces,
			       const neighbour_t	*sender,
			       uint64_t			nonce_value);

static void string_send_to_neighbour(const char		*string,
				     neighbour_t	*dest);

/**
 * Pause the program execution for a random amount of time.
 *
 * @param	ms_min		Minimum random time in milliseconds.
 * @param	ms_max		Maximum random time in milliseconds. Must be
 *				lower than 1000000.
 */
static void execution_pause_random(uint32_t ms_min, uint32_t ms_max)
{
	uint32_t pause_range_ms;
	uint32_t pause_val_ms;

	pause_range_ms = get_random_uint32_t(ms_max - ms_min + 1);
	pause_val_ms   = ms_min + pause_range_ms;

	usleep(pause_val_ms);
}

/**
 * Broadcast a message in a neighbour to neighbour manner - replace the field
 * 'from' with our neighbour-speicific pseudonym.
 *
 * @param	message		The message to be sent. The message content
 *				must be initialized (the type (and the data)).
 *				The rest is initialized in this function.
 * @param	neighbours	Broadcast to these neighbours.
 */
static void message_broadcast_n2n(message_t		*message,
				  const linkedlist_t	*neighbours)
{
	neighbour_t	  *current_neigh;
	linkedlist_node_t *current_node;

	current_node = linkedlist_get_first(neighbours);
	while (current_node) {
		current_neigh = (neighbour_t *) current_node->data;

		message_send_n2n(message, current_neigh);

		current_node = linkedlist_get_next(neighbours, current_node);
	}
}

/**
 * Broadcast a message in a peer-to-peer manner (send the message as is).
 *
 * @param	message		The message to be sent.
 * @param	neighbours	Broadcast to these neighbours.
 * @param	exception	Don't send the message to this neighbour.
 *				This is typically the neighbour from whom
 *				we've received the message.
 *
 * @return	0		Success.
 * @return	1		Failure.
 */
static int message_broadcast_p2p(const message_t	*message,
				 const linkedlist_t	*neighbours,
				 const neighbour_t	*exception)
{
	char			*json_message;
	neighbour_t		*neigh;
	linkedlist_node_t	*node;

	if (encode_message(message, &json_message)) {
		log_debug("message_send_to_neighbour - encoding message");
		return 1;
	}

	node = linkedlist_get_first(neighbours);
	while (node) {
		neigh = (neighbour_t *) node->data;

		if (neigh != exception) {
			string_send_to_neighbour(json_message, neigh);
		}

		node = linkedlist_get_next(neighbours, node);
	}

	free(json_message);
	return 0;
}

/**
 * Set up a message to be sent. The type (and the data) of the message
 * body must be initialized before calling this function.
 *
 * @param	message		The message we want to set up. After successful
 *				completion of this function, this is a
 *				ready to be sent message.
 * @param	from		We want to send the message under this identity.
 * @param	to		Identifier of the destination. Can be NULL, but
 *				'dest_vis' has to be set to DV_HIDDEN.
 * @param	dest_vis	Message's destination visibility.
 *
 * @return	0		Successfully set up.
 * @return	1		Failure.
 */
static int message_finalize(message_t			*message,
			    identity_t			*from,
			    const unsigned char		*to,
			    const enum dest_visibility	dest_vis)
{
	int		cmp_val;
	char		*json_body;
	uint64_t	nonce_value;

	message->version = PROTOCOL_VERSION;
	memcpy(message->from,
	       from->keypair.public_key,
	       crypto_box_PUBLICKEYBYTES);

	if (dest_vis == DV_HIDDEN) {
		memset(message->body.to, 0x0, crypto_box_PUBLICKEYBYTES);
	} else {
		memcpy(message->body.to, to, crypto_box_PUBLICKEYBYTES);
	}

	nonce_value = from->nonce_value + 1;

	cmp_val = memcmp(message->from,
			 message->body.to,
			 crypto_box_PUBLICKEYBYTES);
	/* from ID > to ID => odd nonce; from ID < to ID => even nonce */
	if ((cmp_val > 0 && !(nonce_value & 0x01)) ||
	    (cmp_val < 0 && (nonce_value & 0x01))) {
		nonce_value++;
	}

	message->body.nonce = from->nonce_value = nonce_value;

	if (encode_message_body(&message->body, &json_body)) {
		return 1;
	}

	sign_message(json_body, from->keypair.secret_key, message->sig);
	free(json_body);
	return 0;
}

/**
 * Forward someone's message on its way to destination.
 *
 * @param	message		Forward this message.
 * @param	sender		Neighbour who's sent us the message.
 * @param	global_state	The global state.
 *
 * @return	0		Success.
 * @return	1		Failure.
 */
int message_forward(const message_t	*message,
		    const neighbour_t	*sender,
		    global_state_t	*global_state)
{
	const message_body_t *msg_body;

	msg_body = &message->body;

	/* if all 'to' bytes are set to 0x0, the message is a broadcast */
	if (identifier_empty(msg_body->to)) {
		/* don't send the message to the one who's sent it to us */
		return message_broadcast_p2p(message,
					     &global_state->neighbours,
					     sender);
	}

	message_trace_store(&global_state->message_traces,
			    sender,
			    msg_body->nonce);

	return message_send_p2p(message, &global_state->routing_table);
}

/**
 * Send a message in a neighbour-to-neighbour manner.
 *
 * @param	message		The message to be sent.
 * @param	dest		Send the message to this neighbour.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
static int message_send_n2n(message_t *message, neighbour_t *dest)
{
	if (message_finalize(message,
			     &dest->my_pseudonym,
			     dest->pseudonym.identifier,
			     DV_HIDDEN)) {
		return 1;
	}

	return message_send_to_neighbour(message, dest);
}

/**
 * Send a p2p message towards its destination via next hop.
 *
 * @param	message		Send this p2p message. The message must contain
 *				destination's identifier for the next hop
 *				selection.
 * @param	routing_table	The routing table.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
static int message_send_p2p(const message_t	*message,
			    const linkedlist_t	*routing_table)
{
	neighbour_t		*next_hop;
	linkedlist_node_t	*next_hop_node;
	route_t			*route;

	route = route_find(routing_table, message->body.to);
	/* get the next hop or NULL if the message destination is unknown
	 * or if we have no next hop to reach the destination */
	next_hop_node = route ? linkedlist_get_first(&route->next_hops) : NULL;
	if (next_hop_node) {
		next_hop = (neighbour_t *) next_hop_node->data;

		/* send the message to the next hop */
		return message_send_to_neighbour(message, next_hop);
	}

	return 1;
}

/**
 * Send a message to a neighbour.
 *
 * @param	message		Send this message.
 * @param	dest		The neighbour.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
static int message_send_to_neighbour(const message_t	*message,
				     neighbour_t	*dest)
{
	char *json_message;

	if (encode_message(message, &json_message)) {
		log_debug("message_send_to_neighbour - encoding message");
		return 1;
	}

	string_send_to_neighbour(json_message, dest);

	free(json_message);
	return 0;
}

/**
 * A message trace staleness predicate.
 *
 * @param	msg_trace	The message trace.
 * @param	current_time	Time to compare to.
 *
 * @return	1		The message trace is stale.
 * @return	0		The message trace is not stale.
 */
int message_trace_is_stale(const message_trace_t *msg_trace,
			   const time_t		 *current_time)
{
	return difftime(*current_time, msg_trace->creation) >=
	       MESSAGE_TRACE_STALE_TIME;
}

/**
 * Store a new message trace.
 *
 * @param	msg_traces	Store to this container.
 * @param	sender		The neighbour whose message we've forwarded.
 * @param	nonce_value	The forwarded message's nonce value.
 *
 * @return	0		Successfully stored.
 * @return	1		Failure.
 */
static int message_trace_store(linkedlist_t		*msg_traces,
			       const neighbour_t	*sender,
			       uint64_t			nonce_value)
{
	message_trace_t	*msg_trace;

	msg_trace = (message_trace_t *) malloc(sizeof(message_trace_t));
	if (!msg_trace) {
		log_error("Allocating a message trace");
		return 1;
	}

	msg_trace->sender = sender;
	msg_trace->nonce_value = nonce_value;
	msg_trace->creation = time(NULL);

	if (!linkedlist_append(msg_traces, msg_trace)) {
		log_error("Storing a message trace");
		free(msg_trace);
		return 1;
	}

	return 0;
}

/**
 * Add a new route to the routing table.
 *
 * @param	routing_table	Add the new route in here.
 * @param	dest		Destination peer.
 * @param	next_hop	The neighbour who's announced the new route.
 *
 * @return	route_t		The new route.
 * @return	NULL		Failure.
 */
route_t *route_add(linkedlist_t		*routing_table,
		   peer_t		*dest,
		   neighbour_t		*next_hop)
{
	route_t	*new_route;

	new_route = (route_t *) malloc(sizeof(route_t));
	if (!new_route) {
		log_error("Creating a new route");
		return NULL;
	}

	linkedlist_init(&new_route->next_hops);
	if (!(new_route->node =
	      linkedlist_append(&new_route->next_hops, next_hop))) {
		free(new_route);
		log_error("Storing a new route");
		return NULL;
	}
	new_route->destination = dest;
	new_route->last_update = time(NULL);

	return new_route;
}

/**
 * Safely delete all route's data.
 *
 * @param	route		Delete this route's data.
 */
void route_clear(route_t *route)
{
	linkedlist_remove_all(&route->next_hops);
}

/**
 * Delete a route from a routing table.
 *
 * @param	routing_table	The routing table.
 * @param	dest_id		The route's destination id.
 */
void route_delete(linkedlist_t *routing_table, const unsigned char *dest_id)
{
	route_t *route;

	route = route_find(routing_table, dest_id);
	if (route) {
		linkedlist_delete_safely(route->node, route_clear);
	}
}

/**
 * Find a route in a routing table.
 *
 * @param	routing_table	The routing table.
 * @param	dest_id		The route's destination identififer.
 *
 * @return	route_t		The requested route.
 * @return	NULL		Route not found.
 */
route_t *route_find(const linkedlist_t	*routing_table,
		    const unsigned char	*dest_id)
{
	const unsigned char	*id;
	const linkedlist_node_t	*node;
	route_t			*route;

	/* get the first node of our routing table */
	node = linkedlist_get_first(routing_table);
	while (node) {
		route = (route_t *) node->data;
		id = route->destination->identifier;

		if (!memcmp(id, dest_id, crypto_box_PUBLICKEYBYTES)) {
			return route;
		}

		node = linkedlist_get_next(routing_table, node);
	}

	/* destination unknown */
	return NULL;
}

/**
 * A route staleness predicate.
 *
 * @param	route		The route.
 * @param	current_time	Time to compare to.
 *
 * @return	1		The route is stale.
 * @return	0		The route is not stale.
 */
int route_is_stale(const route_t *route, const time_t *current_time)
{
	return difftime(*current_time, route->last_update) >= ROUTE_STALE_TIME;
}

/**
 * Add new next hop, unless it's already added.
 *
 * @param	route		Add the next hop to this route.
 * @param	next_hop	Add this next hop.
 *
 * @return	0		Added or next hop has already been there.
 * @return	1		Failure.
 */
int route_next_hop_add(route_t *route, neighbour_t *next_hop)
{
	if (!linkedlist_find(&route->next_hops, next_hop)) {
		return linkedlist_append(&route->next_hops, next_hop) == NULL;
	}

	return 0;
}

/**
 * Remove a next hop from a route, if exists.
 *
 * @param	route		Remove the next hop from this route.
 * @param	next_hop	Remove this next hop.
 */
void route_next_hop_remove(route_t *route, neighbour_t *next_hop)
{
	linkedlist_node_t *node = linkedlist_find(&route->next_hops, next_hop);

	if (node) {
		linkedlist_remove(node);
	}
}

/**
 * Reset a route - remove its next hops and add a new one.
 *
 * @param	route		Reset this route.
 * @param	next_hop	The new next hop.
 *
 * @return	0		Success.
 * @return	1		Failure.
 */
int route_reset(route_t *route, neighbour_t *next_hop)
{
	linkedlist_remove_all(&route->next_hops);

	return linkedlist_append(&route->next_hops, next_hop) == NULL;
}

/**
 * Detect a routing loop.
 *
 * @param	msg_traces	Recent message traces.
 * @param	neighbour	The forwarder of a new message.
 * @param	nonce_value	The message's nonce value.
 *
 * @return	1		A routing loop detected.
 * @return	0		No routing loop detected.
 */
int routing_loop_detect(const linkedlist_t	*msg_traces,
			const neighbour_t	*neighbour,
			uint64_t		nonce_value)
{
	const message_trace_t	*msg_trace;
	const linkedlist_node_t	*node;

	node = linkedlist_get_first(msg_traces);
	while (node) {
		msg_trace = (message_trace_t *) node->data;

		/* the same message but different neighbour => routing loop */
		if (msg_trace->nonce_value == nonce_value &&
		    msg_trace->sender != neighbour) {
			return 1;
		}

		node = linkedlist_get_next(msg_traces, node);
	}

	return 0;
}

/**
 * Remove a routing loop from a route given by its destination's identifier.
 * Send p2p.route.sol if we've removed our last next hop.
 *
 * @param	routing_table	The routing table.
 * @param	neighbours	Our neighbours.
 * @param	identities	Our identities.
 * @param	dest_id		The identifier of the destination of the route
 *				that's caused a routing loop.
 */
void routing_loop_remove(linkedlist_t		*routing_table,
			 linkedlist_t		*neighbours,
			 linkedlist_t		*identities,
			 const unsigned char	*dest_id)
{
	identity_t		*identity;
	neighbour_t		*next_hop;
	linkedlist_node_t	*next_hop_node;
	route_t			*route;

	route = route_find(routing_table, dest_id);

	/* get the next hop or NULL if the message destination is unknown
	 * or if we have no next hop to reach the destination */
	next_hop_node = route ? linkedlist_get_first(&route->next_hops) : NULL;
	if (next_hop_node) {
		next_hop = (neighbour_t *) next_hop_node->data;
		route_next_hop_remove(route, next_hop);

		next_hop_node = linkedlist_get_first(&route->next_hops);
		if (!next_hop_node) {
			/* create a temporary identity for the p2p.route.sol */
			identity = identity_generate(IDENTITY_TMP);
			/* if appending succeeded */
			if (linkedlist_append(identities, identity)) {
				/* if successfully sent */
				if (!send_p2p_route_sol(neighbours,
						        identity,
						        dest_id)) {
					return;
				}
			}
			free(identity);
			route_delete(routing_table, dest_id);
		}
	}
}

/**
 * Remove a next hop from all routes.
 *
 * @param	routing_table		The routing table.
 * @param	next_hop		Remove this next hop.
 */
void routing_table_remove_next_hop(linkedlist_t *routing_table,
                                   neighbour_t  *next_hop)
{
	const linkedlist_node_t	*node;
	route_t			*route;

	node = linkedlist_get_first(routing_table);
	while (node) {
		route = (route_t *) node->data;

		route_next_hop_remove(route, next_hop);

		node = linkedlist_get_next(routing_table, node);
	}
}

/**
 * Send p2p.bye message.
 *
 * @param	neighbours	Our neighbours.
 * @param	identity	Announce departure of this identity.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
int send_p2p_bye(linkedlist_t *neighbours, identity_t *identity)
{
	message_t p2p_bye_msg;
	int	  ret;

	if (create_p2p_bye(&p2p_bye_msg)) {
		return 1;
	}
	if (!(ret = message_finalize(&p2p_bye_msg,
				     identity,
				     NULL,
				     DV_HIDDEN))) {
		/* route as P2P (don't use our neighbour pseudonym) */
		ret = message_broadcast_p2p(&p2p_bye_msg,
					    neighbours,
					    NULL);
	}

	message_delete(&p2p_bye_msg);
	return ret;
}

/**
 * Send p2p.hello message.
 *
 * @param	dest		Send the message to this neighbour.
 * @param	port		Our listening port.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
int send_p2p_hello(neighbour_t *dest, unsigned short port)
{
	message_t p2p_hello_msg;
	int	  ret;

	if (create_p2p_hello(&p2p_hello_msg, port)) {
		return 1;
	}
	ret = message_send_n2n(&p2p_hello_msg, dest);

	message_delete(&p2p_hello_msg);
	return ret;
}

/**
 * Send p2p.peers.adv message.
 *
 * @param	dest		Send the message to this neighbour.
 * @param	hosts		Hosts known to us.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
int send_p2p_peers_adv(neighbour_t *dest, const linkedlist_t *hosts)
{
	message_t p2p_peers_adv_msg;
	int	  ret;

	if (create_p2p_peers_adv(&p2p_peers_adv_msg, hosts)) {
		return 1;
	}
	ret = message_send_n2n(&p2p_peers_adv_msg, dest);

	message_delete(&p2p_peers_adv_msg);
	return ret;
}

/**
 * Send p2p.peers.sol message.
 *
 * @param	dest		Send the message to this neighbour.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
int send_p2p_peers_sol(neighbour_t *dest)
{
	message_t p2p_peers_sol_msg;
	int	  ret;

	if (create_p2p_peers_sol(&p2p_peers_sol_msg)) {
		return 1;
	}
	if (!(ret = message_send_n2n(&p2p_peers_sol_msg, dest))) {
		set_neighbour_flags(dest, NEIGHBOUR_ADDRS_REQ);
	}

	message_delete(&p2p_peers_sol_msg);
	return ret;
}

/**
 * Send p2p.ping message.
 *
 * @param	dest		Send the message to this neighbour.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
int send_p2p_ping(neighbour_t *dest)
{
	message_t p2p_ping_msg;
	int	  ret;

	if (create_p2p_ping(&p2p_ping_msg)) {
		return 1;
	}
	ret = message_send_n2n(&p2p_ping_msg, dest);

	message_delete(&p2p_ping_msg);
	return ret;
}

/**
 * Send p2p.pong message.
 *
 * @param	dest		Send the message to this neighbour.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
int send_p2p_pong(neighbour_t *dest)
{
	message_t p2p_pong_msg;
	int	  ret;

	if (create_p2p_pong(&p2p_pong_msg)) {
		return 1;
	}
	ret = message_send_n2n(&p2p_pong_msg, dest);

	message_delete(&p2p_pong_msg);
	return ret;
}

/**
 * Send p2p.route.adv message.
 *
 * @param	neighbours	Our neighbours.
 * @param	identity	Advertise this identity.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
int send_p2p_route_adv(linkedlist_t *neighbours, identity_t *identity)
{
	message_t p2p_route_adv_msg;
	int	  ret;

	if (create_p2p_route_adv(&p2p_route_adv_msg)) {
		return 1;
	}
	if (!(ret = message_finalize(&p2p_route_adv_msg,
				     identity,
				     NULL,
				     DV_HIDDEN))) {
		/* timing attack protection */
		execution_pause_random(250, 2000);

		/* route as P2P (don't use our neighbour pseudonym) */
		if (!(ret = message_broadcast_p2p(&p2p_route_adv_msg,
						  neighbours,
						  NULL))) {
			identity->last_adv = time(NULL);
		}
	}

	message_delete(&p2p_route_adv_msg);
	return ret;
}

/**
 * Send p2p.route.sol message.
 *
 * @param	neighbours	Our neighbours.
 * @param	identity	Make a solicitation under this identity.
 * @param	target		Destination peer's identifier.
 *
 * @return	0		Successfully sent.
 * @return	1		Failure.
 */
int send_p2p_route_sol(linkedlist_t	   *neighbours,
		       identity_t	   *identity,
		       const unsigned char *target)
{
	message_t p2p_route_sol_msg;
	int	  ret;

	if (create_p2p_route_sol(&p2p_route_sol_msg, target)) {
		return 1;
	}
	if (!(ret = message_finalize(&p2p_route_sol_msg,
				     identity,
				     NULL,
				     DV_HIDDEN))) {
		/* route as P2P (don't use our neighbour pseudonym) */
		ret = message_broadcast_p2p(&p2p_route_sol_msg,
					    neighbours,
					    NULL);
	}

	message_delete(&p2p_route_sol_msg);
	return ret;
}

/**
 * Send a text (json message) to a neighbour.
 *
 * @param	string		Send this text.
 * @param	dest		To this neighbour.
 */
static void string_send_to_neighbour(const char *string, neighbour_t *dest)
{
	struct evbuffer *output;

	output = bufferevent_get_output(dest->buffer_event);
	evbuffer_add_printf(output, "%s", string);
}
