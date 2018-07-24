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

static const char *msg_type_str[] = {
	"p2p.ping",
	"p2p.pong"
	/* TODO: and other string representations of daemon type messages */
};

int decode_message(const char *json_message, message_t *message);
int encode_message(const message_t *message, char **json_message);
int fetch_json_message_body(const char *json_message, char **json_message_body);

#endif /* JSON_PARSER_H */
