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

#include "autoconfig.h"
#include "crypto.h"
#include "daemon_messages.h"
#include "hosts.h"
#include "log.h"
#include "trade.h"
#include "trade_basic.h"

static int create_trade_execution_basic(trade_execution_basic_t **basic,
					const trade_t		*trade);

/**
 * Create encrypted container and store it in a message.
 *
 * @param	message			Store encrypted in this message.
 * @param	encrypted_payload	Encrypted data.
 *
 * @return	0			Successfully created.
 * @return	1			Failure.
 */
int create_encrypted(message_t *message, const char *encrypted_payload)
{
	encrypted_t *encrypted;
	char	    *payload;

	encrypted = (encrypted_t *) malloc(sizeof(encrypted_t));
	if (!encrypted) {
		log_error("Creating encrypted");
		return 1;
	}
	payload = (char *) malloc((strlen(encrypted_payload) + 1) *
				  sizeof(char));
	if (!payload) {
		log_error("Creating encrypted payload");
		free(encrypted);
		return 1;
	}
	strcpy(payload, encrypted_payload);

	encrypted->payload = payload;

	message->body.type = ENCRYPTED;
	message->body.data = encrypted;

	return 0;
}

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
	if (!hello) {
		log_error("Creating p2p.hello");
		return 1;
	}

	client = (char *) malloc(sizeof("/" PACKAGE_NAME ":"
					PACKAGE_VERSION "/"));
	if (!client) {
		log_error("Creating p2p.hello");
		free(hello);
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
	if (!peers_adv) {
		log_error("Creating p2p.peers.adv");
		return 1;
	}

	if (hosts_to_str(hosts, &peers_adv->addresses)) {
		log_error("Creating p2p.peers.adv");
		return 1;
	}

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
	if (!route_sol) {
		log_error("Creating p2p.route.sol");
		return 1;
	}

	memcpy(route_sol->target, target, PUBLIC_KEY_SIZE);

	message->body.type = P2P_ROUTE_SOL;
	message->body.data = route_sol;

	return 0;
}

/**
 * Create trade.execution payload.
 *
 * @param	execution	Store data to this trade.execution.
 * @param	trade		Get data from this trade.
 *
 * @return	0		Successfully created.
 * @return	1		Failure.
 */
int create_trade_execution(trade_execution_t **execution, const trade_t *trade)
{
	int ret = 1;

	*execution = malloc(sizeof(trade_execution_t));
	if (!*execution) {
		log_error("Creating trade.execution");
		return 1;
	}

	memcpy((*execution)->order, trade->order->id, SHA3_256_SIZE);

	switch (trade->type) {
		case TT_BASIC:
			ret = create_trade_execution_basic(&(*execution)->data,
							   trade);
	}

	if (ret) {
		free(*execution);
	}

	return ret;
}

/**
 * Create trade.execution of type basic.
 *
 * @param	basic		Basic execution data holder to be dynamically
 *				allocated and initialized in here.
 * @param	trade		Trade to create trade.execution for.
 *
 * @return	0		Successfully created.
 * @return	1		Failure.
 */
static int create_trade_execution_basic(trade_execution_basic_t **basic,
					const trade_t		*trade)
{
	trade_execution_basic_t	*exec_data;
	char			new_id_hex[2 * PUBLIC_KEY_SIZE + 1];
	char			*script;
	trade_basic_t		*trade_data;

	trade_data = (trade_basic_t *) trade->data;

	exec_data = (trade_execution_basic_t *) malloc(
					sizeof(trade_execution_basic_t));
	if (!exec_data) {
		log_error("Creating trade execution data");
		return 1;
	}

	switch (trade->step) {
		case TS_COMMITMENT:
			memcpy(exec_data->commitment,
			       trade_data->my_commitment,
			       SHA3_256_SIZE);

			sodium_bin2hex(new_id_hex,
				       sizeof(new_id_hex),
				       trade->my_identity->keypair.public_key,
				       PUBLIC_KEY_SIZE);
			sign_message(new_id_hex,
				     trade->order->owner.me->keypair.secret_key,
				     exec_data->idsig);
			break;
		case TS_KEY_AND_COMMITTED_EXCHANGE:
			memcpy(exec_data->pubkey,
			       trade->my_keypair.public_key,
			       PUBLIC_KEY_SIZE);
			memcpy(exec_data->committed,
			       trade_data->my_committed,
			       TRADE_BASIC_COMMITTED_SIZE);
			if (!trade_data->my_script) {
				break;
			}
		case TS_SCRIPT_ORIGIN:
			script = (char *) malloc(
					(strlen(trade_data->my_script) + 1) *
					sizeof(char));
			if (!script) {
				log_error("Script for trade.execution message");
				free(exec_data);
				return 1;
			}
			strcpy(script, trade_data->my_script);
			exec_data->script = script;

			memcpy(exec_data->hx, trade_data->hx, RIPEMD_160_SIZE);
			break;
		case TS_SCRIPT_RESPONSE:
			script = (char *) malloc(
					(strlen(trade_data->my_script) + 1) *
					sizeof(char));
			if (!script) {
				log_error("Script for trade.execution message");
				free(exec_data);
				return 1;
			}
			strcpy(script, trade_data->my_script);
			exec_data->script = script;
			break;
		default:
			log_error("Non-existent step for trade.execution "
				  "creation");
			free(exec_data);
			return 1;
	}

	*basic = exec_data;

	return 0;
}

/**
 * Create trade.proposal payload.
 *
 * @param	trade_proposal	trade.proposal to be created.
 * @param	trade		trade.proposal of this trade.
 *
 * @return	0		Successfully created.
 * @return	1		Failure.
 */
int create_trade_proposal(trade_proposal_t	**trade_proposal,
			  const trade_t		*trade)
{
	trade_proposal_t *proposal;

	proposal = (trade_proposal_t *) malloc(sizeof(trade_proposal_t));
	if (!proposal) {
		log_error("Creating trade.proposal");
		return 1;
	}

	*trade_proposal = proposal;

	trade_proposal_init(proposal, trade);

	return 0;
}

/**
 * Create trade.reject payload.
 *
 * @param	trade_reject	trade.reject to be created.
 * @param	order_id	trade.reject's order ID.
 *
 * @return	0		Successfully created.
 * @return	1		Failure.
 */
int create_trade_reject(trade_reject_t		**trade_reject,
			const unsigned char	*order_id)
{
	trade_reject_t *reject;

	reject = (trade_reject_t *) malloc(sizeof(trade_reject_t));
	if (!reject) {
		log_error("Creating trade.reject");
		return 1;
	}

	*trade_reject = reject;

	memcpy(reject->order, order_id, SHA3_256_SIZE);

	return 0;
}

/**
 * Safely delete a message.
 *
 * @param	message		Delete this message.
 */
void message_delete(message_t *message)
{
	void *data = message->body.data;

	if (!data) {
		return;
	}

	switch (message->body.type) {
		case ENCRYPTED:
			free(((encrypted_t *) data)->payload);
			free(data);
		case P2P_BYE:
			break;
		case P2P_HELLO:
			free(((p2p_hello_t *) data)->client);
			free(data);
			break;
		case P2P_PEERS_ADV:
			free(((p2p_peers_adv_t *) data)->addresses);
			free(data);
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
			free(data);
			break;
		default:
			break;
	}
}

/**
 * Safely delete a message payload.
 *
 * @param	type		Type of the message payload.
 * @param	data		Payload's data.
 */
void payload_delete(enum payload_type type, void *data)
{
	switch (type) {
		case TRADE_PROPOSAL:
			free(data);
			break;
		case TRADE_REJECT:
			free(data);
			break;
	}
}
