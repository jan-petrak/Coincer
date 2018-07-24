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

#ifndef DAEMON_MESSAGES_H
#define DAEMON_MESSAGES_H

#include <sodium.h>
#include <stdint.h>

/** Types of daemon messages. */
enum message_type {
	P2P_PING,
	P2P_PONG
	/* TODO: and other message types */
};

/* TODO: define all structs of daemon messages in here */

typedef struct s_message_body {
	unsigned char		to[crypto_box_PUBLICKEYBYTES];
	enum message_type	type;
	void			*data;
	uint64_t		nonce;
} message_body_t;

typedef struct s_message {
	int		version;
	unsigned char	from[crypto_box_PUBLICKEYBYTES];
	message_body_t	body;
	unsigned char	sig[crypto_sign_BYTES];
} message_t;

#endif /* DAEMON_MESSAGES_H */
