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

#include <netinet/in.h>
#include <sodium.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crypto.h"
#include "daemon_messages.h"
#include "daemon_messages_processor.h"
#include "global_state.h"
#include "hosts.h"
#include "json_parser.h"
#include "log.h"
#include "neighbours.h"
#include "routing.h"

/** The minimum time in seconds that has to pass between the announcements of
 *  presence of one identity. */
#define ADV_GAP_TIME	10

static enum process_message_result
	process_message(message_t	*message,
			const char	*json_message,
			const char	*json_data,
			neighbour_t	*sender,
			global_state_t	*global_state);

static int process_p2p_bye(const message_t	*message,
			   const char		*json_message,
			   neighbour_t		*sender,
			   peer_t		*sender_peer,
			   global_state_t	*global_state);
static int process_p2p_hello(const message_t	*message,
			     neighbour_t	*sender,
			     linkedlist_t	*neighbours,
			     unsigned short	port);
static int process_p2p_peers_adv(const message_t *message,
				 neighbour_t	 *sender,
				 linkedlist_t	 *hosts);
static int process_p2p_peers_sol(neighbour_t	    *sender,
				 const linkedlist_t *hosts);
static int process_p2p_ping(neighbour_t *sender);
static int process_p2p_pong(neighbour_t *sender);
static int process_p2p_route_adv(const message_t *message,
				 const char	 *json_message,
				 neighbour_t	 *sender,
				 peer_t		 *sender_peer,
				 global_state_t	 *global_state);
static int process_p2p_route_sol(const message_t *message,
				 const char	 *json_message,
				 neighbour_t	 *sender,
				 global_state_t	 *global_state);

/**
 * Process a JSON message received from its forwarder/sender (our neighbour).
 *
 * @param	json_message		The received JSON message.
 * @param	sender			The message sender/forwarder.
 * @param	global_state		The global state.
 *
 * @return	PMR_DONE		The received message was successfully
 *					processed.
 * @return	PMR_ERR_INTEGRITY	Someone tampered with the message.
 * @return	PMR_ERR_INTERNAL	Internal processing error.
 * @return	PMR_ERR_PARSING		Parsing failure.
 * @return	PMR_ERR_SEMANTIC	Semantic error.
 * @return	PMR_ERR_VERSION		The message is of different
 *					protocol version.
 */
enum process_message_result
	process_encoded_message(const char	*json_message,
				neighbour_t	*sender,
				global_state_t	*global_state)
{
	char				*json_message_body;
	message_t			message;
	char				*json_data;
	enum process_message_result	ret;

	/* decode JSON message; if parsing JSON message into message_t failed */
	if (decode_message(json_message, &message, &json_message_body)) {
		log_debug("process_encoded_message - decoding a received "
			  "message has failed. The message:\n%s", json_message);
		return PMR_ERR_PARSING;
	}

	if (message.version != PROTOCOL_VERSION) {
		free(json_message_body);
		return PMR_ERR_VERSION;
	}

	/* integrity verification part; if the message integrity is violated */
	if (verify_signature(json_message_body, message.from, message.sig)) {
		log_warn("Someone tampered with a received message");
		log_debug("The tampered message:\n%s", json_message);
		free(json_message_body);
		return PMR_ERR_INTEGRITY;
	}
	/* message body intact; decode it */
	if (decode_message_body(json_message_body,
				&message.body,
				&json_data)) {
		log_debug("process_encoded_message - decoding message body has "
			  "failed. The message body:\n%s", json_message_body);
		free(json_message_body);
		return PMR_ERR_PARSING;
	}
	free(json_message_body);

	/* process the message */
	ret = process_message(&message,
			      json_message,
			      json_data,
			      sender,
			      global_state);

	if (json_data != NULL) {
		free(json_data);
	}
	message_delete(&message);
	return ret;
}

/**
 * Process a message received from its forwarder/sender (our neighbour).
 *
 * @param	message			The received message.
 * @param	json_message		The received message in JSON format.
 * @param	json_data		The received message's JSON data.
 * @param	sender			The message sender/forwarder.
 * @param	global_state		The global state.
 *
 * @return	PMR_DONE		The received message was successfully
 *					processed.
 * @return	PMR_ERR_INTERNAL	Internal processing error.
 * @return	PMR_ERR_PARSING		Parsing failure.
 * @return	PMR_ERR_SEMANTIC	Semantic error.
 */
static enum process_message_result
	process_message(message_t	*message,
			const char	*json_message,
			const char	*json_data,
			neighbour_t	*sender,
			global_state_t	*global_state)
{
	int			cmp_val;
	linkedlist_t		*hosts;
	linkedlist_t		*identities;
	identity_t		*identity;
	message_body_t		*msg_body;
	const linkedlist_t	*msg_traces;
	enum message_type	msg_type;
	linkedlist_t		*neighbours;
	uint64_t		nonce_value;
	linkedlist_t		*peers;
	int			res;
	linkedlist_t		*routing_table;
	peer_t			*sender_peer;

	msg_body    = &message->body;
	msg_type    = msg_body->type;
	nonce_value = msg_body->nonce;

	hosts	      = &global_state->hosts;
	identities    = &global_state->identities;
	msg_traces    = &global_state->message_traces;
	neighbours    = &global_state->neighbours;
	peers	      = &global_state->peers;
	routing_table = &global_state->routing_table;

	/* if we haven't yet received p2p.hello from the neighbour who's
	 * sent/forwarded the received message */
	if (!(sender->flags & NEIGHBOUR_ACTIVE)) {
		/* and the message is not p2p.hello */
		if (msg_type != P2P_HELLO) {
			log_debug("process_message - received non-hello msg "
				  "before p2p.hello");
			return PMR_ERR_SEMANTIC;
		}

		if (decode_message_data(json_data, msg_type, &msg_body->data)) {
			log_debug("process_message - decoding message data "
				  "has failed. The data:\n%s", json_data);
			return PMR_ERR_PARSING;
		}

		res = process_p2p_hello(message,
					sender,
					neighbours,
					global_state->port);
		if (!res) {
			nonce_store(&sender->pseudonym.nonces, nonce_value);
			return PMR_DONE;
		}

		return PMR_ERR_INTERNAL;
	}

	/* if we've received a message that we've created */
	if (identity_find(identities, message->from)) {
		/* and if it is a p2p message */
		if (!identifier_empty(msg_body->to)) {
			routing_loop_remove(routing_table,
					    neighbours,
					    identities,
					    msg_body->to);
		}
		return PMR_DONE;
	}

	/* let's get peer representations of the sender and appropriate ours */

	cmp_val	    = 0;
	sender_peer = NULL;
	/* all bytes of 'to' are set to 0x0 => we are one of the recipients */
	if (identifier_empty(msg_body->to)) {
		/* 'from' different from the neighbour's pseudonym =>
		 *  the message should be a broadcast (we will check later) */
		if (memcmp(message->from,
			   sender->pseudonym.identifier,
			   PUBLIC_KEY_SIZE)) {
			/* process the broadcast with the true identity */
			identity = global_state->true_identity;
		/* otherwise it is a n2n message */
		} else {
			identity = &sender->my_pseudonym;
			sender_peer = &sender->pseudonym;

			/* for later parity check */
			cmp_val = memcmp(message->from,
					 identity->keypair.public_key,
					 PUBLIC_KEY_SIZE);
		}
	/* the message is of type p2p */
	} else {
		/* identity will be NULL if the message is not meant for us */
		identity = identity_find(identities, msg_body->to);
		/* for later parity check */
		cmp_val = memcmp(message->from, msg_body->to, PUBLIC_KEY_SIZE);
	}

	/* nonce parity check; the message must have:
	 * sender ID > our ID => odd nonce; sender ID < our ID => even nonce */
	if ((cmp_val > 0 && !(nonce_value & 0x01)) ||
	    (cmp_val < 0 && (nonce_value & 0x01))) {
		log_debug("process_mesage - wrong nonce parity");
		return PMR_ERR_SEMANTIC;
	}

	/* if the message is not n2n */
	if (!sender_peer) {
		/* from what peer is this message? */
		sender_peer = peer_find(peers, message->from);
	}
	/* if we don't know this peer yet */
	if (!sender_peer) {
		sender_peer = peer_store(peers, message->from);
		/* storing the new peer has failed */
		if (!sender_peer) {
			return PMR_ERR_INTERNAL;
		}
	} else {
		if (msg_type == P2P_ROUTE_ADV) {
			/* if it's an old announcement of presence, skip it */
			if (nonce_value < sender_peer->presence_nonce.value) {
				return PMR_DONE;
			}
		} else {
			/* have we already processed this message? */
			if (nonces_find(&sender_peer->nonces, nonce_value)) {
				/* if broadcast or n2n msg */
				if (identifier_empty(msg_body->to) ||
				    identity == &sender->my_pseudonym) {
					return PMR_DONE;
				}
				/* check if there's a routing loop */
				if (routing_loop_detect(msg_traces,
							sender,
							nonce_value,
							message->from)) {
					routing_loop_remove(routing_table,
							    neighbours,
							    identities,
							    msg_body->to);
				}
				return PMR_DONE;
			}
			/* if the message smells like replay attack */
			if (!linkedlist_empty(&sender_peer->nonces) &&
			    nonce_value <
			    (nonces_get_oldest(&sender_peer->nonces))->value) {
				log_warn("Potential replay attack detected");
				return PMR_ERR_SEMANTIC;
			}
		}
	}

	/* no corresponding identity to process the message => the message is
	 * not meant for us */
	if (!identity) {
		/* if forwarding succeeded */
		if (!message_forward(message,
				     json_message,
				     sender,
				     global_state)) {
			/* store message's nonce */
			nonce_store(&sender_peer->nonces, nonce_value);
		}
		return PMR_DONE;
	} else if (identity->flags & IDENTITY_TMP) {
		/* no one is supposed to send a message to this identity */
		return PMR_ERR_SEMANTIC;
	}

	/* the message is meant for us; process it */

	if (decode_message_data(json_data, msg_type, &msg_body->data)) {
		log_debug("process_encoded_message - decoding message data "
			  "has failed. The data:\n%s", json_data);
		return PMR_ERR_PARSING;
	}

	switch (msg_type) {
		case P2P_BYE:
			res = process_p2p_bye(message,
					      json_message,
					      sender,
					      sender_peer,
					      global_state);
			break;
		case P2P_HELLO:
			res = process_p2p_hello(message,
						sender,
						neighbours,
						global_state->port);
			break;
		case P2P_PEERS_ADV:
			/* if the message was not sent by the same peer
			 * pseudonym of the sender as their p2p.hello */
			if (&sender->pseudonym != sender_peer) {
				log_debug("process_message - incorrect sender");
				/* since the message was sent by other ID than
				 * neighbour's pseudonym, we've treated it as a
				 * new peer; that's unwanted for n2n messages */
				peer_delete(sender_peer);
				return PMR_ERR_SEMANTIC;
			}
			res = process_p2p_peers_adv(message, sender, hosts);
			break;
		case P2P_PEERS_SOL:
			res = process_p2p_peers_sol(sender, hosts);
			break;
		case P2P_PING:
			res = process_p2p_ping(sender);
			break;
		case P2P_PONG:
			if (&sender->pseudonym != sender_peer) {
				log_debug("process_message - incorrect sender");
				peer_delete(sender_peer);
				return PMR_ERR_SEMANTIC;
			}
			res = process_p2p_pong(sender);
			break;
		case P2P_ROUTE_ADV:
			res = process_p2p_route_adv(message,
						    json_message,
						    sender,
						    sender_peer,
						    global_state);
			break;
		case P2P_ROUTE_SOL:
			res = process_p2p_route_sol(message,
						    json_message,
						    sender,
						    global_state);
			break;
		default:
			log_debug("process_message - unknown message type");
			return PMR_ERR_SEMANTIC;
	}

	/* if the message processing has failed */
	if (res) {
		return PMR_ERR_INTERNAL;
	}

	/* sucessfully processed; store message's nonce */
	if (msg_type == P2P_ROUTE_ADV) {
		presence_nonce_store(sender_peer, nonce_value);
	} else if (msg_type != P2P_BYE) {
		nonce_store(&sender_peer->nonces, nonce_value);
	}

	sender->failed_pings = 0;

	return PMR_DONE;
}

/**
 * Process p2p.bye.
 *
 * @param	message		Process this message.
 * @param	json_message	The received message in the JSON format.
 * @param	sender		We've received the message from this neighbour.
 * @param	sender_peer	Sender's peer representation.
 * @param	global_state	The global state.
 *
 * @return	0		Successfully processed.
 */
static int process_p2p_bye(const message_t	*message,
			   const char		*json_message,
			   neighbour_t		*sender,
			   peer_t		*sender_peer,
			   global_state_t	*global_state)
{
	route_delete(&global_state->routing_table, message->from);
	peer_delete(sender_peer);

	message_forward(message, json_message, sender, global_state);

	return 0;
}

/**
 * Process p2p.hello.
 *
 * @param	message		Process this message.
 * @param	sender		We've received the message from this neighbour.
 * @param	neighbours	Our neighbours.
 * @param	port		Our listening port.
 *
 * @return	0		Successfully processed.
 * @return	1		Failure.
 */
static int process_p2p_hello(const message_t	*message,
			     neighbour_t	*sender,
			     linkedlist_t	*neighbours,
			     unsigned short	port)
{
	p2p_hello_t *hello;
	neighbour_t *neighbour;

	/* don't take hello from already active neighbour */
	if (sender->flags & NEIGHBOUR_ACTIVE) {
		return 0;
	}

	/* check self-neighbouring; if we have a neighbour with 'my_pseudonym'
	 * the same as the received message's sender ID, then we've
	 * detected self-neighbouring; delete the mentioned neighbour,
	 * and also delete the message sender (our neighbour representation of
	 * the sending counterparty)
	 */
	if ((neighbour = find_neighbour(neighbours,
				message->from,
				compare_neighbour_my_pseudonyms))) {
		linkedlist_delete_safely(neighbour->node, clear_neighbour);
		linkedlist_delete_safely(sender->node, clear_neighbour);
		return 0;
	}

	hello = (p2p_hello_t *) message->body.data;

	sender->client = (char *) malloc((strlen(hello->client) + 1) *
					 sizeof(char));
	if (!sender->client) {
		log_error("Processing p2p.hello");
		return 1;
	}
	strcpy(sender->client, hello->client);

	/* if the new neighbour is not one of the default hosts */
	if (sender->host) {
		sender->host->port = hello->port;
	}

	memcpy(sender->pseudonym.identifier, message->from, PUBLIC_KEY_SIZE);

	set_neighbour_flags(sender, NEIGHBOUR_ACTIVE);

	return send_p2p_hello(sender, port);
}

/**
 * Process p2p.peers.adv.
 *
 * @param	message		Process this message.
 * @param	sender		We've received the message from this neighbour.
 * @param	hosts		Our known hosts.
 *
 * @return	0		Successfully processed.
 */
static int process_p2p_peers_adv(const message_t *message,
				 neighbour_t	 *sender,
				 linkedlist_t	 *hosts)
{
	struct in6_addr	addr;
	char		addr_str[INET6_ADDRSTRLEN];
	size_t		n;
	p2p_peers_adv_t	*peers_adv;
	unsigned short	port;
	char		*pos;
	char		tuple[55]; /* string address	 = 45 chars,
				    * string port	 = 5  chars,
				    * ',' + 3x' ' + '\0' = 5  chars,
				    * total		 = 55 chars
				    */

	/* if we haven't asked this neighbour for addresses */
	if (!(sender->flags & NEIGHBOUR_ADDRS_REQ)) {
		log_debug("process_p2p_peers_adv - unwanted addrs arrived");
		return 0;
	}
	unset_neighbour_flags(sender, NEIGHBOUR_ADDRS_REQ);

	peers_adv = (p2p_peers_adv_t *) message->body.data;

	/* if the list doesn't have enough chars to be even empty ("[  ]",
	 * which is 4 chars) */
	if (strlen(peers_adv->addresses) < 4) {
		log_debug("process_p2p_peers_adv - wrong addrs format");
		return 0;
	}

	/* set initial pointer position; skip the outer list '[' */
	pos = peers_adv->addresses + 1;
	/* get position of the first '[' */
	pos = strchr(pos, '[');

	/* while there is a tuple to be processed */
	while (pos) {
		/* get the number of chars between the current '[' and the next
		 * ']', both boundary chars excluding */
		n = strcspn(++pos, "]");

		/* the tuple can not have more than 54 chars (plus '\0') */
		if (n > 54) {
			log_debug("process_p2p_peers_adv - wrong addrs format");
			return 0;
		}

		/* copy the chars between '[' and ']', both boundary
		 * chars excluding */
		strncpy(tuple, pos, n);
		tuple[n] = '\0';

		/* get string addr and numerical port from the tuple */
		if (sscanf(tuple, " %[^,], %hu ", addr_str, &port) != 2) {
			log_debug("process_p2p_peers_adv - wrong addrs format");
			return 0;
		}

		/* if conversion of addr_str into addr succeeded */
		if (inet_pton(AF_INET6, addr_str, &addr) == 1) {
			save_host(hosts, &addr, port, HOST_AVAILABLE);
		}

		/* go to next tuple */
		pos = strchr(pos, '[');
	}

	return 0;
}

/**
 * Process p2p.peers.sol.
 *
 * @param	sender		We've received the message from this neighbour.
 * @param	hosts		Hosts known to us.
 *
 * @return	0		Successfully processed.
 * @return	1		Failure.
 */
static int process_p2p_peers_sol(neighbour_t *sender, const linkedlist_t *hosts)
{
	return send_p2p_peers_adv(sender, hosts);
}

/**
 * Process p2p.ping.
 *
 * @param	sender		We've received the message from this neighbour.
 *
 * @return	0		Successfully processed.
 * @return	1		Failure.
 */
static int process_p2p_ping(neighbour_t *sender)
{
	return send_p2p_pong(sender);
}

/**
 * Process p2p.pong.
 *
 * @param	sender		We've received the message from this neighbour.
 *
 * @return	0		Successfully processed.
 */
static int process_p2p_pong(neighbour_t *sender)
{
	sender->failed_pings = 0;

	return 0;
}

/**
 * Process p2p.route.adv.
 *
 * @param	message		Process this message.
 * @param	json_message	The received message in the JSON format.
 * @param	sender		We've received the message from this neighbour.
 * @param	sender_peer	Sender's peer representation.
 * @param	global_state	The global state.
 *
 * @return	0		Successfully processed.
 * @return	1		Failure.
 */
static int process_p2p_route_adv(const message_t *message,
				 const char	 *json_message,
				 neighbour_t	 *sender,
				 peer_t		 *sender_peer,
				 global_state_t	 *global_state)
{
	route_t		*route;
	linkedlist_t	*routing_table;

	routing_table = &global_state->routing_table;

	route = route_find(routing_table, message->from);
	if (!route) {
		if (!(route = route_add(routing_table, sender_peer, sender))) {
			log_error("Adding a new route");
			return 1;
		}
	}

	route->last_update = time(NULL);

	/* if we are dealing with the newest p2p.route.adv */
	if (message->body.nonce > route->destination->presence_nonce.value) {
		if (route_reset(route, sender)) {
			log_error("Reseting a route");
			return 1;
		}
	/* otherwise just add another next hop */
	} else if (route_next_hop_add(route, sender)) {
		log_error("Adding a new next hop");
		return 1;
	}

	message_forward(message, json_message, sender, global_state);
	return 0;
}

/**
 * Process p2p.route.sol.
 *
 * @param	message		Process this message.
 * @param	json_message	The received message in the JSON format.
 * @param	sender		We've received the message from this neighbour.
 * @param	global_state	The global state.
 *
 * @return	0		Successfully processed.
 * @return	1		Failure.
 */
static int process_p2p_route_sol(const message_t *message,
				 const char	 *json_message,
				 neighbour_t	 *sender,
				 global_state_t	 *global_state)
{
	identity_t 	*identity;
	p2p_route_sol_t *route_sol;

	route_sol = (p2p_route_sol_t *) message->body.data;
	identity = identity_find(&global_state->identities, route_sol->target);

	/* if we wouldn't rebroadcast the p2p.route.sol, and just sent
	 * a p2p.route.adv, it could be obvious the 'target' is us */
	if (message_forward(message,
			    json_message,
			    sender,
			    global_state)) {
		return 1;
	}
	/* if someone's looking for us */
	if (identity) {
		if (difftime(time(NULL),
			     identity->last_adv) > ADV_GAP_TIME) {
			/* broadcast the identity */
			return send_p2p_route_adv(
					&global_state->neighbours,
					identity);
		}
		/* returning 1 means not storing the message's nonce,
		 * and so if we receive this message again, we can
		 * re-check the p2p.route.adv gap time */
		return 1;
	}

	return 0;
}
