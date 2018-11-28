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

#include <event2/event.h>

#include "configuration.h"
#include "global_state.h"
#include "linkedlist.h"
#include "log.h"
#include "neighbours.h"
#include "paths.h"
#include "peers.h"
#include "routing.h"

/**
 * Clear global state variables.
 *
 * @param	global_state	The global state.
 */
void global_state_clear(global_state_t *global_state)
{
	linkedlist_destroy(&global_state->routing_table, route_clear);
	linkedlist_destroy(&global_state->pending_neighbours, clear_neighbour);
	linkedlist_destroy(&global_state->peers, peer_clear);
	linkedlist_destroy(&global_state->neighbours, clear_neighbour);
	linkedlist_destroy(&global_state->message_traces, NULL);
	linkedlist_destroy(&global_state->hosts, NULL);
	/* event nodes' data are not malloc'd; just apply the removal */
	linkedlist_apply(&global_state->events, event_free, linkedlist_remove);
	linkedlist_destroy(&global_state->identities, NULL);

	clear_paths(&global_state->filepaths);
	event_base_free(global_state->event_loop);
}

/**
 * Initialize global state variables.
 *
 * @param	global_state	The global state.
 *
 * @return	0		Successfully initialized.
 * @return	1		Failure.
 */
int global_state_init(global_state_t *global_state)
{
	global_state->event_loop = NULL;
	if (setup_paths(&global_state->filepaths)) {
		log_error("Initializing paths to needed files/dirs");
		return 1;
	}
	if (create_dirs(&global_state->filepaths)) {
		log_error("Creating directories");
		return 1;
	}

	linkedlist_init(&global_state->identities);
	if (!(global_state->true_identity = identity_generate(
						&global_state->identities,
						0x00))) {
		log_error("Creating our true identity");
		return 1;
	}
	linkedlist_init(&global_state->events);
	linkedlist_init(&global_state->hosts);
	linkedlist_init(&global_state->message_traces);
	linkedlist_init(&global_state->neighbours);
	linkedlist_init(&global_state->peers);
	linkedlist_init(&global_state->pending_neighbours);
	linkedlist_init(&global_state->routing_table);

	global_state->port = 0;

	return 0;
}
