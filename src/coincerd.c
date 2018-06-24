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

#include <event2/event.h>
#include <event2/listener.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#include "filing.h"
#include "log.h"
#include "neighbours.h"
#include "p2p.h"
#include "peers.h"

static void signal_cb(evutil_socket_t fd, short events, void *ctx);
static void conns_cb(evutil_socket_t fd, short events, void *ctx);

int main(void)
{
	/*
	 * TODO
	 * - initialise network
	 *   - load known peers
	 *   - or seed
	 * - load configuration
	 * - try to connect to bitcoind and co.
	 * - init markets from the network
	 * - publish non-expired order [default: 1 hour]
	 * - check state of trades
	 *
	 * [could be 1 thread → libevent]
	 * - process network messages
	 * - await commands [via UNIX socket]
	 *
	 * - terminate on SIGTERM
	 */

	struct event		*conns_event;
	struct timeval		conns_interval;
	global_state_t		global_state;
	struct evconnlistener	*listener;
	struct event		*sigint_event;
	time_t			t;

	srand((unsigned) time(&t));

	/* initialize all global_state variables */
	global_state.event_loop	= NULL;
	if (setup_paths(&global_state.filepaths)) {
		return 1;
	}

	linkedlist_init(&global_state.pending_neighbours);
	linkedlist_init(&global_state.neighbours);
	linkedlist_init(&global_state.peers);

	if (fetch_peers(global_state.filepaths.peers, &global_state.peers)) {
		return 2;
	}

	/* setup everything needed for TCP listening */
	if (listen_init(&listener, &global_state) != 0) {
		return 3;
	}

	/* register SIGINT event to its callback */
	sigint_event = evsignal_new(global_state.event_loop,
				    SIGINT,
				    signal_cb,
				    &global_state);
	if (!sigint_event || event_add(sigint_event, NULL) < 0) {
		log_error("main - couldn't create or add SIGINT event");
		return 4;
	}

	/* setup a function that actively checks the number of neighbours */
	conns_interval.tv_sec  = 10;
	conns_interval.tv_usec = 0;

	conns_event = event_new(global_state.event_loop,
				-1,
				EV_PERSIST,
				conns_cb,
				&global_state);
	if (!conns_event || event_add(conns_event, &conns_interval) < 0) {
		log_error("main - couldn't create or add conns_event");
		return 4;
	}

	/* initiate joining the coincer network */
	add_more_connections(&global_state, MIN_NEIGHBOURS);

	/* start the event loop */
	event_base_dispatch(global_state.event_loop);

	/* SIGINT received, loop terminated; the clean-up part */
	clear_neighbours(&global_state.pending_neighbours);
	clear_neighbours(&global_state.neighbours);
	/* store peers into 'peers' file before cleaning them */
	store_peers(global_state.filepaths.peers, &global_state.peers);
	clear_peers(&global_state.peers);
	clear_paths(&global_state.filepaths);

	evconnlistener_free(listener);
	event_free(sigint_event);
	event_free(conns_event);
	event_base_free(global_state.event_loop);

	printf("\nCoincer says: Bye!\n");

	return 0;
}

/**
 * Callback function for a received signal.
 *
 * @param	signal	What signal was invoked.
 * @param	events	Flags of the event occured.
 * @param	ctx	Global state.
 */
static void signal_cb(evutil_socket_t	signal __attribute__((unused)),
		      short		events __attribute__((unused)),
		      void		*ctx)
{
	global_state_t *global_state = (global_state_t *) ctx;

	event_base_loopexit(global_state->event_loop, NULL);
}

/**
 * Actively check the number of neighbours and add more if needed.
 *
 * @param	fd	File descriptor.
 * @param	events	Event flags.
 * @param	ctx	Global state.
 */
static void conns_cb(int	fd	__attribute__((unused)),
		     short	events	__attribute__((unused)),
		     void	*ctx)
{
	int needed_conns;

	global_state_t *global_state = (global_state_t *) ctx;
	needed_conns = MIN_NEIGHBOURS -
		       linkedlist_size(&global_state->neighbours);
	if (needed_conns > 0) {
		/* ask twice more peers than we need; it's preferable
		 * to have more neighbours than minimum
		 */
		log_debug("conns_cb - we need %d more neighbours");
		add_more_connections(global_state, 2 * needed_conns);
	}
}

