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

#include "daemon_messages.h"
#include "json_parser.h"
#include "log.h"
#include "trade.h"
#include "trade_basic.h"

/* TODO: Implement */
/**
 * Decode a JSON message into a daemon message.
 *
 * @param	json_message		Decode this JSON message.
 * @param	message			Store decoded data in here.
 * @param	json_body		Store the message's JSON body in here.
 *
 * @return	0			Decoding successful.
 * @return	1			Failure.
 */
int decode_message(const char	*json_message,
		   message_t	*message,
		   char		**json_body)
{
	char from_hex[65];
	char to_hex[65];
	char sig_hex[129];
	/* TODO: fill the above arrays with values from 'json_message' */

	/* after sodium_hex2bin, the unused bytes will be set to 0 */
	memset(message->from, 0x0, sizeof(message->from));
	memset(message->body.to, 0x0, sizeof(message->body.to));
	memset(message->sig, 0x0, sizeof(message->sig));

	/* if converting field 'from' to binary failed */
	if (sodium_hex2bin(message->from,
			   sizeof(message->from),
			   from_hex,
			   strlen(from_hex),
			   NULL, /* disallow non-hexa chars */
			   NULL, /* don't remember the number of used bytes */
			   NULL)) { /* we don't need the address of a byte
				     * after the last parsed char, hence NULL */
		log_debug("decode_message - 'from' to bin");
		return 1;
	}
	/* if converting field 'to' to binary failed */
	if (sodium_hex2bin(message->body.to,
			   sizeof(message->body.to),
			   to_hex,
			   strlen(to_hex),
			   NULL,
			   NULL,
			   NULL)) {
		log_debug("decode_message - 'to' to bin");
		return 1;
	}
	/* if converting field 'sig' to binary failed */
	if (sodium_hex2bin(message->sig,
			   sizeof(message->sig),
			   sig_hex,
			   strlen(sig_hex),
			   NULL,
			   NULL,
			   NULL)) {
		log_debug("decode_message - 'sig' to bin");
		return 1;
	}
	/* message->from, message->body.to, message->sig are set now */

	return 0;
}

/* TODO: Implement */
/**
 * Decode a JSON format message body into a daemon message body.
 *
 * @param	json_body	Decode this JSON message body.
 * @param	body		Store decoded body in here.
 * @param	json_data	Store the message's JSON data in here, if any.
 *				Otherwise set to NULL.
 *
 * @return	0		Decoding successful.
 * @return	1		Failure.
 */
int decode_message_body(const char	*json_body,
			message_body_t	*body,
			char		**json_data)
{
	*json_data = NULL;
	return 0;
}

/* TODO: Implement */
/**
 * Decode a JSON format message data into an appropriate structure.
 *
 * @param	json_data	Decode this JSON data. This can be NULL
 *				depending on message type.
 * @param	type		Type of the message data.
 * @param	data		Dynamically allocated structure storing the
 *				message data. NULL if the message has no data.
 *
 * @return	0		Decoding successful.
 * @return	1		Failure.
 */
int decode_message_data(const char		*json_data,
			const enum message_type	type,
			void			**data)
{
	*data = NULL;

	switch (type) {
		case P2P_BYE:
			return 0;
		case P2P_HELLO:
			if (json_data == NULL) {
				return 1;
			}
			*data = (p2p_hello_t *) malloc(sizeof(p2p_hello_t));
			if (!*data) {
				log_error("Allocating p2p.hello");
				return 1;
			}
			return 0;
		case P2P_PEERS_ADV:
			if (json_data == NULL) {
				return 1;
			}
			*data = (p2p_peers_adv_t *) malloc(
						sizeof(p2p_peers_adv_t));
			if (!*data) {
				log_error("Allocating p2p.peers.adv");
				return 1;
			}
			return 0;
		case P2P_PEERS_SOL:
			return 0;
		case P2P_PING:
			return 0;
		case P2P_PONG:
			return 0;
		case P2P_ROUTE_ADV:
			return 0;
		case P2P_ROUTE_SOL:
			if (json_data == NULL) {
				return 1;
			}
			*data = (p2p_route_sol_t *) malloc(
						sizeof(p2p_route_sol_t));
			if (!*data) {
				log_error("Allocating p2p.route.sol");
				return 1;
			}
			return 0;
		default:
			log_error("Unknown message type parsing");
			return 1;
	}

	return 1;
}

/* TODO: Implement */
/**
 * Decode a JSON trade.
 *
 * @param	json_trade	Encoded JSON trade.
 * @param	trade		Internal trade representation, dynamically
 *				allocated in here.
 *
 * @return	0		Successfully decoded.
 * @return	1		Failure.
 */
int decode_trade(const char *json_trade, trade_t **trade)
{
	return 0;
}

/* TODO: Implement */
/**
 * Encode a daemon message into JSON format output.
 *
 * @param	message		Message to be encoded.
 * @param	json_message	Dynamically allocated string of JSON format.
 *
 * @return	0		Encoding successful.
 * @return	1		Failure.
 */
int encode_message(const message_t *message, char **json_message)
{
	char from_hex[65];
	char to_hex[65];
	char sig_hex[129];

	/* sodium_bin2hex also appends '\0' */

	/* if converting field 'from' to hexadecimal failed */
	if (!sodium_bin2hex(from_hex,
			    sizeof(from_hex),
			    message->from,
			    sizeof(message->from))) {
		log_debug("encode_message - 'from' to hex");
		return 1;
	}
	/* if converting field 'to' to hexadecimal failed */
	if (!sodium_bin2hex(to_hex,
			    sizeof(to_hex),
			    message->body.to,
			    sizeof(message->body.to))) {
		log_debug("encode_message - 'to' to hex");
		return 1;
	}
	/* if converting field 'sig' to hexadecimal failed */
	if (!sodium_bin2hex(sig_hex,
			    sizeof(sig_hex),
			    message->sig,
			    sizeof(message->sig))) {
		log_debug("encode_message - 'sig' to hex");
		return 1;
	}
	/* from_hex, to_hex, sig_hex are initialized now, ready to be
	 * included in output 'json_message' */

	return 0;
}

/* TODO: Implement */
/**
 * Encode a daemon message body into JSON format message body.
 *
 * @param	body		Message body to be encoded.
 * @param	json_body	Dynamically allocated string
 *				of JSON message body.
 *
 * @return	0		Encoding successful.
 * @return	1		Failure.
 */
int encode_message_body(const message_body_t *body, char **json_body)
{
	return 0;
}

/* TODO: Implement */
/**
 * Encode a trade into JSON.
 *
 * @param	trade		Trade to encode.
 * @param	json_trade	Encoded JSON trade.
 *
 * @return	0		Successfully encoded.
 * @return	1		Failure.
 */
int encode_trade(const trade_t *trade, char **json_trade)
{
	return 0;
}
