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

#ifndef NEIGHBOURS_H
#define NEIGHBOURS_H

#include <event2/bufferevent.h>
#include <netinet/in.h>
#include <stddef.h>

#include "linkedlist.h"

/* minimum number of peers we need to be connected to */
#define MIN_NEIGHBOURS	3

/* request for addresses */
#define NEIGHBOUR_ADDRS_REQ	0x01

/** Data type for the linkedlist of neighbours. */
typedef struct s_neighbour {
	/** Neighbour's IPv6 address.
	 *  Also allows storing IPv4-mapped IPv6 addresses. */
	struct	in6_addr addr;
	/** Bufferevent belonging to this neighbour. */
	struct	bufferevent *buffer_event;
	/** Number of failed ping attempts -- max 3, then disconnect. */
	size_t	failed_pings;
	/** A set of flags for this neighbour. */
	int	flags;
} neighbour_t;

neighbour_t *add_new_neighbour(linkedlist_t		*neighbours,
			       const struct in6_addr	*ip_addr,
			       struct bufferevent  	*bev);

void clear_neighbours(linkedlist_t *neighbours);

void delete_neighbour(linkedlist_t *neighbours, struct bufferevent *bev);

int fetch_specific_neighbours(const linkedlist_t	*neighbours,
			      neighbour_t		***output,
			      int			flags);

neighbour_t *find_neighbour(const linkedlist_t		*neighbours,
			    const struct bufferevent	*bev);

neighbour_t *find_neighbour_by_addr(const linkedlist_t		*neighbours,
				    const struct in6_addr	*ip_addr);

void set_neighbour_flags(neighbour_t *neighbour, int flags);

void unset_neighbour_flags(neighbour_t *neighbour, int flags);
#endif /* NEIGHBOURS_H */
