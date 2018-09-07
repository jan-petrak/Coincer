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

/* the order of these strings must be the same as the order of enums within
 * enum message_type at daemon_messages.h */
static const char *msg_type_str[] = {
	"p2p.bye",
	"p2p.hello",
	"p2p.peers.adv",
	"p2p.peers.sol"
	"p2p.ping",
	"p2p.pong",
	"p2p.route.adv",
	"p2p.route.sol"
};

int decode_message(const char *json_message, message_t *message);
int decode_message_body(const char *json_body, message_body_t *body);
int encode_message(const message_t *message, char **json_message);
int encode_message_body(const message_body_t *body, char **json_body);

#endif /* JSON_PARSER_H */
