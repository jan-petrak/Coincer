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

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crypto.h"
#include "hosts.h"
#include "linkedlist.h"
#include "log.h"

/**
 * Fetch hosts from file into linkedlist.
 *
 * @param	hosts_path	Path to hosts file.
 * @param	hosts		Fetch loaded hosts in here.
 *
 * @return	0		Successfully fetched.
 * @return	1		Hosts file could not be opened.
 */
int fetch_hosts(const char *hosts_path, linkedlist_t *hosts)
{
	host_t		current_host;
	FILE		*hosts_file;

	hosts_file = fopen(hosts_path, "rb");
	if (!hosts_file) {
		log_warn("Hosts file not found at %s. "
			 "It is safe to ignore this warning", hosts_path);
		return 1;
	}

	while (fread(&current_host, sizeof(host_t), 1, hosts_file) == 1) {
		save_host(hosts,
			  &current_host.addr,
			  current_host.port,
			  current_host.flags);
	}

	fclose(hosts_file);
	return 0;
}

/**
 * Fetch pointers to hosts with specific flags set, into array that is being
 * allocated in here.
 *
 * @param	hosts		All hosts known to us.
 * @param	output		Output array of pointers to satisfying hosts.
 *				If set to NULL, function just returns
 *				the number of them.
 * @param	flags		Choose hosts based on these flags.
 *				Fetches output with all known hosts if set to 0.
 *
 * @return	>=0		The number of satisfying hosts.
 * @return	-1		Allocation failure.
 */
int fetch_specific_hosts(const linkedlist_t	*hosts,
			 host_t			***output,
			 int			flags)
{
	linkedlist_node_t	*current_node;
	host_t			*current_host;
	size_t			n = 0;

	if (output) {
		*output = (host_t **) malloc(linkedlist_size(hosts) *
					     sizeof(host_t *));
		if (!*output) {
			log_error("Fetching specific hosts");
			return -1;
		}
	}

	current_node = linkedlist_get_first(hosts);
	while (current_node) {
		current_host = (host_t *) current_node->data;
		/* if all specified flags are being set on this host */
		if ((current_host->flags & flags) == flags) {
			if (output) {
				(*output)[n++] = current_host;
			} else {
				n++;
			}
		}
		current_node = linkedlist_get_next(hosts, current_node);
	}

	return n;
}

/**
 * Find host in the linkedlist by its address.
 *
 * @param	hosts	Linkedlist of hosts.
 * @param	addr	Address of the host we want.
 *
 * @return	host_t	Requested host.
 * @return	NULL	Host not found.
 */
host_t *find_host(const linkedlist_t	*hosts,
		  const struct in6_addr *addr)
{
	const linkedlist_node_t *current = linkedlist_get_first(hosts);

	while (current) {
		host_t *current_data = (host_t *) current->data;

		/* ip addresses match => requested host found */
		if (memcmp(&current_data->addr, addr, 16) == 0) {
			/* return node's data; struct s_host */
			return current_data;
		}
		current = linkedlist_get_next(hosts, current);
	}
	/* requested host not found */
	return NULL;
}

/**
 * '\n' separated output string of host addresses in readable form.
 *
 * @param	hosts	List of hosts.
 * @param	output	Output string.
 *
 * @return	0	Success.
 * @return	1	Failure.
 */
int hosts_to_str(const linkedlist_t *hosts, char **output)
{
	linkedlist_node_t	*current_node;
	host_t			*current_host;
	size_t			output_size = 0;
	char			text_ip[INET6_ADDRSTRLEN];

	/* TODO: Don't assume any buffer size. This will be fixed in the
	 * 	 next commit. */
	*output = (char *) malloc(4096 * sizeof(char));
	if (!*output) {
		log_error("Hosts to string");
		return 1;
	}

	*output[0] = '\0';
	current_node = linkedlist_get_first(hosts);
	while (current_node != NULL) {
		current_host = (host_t *) current_node->data;

		/* binary ip to text ip conversion */
		inet_ntop(AF_INET6,
			  &current_host->addr,
			  text_ip,
			  INET6_ADDRSTRLEN);
		output_size += strlen(text_ip);
		strcat(*output, text_ip);

		current_node = linkedlist_get_next(hosts, current_node);
		/* if it's not the last host, append '\n' */
		if (current_node != NULL) {
			*output[output_size++] = '\n';
		}
		*output[output_size] = '\0';
	}

	return 0;
}

/**
 * Save new host into sorted linkedlist of hosts.
 *
 * @param	hosts		The linkedlist of hosts.
 * @param	addr		Address of the new host.
 * @param	port		Host's listening port.
 * @param	flags		Host's flags.
 *
 * @return	host_t		Newly saved host.
 * @return	NULL		Host is already saved, default or
 *				allocation failure.
 */
host_t *save_host(linkedlist_t		*hosts,
		  const struct in6_addr *addr,
		  unsigned short	port,
		  int			flags)
{
	int			cmp_value;
	struct in6_addr		curr_addr;
	linkedlist_node_t	*current_node;
	host_t			*current_host;
	int			i;
	linkedlist_node_t	*new_node;
	host_t			*new_host;
	char			text_ip[INET6_ADDRSTRLEN];

	/* save host to the list of known hosts, unless it's a default host */
	for (i = 0; i < DEFAULT_HOSTS_SIZE; i++) {
		memcpy(&curr_addr, DEFAULT_HOSTS[i], 16);
		/* it's a default host, don't save it into 'hosts' */
		if (memcmp(&curr_addr, addr, 16) == 0) {
			return NULL;
		}
	}

	/* allocate memory for a new host */
	new_host = (host_t *) malloc(sizeof(host_t));
	if (!new_host) {
		log_error("Saving new host");
		return NULL;
	}

	/* initialize all attributes of the new host */
	memcpy(&new_host->addr, addr, 16);
	new_host->port = port;
	new_host->flags = 0x0;
	set_host_flags(new_host, flags);

	/* get textual representation of 'addr' */
	inet_ntop(AF_INET6, addr, text_ip, INET6_ADDRSTRLEN);

	/* insert 'new_host' to its proper position in the sorted linkedlist;
	 * start from the last node of 'hosts', as 'fetch_hosts' (using this
	 * function) is likely to save in ascending order => better performance
	 */
	current_node = linkedlist_get_last(hosts);
	while (current_node) {
		current_host = (host_t *) current_node->data;

		cmp_value = memcmp(&new_host->addr, &current_host->addr, 16);
		/* the linkedlist already contains this host */
		if (cmp_value == 0) {
			free(new_host);
			return NULL;
		} else if (cmp_value > 0) {
			/* the proper position found */
			new_node = linkedlist_insert_after(hosts,
							   current_node,
							   new_host);
			if (new_node) {
				log_debug("save_host - %s successfully saved",
					  text_ip);
			}
			return new_host;
		}
		current_node = linkedlist_get_prev(hosts, current_node);
	}
	/* the new host's addr is lexicographically the lowest */
	new_node = linkedlist_insert_after(hosts, &hosts->first, new_host);
	if (new_node) {
		log_debug("save_host - %s successfully saved", text_ip);
	}

	return new_host;
}

/**
 * Set flags on given host.
 *
 * @param	host	Set flags on this host.
 * @param	flags	Set these flags on the host.
 */
void set_host_flags(host_t *host, int flags)
{
	host->flags |= flags;
}

/**
 * Shuffle an input array of hosts.
 *
 * @param	hosts		The array of hosts to be shuffled.
 * @param	hosts_size	The number of hosts to be shuffled.
 */
void shuffle_hosts_arr(host_t **hosts, size_t hosts_size)
{
	size_t	i, j;
	host_t	*tmp;

	for (i = 0; i < hosts_size; i++) {
		/* don't swap with already swapped;
		 * swapping hosts[i] with hosts[j], where j is a random index
		 * such that j >= i AND j < hosts_size
		 */
		j = i + get_random_uint32_t(hosts_size - i);

		tmp = hosts[i];
		hosts[i] = hosts[j];
		hosts[j] = tmp;
	}
}

/**
 * Store hosts from a linkedlist into a file.
 *
 * @param	hosts_path		Path to 'hosts' file.
 * @param	hosts			Linkedlist of the hosts to be stored.
 *
 * @return	0			Successfully stored.
 * @return	1			Failure.
 */
int store_hosts(const char *hosts_path, const linkedlist_t *hosts)
{
	const linkedlist_node_t *current;
	const host_t		*current_host;
	FILE			*hosts_file;

	hosts_file = fopen(hosts_path, "wb");
	if (!hosts_file) {
		log_error("Can not create hosts file at %s", hosts_path);
		return 1;
	}

	current = linkedlist_get_first(hosts);
	while (current) {
		current_host = (host_t *) current->data;
		/* if fwrite fails, terminate storing */
		if (fwrite(current_host,
			   sizeof(host_t),
			   1,
			   hosts_file) != 1) {
			log_error("Storing hosts");
			fclose(hosts_file);
			return 1;
		}
		current = linkedlist_get_next(hosts, current);
	}

	fclose(hosts_file);
	return 0;
}

/**
 * Unset flags on given host.
 *
 * @param	host	Unset flags on this host.
 * @param	flags	Unset these flags on the host.
 */
void unset_host_flags(host_t *host, int flags)
{
	host->flags &= ~flags;
}
