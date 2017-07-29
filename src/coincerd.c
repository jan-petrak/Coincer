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

	int r;
	if ((r = listen_init()) != 0)
		return r;
    
	return 0;
}
