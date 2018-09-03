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

#include <sodium.h>
#include <stdlib.h>
#include <string.h>

#include "autoconfig.h"
#include "daemon_messages.h"
#include "hosts.h"
#include "log.h"

/**
 * Create a p2p.bye and store it in a message.
 *
 * @param	message		Store p2p.bye in this message.
 *
 * @return	0		Successfully created.
 */
int create_p2p_bye(message_t *message)
{
	message->body.type = P2P_BYE;
	message->body.data = NULL;

	return 0;
}

/**
 * Create a p2p.hello and store it in a message.
 *
 * @param	message		Store p2p.hello in this message.
 * @param	port		Our listening port.
 *
 * @return	0		Successfully created.
 * @return	1		Failure.
 */
int create_p2p_hello(message_t *message, unsigned short port)
{
	char		*client;
	p2p_hello_t	*hello;

	hello = (p2p_hello_t *) malloc(sizeof(p2p_hello_t));
	if (hello == NULL) {
		log_error("Creating p2p.hello");
		return 1;
	}

	client = (char *) malloc(sizeof("/" PACKAGE_NAME ":"
					PACKAGE_VERSION "/"));
	if (client == NULL) {
		log_error("Creating p2p.hello");
		return 1;
	}

	strcpy(client, "/" PACKAGE_NAME ":" PACKAGE_VERSION "/");

	hello->client = client;
	hello->port   = port;

	message->body.type = P2P_HELLO;
	message->body.data = hello;

	return 0;
}

/**
 * Create a p2p.peers.adv and store it in a message.
 *
 * @param	message		Store p2p.peers.adv in this message.
 * @param	host		List of known hosts.
 *
 * @return	0		Successfully created.
 * @return	1		Failure.
 */
int create_p2p_peers_adv(message_t *message, const linkedlist_t *hosts)
{
	p2p_peers_adv_t *peers_adv;

	peers_adv = (p2p_peers_adv_t *) malloc(sizeof(p2p_peers_adv_t));
	if (peers_adv == NULL) {
		log_error("Creating p2p.peers.adv");
		return 1;
	}

	hosts_to_str(hosts, &peers_adv->addresses);

	message->body.type = P2P_PEERS_ADV;
	message->body.data = peers_adv;

	return 0;
}

/**
 * Create a p2p.peers.sol and store it in a message.
 *
 * @param	message		Store p2p.peers.sol in this message.
 *
 * @return	0		Successfully created.
 */
int create_p2p_peers_sol(message_t *message)
{
	message->body.type = P2P_PEERS_SOL;
	message->body.data = NULL;

	return 0;
}

/**
 * Create a p2p.ping and store it in a message.
 *
 * @param	message		Store p2p.ping in this message.
 *
 * @return	0		Successfully created.
 */
int create_p2p_ping(message_t *message)
{
	message->body.type = P2P_PING;
	message->body.data = NULL;

	return 0;
}

/**
 * Create a p2p.pong and store it in a message.
 *
 * @param	message		Store p2p.pong in this message.
 *
 * @return	0		Successfully created.
 */
int create_p2p_pong(message_t *message)
{
	message->body.type = P2P_PONG;
	message->body.data = NULL;

	return 0;
}

/**
 * Create a p2p.route.adv and store it in a message.
 *
 * @param	message		Store p2p.route.adv in this message.
 *
 * @return	0		Successfully created.
 */
int create_p2p_route_adv(message_t *message)
{
	message->body.type = P2P_ROUTE_ADV;
	message->body.data = NULL;

	return 0;
}

/**
 * Create a p2p.route.sol and store it in a message.
 *
 * @param	message		Store p2p.route.sol in this message.
 * @param	target		Looking for a peer with this identifier.
 *
 * @return	0		Successfully created.
 * @return	1		Failure.
 */
int create_p2p_route_sol(message_t *message, const unsigned char *target)
{
	p2p_route_sol_t *route_sol;

	route_sol = (p2p_route_sol_t *) malloc(sizeof(p2p_route_sol_t));
	if (route_sol == NULL) {
		log_error("Creating p2p.route.sol");
		return 1;
	}

	memcpy(route_sol->target, target, crypto_box_PUBLICKEYBYTES);

	message->body.type = P2P_ROUTE_SOL;
	message->body.data = route_sol;

	return 0;
}

/**
 * Safely delete a message.
 *
 * @param	message		Delete this message.
 */
void message_delete(message_t *message)
{
	message_body_t *body = &message->body;

	switch (body->type) {
		case P2P_BYE:
			break;
		case P2P_HELLO:
			free(((p2p_hello_t *) body->data)->client);
			free(body->data);
			break;
		case P2P_PEERS_ADV:
			free(((p2p_peers_adv_t *) body->data)->addresses);
			free(body->data);
			break;
		case P2P_PEERS_SOL:
			break;
		case P2P_PING:
			break;
		case P2P_PONG:
			break;
		case P2P_ROUTE_ADV:
			break;
		case P2P_ROUTE_SOL:
			free(body->data);
			break;
		default:
			break;
	}
}
