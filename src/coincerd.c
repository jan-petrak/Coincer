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
#include <event2/listener.h>
#include <sodium.h>

#include "daemon_events.h"
#include "global_state.h"
#include "hosts.h"
#include "log.h"
#include "p2p.h"

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

	global_state_t		global_state;
	struct evconnlistener	*listener;

	if (sodium_init() < 0) {
		log_error("Libsodium failed to initialize");
		return 4;
	}

	/* TODO: use randombytes (from libsodium?) for the seed of randomness */
	srand((unsigned) time(NULL));

	if (global_state_init(&global_state)) {
		log_error("Basic daemon setup");
		return 1;
	}

	fetch_hosts(global_state.filepaths.hosts, &global_state.hosts);

	/* setup everything needed for TCP listening */
	if (listen_init(&listener, &global_state) != 0) {
		log_error("Initialization of TCP listening");
		return 2;
	}

	/* setup needed daemon events */
	if (daemon_events_setup(&global_state)) {
		log_error("Setting up events");
		return 3;
	}

	/* initiate joining the coincer network */
	add_more_connections(&global_state, MIN_NEIGHBOURS);

	/* start the event loop */
	event_base_dispatch(global_state.event_loop);

	/* SIGINT or SIGTERM received, the loop is terminated */

	/* store hosts into 'hosts' file before clearing global_state */
	store_hosts(global_state.filepaths.hosts, &global_state.hosts);

	evconnlistener_free(listener);
	global_state_clear(&global_state);

	return 0;
}
