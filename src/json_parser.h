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

#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "daemon_messages.h"
#include "trade.h"

/* the order of these strings must be the same as the order of enums within
 * enum message_type at daemon_messages.h */
static const char *msg_type_str[] = {
	/* encrypted */
	"encrypted",
	/* p2p */
	"p2p.bye",
	"p2p.hello",
	"p2p.peers.adv",
	"p2p.peers.sol",
	"p2p.ping",
	"p2p.pong",
	"p2p.route.adv",
	"p2p.route.sol"
};

/* the order of these strings must be the same as the order of enums within
 * enum payload_type at daemon_messages.h */
static const char *payload_type_str[] = {
	"trade.execution",
	"trade.proposal",
	"trade.reject"
};

int decode_message(const char	*json_message,
		   message_t	*message,
		   char		**json_body);
int decode_message_body(const char	*json_body,
			message_body_t	*body,
			char		**json_data);
int decode_message_data(const char		*json_data,
			const enum message_type	type,
			void			**data);

int decode_payload_type(const char		*json_payload,
			enum payload_type	*type,
			char			**json_data);
int decode_payload_data(const char		*json_data,
			enum payload_type	type,
			void			**data);
int decode_trade_execution(const char		*json_data,
			   enum trade_type	type,
			   enum trade_step	step,
			   trade_execution_t	**data);

int decode_trade(const char *json_trade, trade_t **trade);

int encode_message(const message_t *message, char **json_message);
int encode_message_body(const message_body_t *body, char **json_body);

int encode_payload(enum payload_type type, const void *data, char **encoded);
int encode_trade_execution(const trade_execution_t	*trade_execution,
			   enum trade_type		type,
			   enum trade_step		step,
			   char				**encoded);

int encode_trade(const trade_t *trade, char **json_trade);

#endif /* JSON_PARSER_H */
