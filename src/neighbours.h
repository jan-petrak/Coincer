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
#include <stddef.h>

/** data type for the linked list of neighbours */
struct s_neighbour {
	/** neighbours's IPv6 address;
	 *  also allows storing IPv4-mapped IPv6 addresses */
	char ip_addr[46];

	/** bufferevent belonging to this neighbour */
	struct bufferevent *buffer_event;

	/** number of failed ping attempts -- max 3, then disconnect */
	size_t failed_pings;

	/** next neighbour in the linked list (struct s_neighbours) */
	struct s_neighbour *next;
};

/** linked list of neighbours */
struct s_neighbours {
	/** number of neighbours we are connected to */
	size_t size;

	struct s_neighbour *first;
	struct s_neighbour *last;
};

/** set linked list variables to their default values */
void neighbours_init(struct s_neighbours *neighbours);

/** find neighbour in neighbours based on their buffer_event,
    returns NULL if not found */
struct s_neighbour *find_neighbour(const struct s_neighbours *neighbours,
				   const struct bufferevent  *bev);

/** find neighbour in neighbours based on their ip_addr,
    returns NULL if not found */
struct s_neighbour *find_neighbour_by_ip(const struct s_neighbours *neighbours,
					 const char *ip_addr);

/** add new neighbour into neighbours
    returns NULL if neighbour already exists */
struct s_neighbour *add_new_neighbour(struct s_neighbours *neighbours,
				      const char	  *ip_addr,
				      struct bufferevent  *bev);

/* delete neighbour from neighbours */
void delete_neighbour(struct s_neighbours *neighbours,
		      struct bufferevent  *bev);

/* delete all neighbours */
void clear_neighbours(struct s_neighbours *neighbours);

#endif /* NEIGHBOURS_H */
