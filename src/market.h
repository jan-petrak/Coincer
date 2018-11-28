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

#ifndef MARKET_H
#define MARKET_H

#include "crypto.h"
#include "linkedlist.h"
#include "peers.h"

/** Order that does not belong to us. */
#define ORDER_FOREIGN	0x01
/** Order that is being traded */
#define ORDER_TRADING	0x02

typedef struct s_order {
	unsigned char id[SHA3_256_SIZE];
	int flags;
	union owner {
		peer_t	   *cp;
		identity_t *me;
	} owner;
	linkedlist_t blacklist;
	linkedlist_node_t *node;
} order_t;

#endif /* MARKET_H */
