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
#include <signal.h>

#include "daemon_events.h"
#include "global_state.h"
#include "linkedlist.h"
#include "log.h"
#include "neighbours.h"
#include "p2p.h"
#include "peers.h"

/** The time interval in seconds between loop_update_long calls. */
#define UPDATE_TIME_LONG	60
/** The time interval in seconds between loop_update_short calls. */
#define UPDATE_TIME_SHORT	10

static void loop_update_long(evutil_socket_t	fd     __attribute__((unused)),
			     short		events __attribute__((unused)),
			     void		*ctx);
static void loop_update_short(evutil_socket_t	fd     __attribute__((unused)),
			      short		events __attribute__((unused)),
			      void		*ctx);

static void remove_stale_records(global_state_t *global_state);

static void signal_cb(evutil_socket_t	signal __attribute__((unused)),
		      short		events __attribute__((unused)),
		      void		*ctx);

/**
 * Check the number of neighbours and ask for more if needed.
 *
 * @param	global_state	The global state.
 */
static void connections_maintain(global_state_t *global_state)
{
	int needed_conns;

	needed_conns = MIN_NEIGHBOURS -
		       linkedlist_size(&global_state->neighbours);
	if (needed_conns > 0) {
		log_debug("conns_cb - need %d more neighbours", needed_conns);
		/* ask twice more hosts than we need; it's preferable
		 * to have more neighbours than minimum */
		add_more_connections(global_state, 2 * needed_conns);
	}
}

/**
 * Setup and add daemon events into global state's event loop.
 *
 * @param	global_state	Global state.
 *
 * @return	0		Successfully set up.
 * @return	1		Creating or adding an event failed.
 */
int daemon_events_setup(global_state_t *global_state)
{
	struct event	*event;
	linkedlist_t	*events;
	struct timeval	interval;

	events = &global_state->events;

	/* register SIGINT event to its callback */
	event = evsignal_new(global_state->event_loop,
			     SIGINT,
			     signal_cb,
			     global_state);
	if (!event ||
	    event_add(event, NULL) < 0 ||
	    !linkedlist_append(events, event)) {
		log_error("Creating or adding SIGINT event");
		return 1;
	}

	/* register SIGTERM event too */
	event = evsignal_new(global_state->event_loop,
			     SIGTERM,
			     signal_cb,
			     global_state);
	if (!event ||
	    event_add(event, NULL) < 0 ||
	    !linkedlist_append(events, event)) {
		log_error("Creating or adding SIGTERM event");
		return 1;
	}

	/* register the short time-interval loop updates */
	interval.tv_sec  = UPDATE_TIME_SHORT;
	interval.tv_usec = 0;

	event = event_new(global_state->event_loop,
			  -1,
			  EV_PERSIST,
			  loop_update_short,
			  global_state);
	if (!event ||
	    event_add(event, &interval) < 0 ||
	    !linkedlist_append(events, event)) {
		log_error("Creating or adding short loop update callback");
		return 1;
	}

	/* register the long time-interval loop updates */
	interval.tv_sec  = UPDATE_TIME_LONG;
	interval.tv_usec = 0;

	event = event_new(global_state->event_loop,
			  -1,
			  EV_PERSIST,
			  loop_update_long,
			  global_state);

	if (!event ||
	    event_add(event, &interval) < 0 ||
	    !linkedlist_append(events, event)) {
		log_error("Creating or adding long loop update callback");
		return 1;
	}

	return 0;
}

/**
 * The long time-triggered loop update.
 *
 * @param	fd	File descriptor.
 * @param	events	Event flags.
 * @param	ctx	Global state.
 */
static void loop_update_long(evutil_socket_t	fd     __attribute__((unused)),
			     short		events __attribute__((unused)),
			     void		*ctx)
{
	global_state_t *global_state = (global_state_t *) ctx;

	remove_stale_records(global_state);
}

/**
 * The short time-triggered loop update.
 *
 * @param	fd	File descriptor.
 * @param	events	Event flags.
 * @param	ctx	Global state.
 */
static void loop_update_short(evutil_socket_t fd     __attribute__((unused)),
			      short	      events __attribute__((unused)),
			      void	      *ctx)
{
	global_state_t *global_state = (global_state_t *) ctx;

	connections_maintain(global_state);
}

/**
 * Remove old records from every global state's container.
 *
 * @param	global_state	The global state.
 */
static void remove_stale_records(global_state_t *global_state)
{
	identity_t		*current_identity;
	linkedlist_t		*current_list;
	neighbour_t		*current_neighbour;
	linkedlist_node_t	*current_node;
	peer_t			*current_peer;
	linkedlist_node_t	*next_node;

	current_time = time(NULL);

	/* remove stale nonces of known peers */
	current_list = &global_state->peers;
	current_node = linkedlist_get_first(current_list);
	while (current_node) {
		current_peer = (peer_t *) current_node->data;

		nonces_remove_stale(&current_peer->nonces);

		current_node = linkedlist_get_next(current_list, current_node);
	}

	/* remove stale nonces of the pseudonyms of our neighbours */
	current_list = &global_state->neighbours;
	current_node = linkedlist_get_first(current_list);
	while (current_node) {
		current_neighbour = (neighbour_t *) current_node->data;

		nonces_remove_stale(&current_neighbour->pseudonym.nonces);

		current_node = linkedlist_get_next(current_list, current_node);
	}

	/* remove unneeded identities */
	current_list = &global_state->identities;
	current_node = linkedlist_get_first(current_list);
	while (current_node) {
		current_identity = (identity_t *) current_node->data;
		next_node = linkedlist_get_next(current_list, current_node);

		if (current_identity->flags & IDENTITY_TMP) {
			linkedlist_delete(current_node);
		}

		current_node = next_node;
	}
}

/**
 * Callback function for a received signal.
 *
 * @param	signal	What signal was invoked.
 * @param	events	Flags of the event occurred.
 * @param	ctx	Global state.
 */
static void signal_cb(evutil_socket_t	signal __attribute__((unused)),
		      short		events __attribute__((unused)),
		      void		*ctx)
{
	global_state_t *global_state = (global_state_t *) ctx;

	event_base_loopexit(global_state->event_loop, NULL);
}
