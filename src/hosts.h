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

#ifndef HOSTS_H
#define HOSTS_H

#include <arpa/inet.h>
#include <stdint.h>

#include "linkedlist.h"

/** The number of hosts guaranteed to be in the network. */
#define	DEFAULT_HOSTS_SIZE	2
/** Maximum number of hosts we store. */
#define	MAX_HOSTS_SIZE		50

/** A host is not available if they is already our neighbour,
 *  pending to become one, or if we are unnable to connect to them. */
#define HOST_AVAILABLE	0x01

/** IPv6 addresses of hosts guaranteed to be in the network. */
static const unsigned char DEFAULT_HOSTS[DEFAULT_HOSTS_SIZE][16] = {
/* TODO: Replace with real default hosts */
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xff, 0xff,
		192, 168, 0, 124
	},
	{
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0xff, 0xff,
		192, 168, 0, 125
	}
};

/** Host info holder. */
typedef struct s_host {
	/** Binary IPv6 address. */
	struct in6_addr	addr;
	/** A set of flags for this host. */
	int flags;
	/* Host's listening port. */
	unsigned short port;
	/* TODO: add uptime */
} host_t;

int fetch_hosts(const char *hosts_path, linkedlist_t *hosts);

int fetch_specific_hosts(const linkedlist_t	*hosts,
			 host_t			***output,
			 int			flags);

host_t *find_host(const linkedlist_t	*hosts,
		  const struct in6_addr	*addr);

int hosts_to_str(const linkedlist_t *hosts, char **output);

void reset_hosts_availability(linkedlist_t *hosts);

host_t *save_host(linkedlist_t		*hosts,
		  const struct in6_addr	*addr,
		  unsigned short	port,
		  int			flags);

void set_host_flags(host_t *host, int flags);

void shuffle_hosts_arr(host_t **hosts, size_t hosts_size);

int store_hosts(const char *hosts_path, const linkedlist_t *hosts);

void unset_host_flags(host_t *host, int flags);

#endif /* HOSTS_H */
