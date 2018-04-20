/*
 *  Coincer
 *  Copyright (C) 2017  Coincer Developers
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

#include <event2/event.h>

#include "linkedlist.h"

#define	DEFAULT_PORT	31070
/* after (READ/WRITE)_TIMEOUT seconds invoke timeout callback */
#define READ_TIMEOUT 	30
#define WRITE_TIMEOUT 	30

/**
 * Event loop works with the data stored in an instance of this struct.
 */
typedef struct s_global_state {
	struct event_base *event_loop; /**< For holding and polling events. */
	linkedlist_t neighbours; /**< Linked list of our neighbours. */
} global_state_t;

int listen_init(struct evconnlistener **listener,
		struct s_global_state *global_state);

#endif /* P2P_H */
