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

#include "hosts.h"
#include "linkedlist.h"
#include "peers.h"

/** Request for addresses. */
#define NEIGHBOUR_ADDRS_REQ	0x01
/** Practically, this means that we've received p2p.hello from this
 *  neighbour (and so we know their pseudonym). */
#define NEIGHBOUR_ACTIVE	0x02

/** Data type for the linkedlist of neighbours. */
typedef struct s_neighbour {
	/** Neighbour's IPv6 address.
	 *  Also allows storing IPv4-mapped IPv6 addresses. */
	struct in6_addr addr;
	/** Bufferevent belonging to this neighbour. */
	struct bufferevent *buffer_event;
	/** Client info. */
	char *client;
	/** Number of failed ping attempts -- max 3, then disconnect. */
	size_t failed_pings;
	/** A set of flags for this neighbour. */
	int flags;
	/** Corresponding host. */
	host_t *host;
	/** Our peer pseudonym for this neighbour. */
	identity_t my_pseudonym;
	/** Neighbour's node in the neighbours container. */
	linkedlist_node_t *node;
	/** Neighbour's peer pseudonym for us. */
	peer_t pseudonym;
} neighbour_t;

neighbour_t *add_new_neighbour(linkedlist_t		*neighbours,
			       const struct in6_addr	*ip_addr,
			       struct bufferevent  	*bev);

void clear_neighbour(neighbour_t *neighbour);

int compare_neighbour_addrs(const neighbour_t		*neighbour,
			    const struct in6_addr	*addr);

int compare_neighbour_bufferevents(const neighbour_t		*neighbour,
				   const struct bufferevent	*bev);

int compare_neighbour_my_pseudonyms(const neighbour_t *neighbour,
				    unsigned char     *public_key);

int compare_neighbour_pseudonyms(const neighbour_t *neighbour,
				 unsigned char	   *public_key);

int fetch_specific_neighbours(const linkedlist_t	*neighbours,
			      neighbour_t		***output,
			      int			flags);

neighbour_t *find_neighbour(const linkedlist_t	*neighbours,
			    const void		*attribute,
			    int (*cmp_func) (const neighbour_t	*neighbour,
					     const void		*attribute));

void set_neighbour_flags(neighbour_t *neighbour, int flags);

void unset_neighbour_flags(neighbour_t *neighbour, int flags);

#endif /* NEIGHBOURS_H */
