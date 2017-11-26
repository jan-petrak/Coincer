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

#include "neighbours.h"
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

	struct evconnlistener *listener;
	struct Loop_Data loop_data;

	/* To store the return values of setup functions */
	int ret;

	loop_data.event_loop = NULL;
	neighbours_init(&loop_data.neighbours);

	if ((ret = listen_init(&listener, &loop_data)) != 0) {
		return ret;
	}

	event_base_dispatch(loop_data.event_loop);

	event_base_free(loop_data.event_loop);
	evconnlistener_free(listener);
	clear_neighbours(&loop_data.neighbours);

	return 0;
}
