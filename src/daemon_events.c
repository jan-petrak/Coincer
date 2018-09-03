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

static void signal_cb(evutil_socket_t	signal __attribute__((unused)),
		      short		events __attribute__((unused)),
		      void		*ctx);

/**
 * Actively check the number of neighbours and ask for more if needed.
 *
 * @param	fd	File descriptor.
 * @param	events	Event flags.
 * @param	ctx	Global state.
 */
static void conns_cb(int	fd	__attribute__((unused)),
		     short	events	__attribute__((unused)),
		     void	*ctx)
{
	global_state_t *global_state;
	int		needed_conns;

	global_state = (global_state_t *) ctx;

	needed_conns = MIN_NEIGHBOURS -
		       linkedlist_size(&global_state->neighbours);
	if (needed_conns > 0) {
		log_debug("conns_cb - we need %d more neighbours");
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

	/* check the number of neighbours every 10 seconds */
	interval.tv_sec  = 10;
	interval.tv_usec = 0;

	event = event_new(global_state->event_loop,
			  -1,
			  EV_PERSIST,
			  conns_cb,
			  global_state);
	if (!event ||
	    event_add(event, &interval) < 0 ||
	    !linkedlist_append(events, event)) {
		log_error("Creating or adding Connections event");
		return 1;
	}

	return 0;
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
