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

#ifndef P2P_H
#define P2P_H

#include <event2/listener.h>
#include <netinet/in.h>

#include "global_state.h"

/** Default port for TCP listening. */
#define	DEFAULT_PORT	31070
/** Minimum number of peers we need to be connected to. */
#define MIN_NEIGHBOURS	3
/** After TIMEOUT_TIME seconds invoke timeout callback. */
#define	TIMEOUT_TIME 	30

void add_more_connections(global_state_t *global_state, size_t conns_amount);

int connect_to_addr(global_state_t		*global_state,
		    const struct in6_addr	*addr);

int listen_init(struct evconnlistener	**listener,
		global_state_t		*global_state);

#endif /* P2P_H */
