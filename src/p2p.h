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

#include "neighbours.h"

#define	DEFAULT_PORT	31070
/* after (READ/WRITE)_TIMEOUT seconds invoke timeout callback */
#define READ_TIMEOUT 	30
#define WRITE_TIMEOUT 	30

/* event loop will work with the data stored in an instance of this struct */
struct Loop_Data {
	struct event_base *event_loop;
	struct Neighbours neighbours;
};

/**
 * @brief Initialize listening and set up callbacks
 *
 * @param	listener	The even loop listener
 * @param	loop_data	Data for the event loop to work with
 *
 * @return	1 if an error occured
 * @return	0 if successfully initialized
 */
int listen_init(struct evconnlistener 	**listener,
		struct Loop_Data 	*loop_data);

#endif /* P2P_H */
