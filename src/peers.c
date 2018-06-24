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

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linkedlist.h"
#include "log.h"
#include "peers.h"

/**
 * Add new peer into sorted linkedlist of peers.
 *
 * @param	peers	The linkedlist of peers.
 * @param	addr	Address of the new peer.
 *
 * @return	linkedlist_node_t	Newly added node in the linkedlist.
 * @return	NULL			Peer is already added, default or
 *					allocation failure.
 */
linkedlist_node_t *add_peer(linkedlist_t *peers, const struct in6_addr *addr)
{
	int			cmp_value;
	struct in6_addr		curr_addr;
	linkedlist_node_t	*current_node;
	peer_t			*current_peer;
	int			i;
	linkedlist_node_t	*new_node;
	peer_t			*new_peer;
	char			text_ip[INET6_ADDRSTRLEN];

	/* add peer to the list of known peers, unless it's a default peer */
	for (i = 0; i < DEFAULT_PEERS_SIZE; i++) {
		memcpy(&curr_addr, DEFAULT_PEERS[i], 16);
		/* it's a default peer, don't add it into 'peers' */
		if (memcmp(&curr_addr, addr, 16) == 0) {
			return NULL;
		}
	}

	/* allocate memory for a new peer */
	new_peer = (peer_t *) malloc(sizeof(peer_t));
	if (new_peer == NULL) {
		log_error("add_peer - malloc");
		return NULL;
	}

	/* initialize all attributes of the new peer */
	memcpy(&new_peer->addr, addr, 16);
	new_peer->is_available = 1;

	/* get textual representation of 'addr' */
	inet_ntop(AF_INET6, addr, text_ip, INET6_ADDRSTRLEN);

	/* insert 'new_peer' to its proper position in the sorted linkedlist;
	 * start from the last node of 'peers', as 'fetch_peers' (using this
	 * function) is likely to add in ascending order => better performance
	 */
	current_node = linkedlist_get_last(peers);
	while (current_node != NULL) {
		current_peer = (peer_t *) current_node->data;

		cmp_value = memcmp(&new_peer->addr, &current_peer->addr, 16);
		/* the linkedlist already contains this peer */
		if (cmp_value == 0) {
			free(new_peer);
			return NULL;
		} else if (cmp_value > 0) {
			/* the proper position found */
			new_node = linkedlist_insert_after(peers,
							   current_node,
							   new_peer);
			if (new_node != NULL) {
				log_debug("add_peer - %s successfully added",
					  text_ip);
			}
			return new_node;
		}
		current_node = linkedlist_get_prev(peers, current_node);
	}
	/* the new peer's addr is lexicographically the lowest */
	new_node = linkedlist_insert_after(peers, &peers->first, new_peer);
	if (new_node != NULL) {
		log_debug("add_peer - %s successfully added", text_ip);
	}
	return new_node;
}

/**
 * Delete all peers and their data.
 *
 * @param	peers	Linkedlist of peers.
 */
void clear_peers(linkedlist_t *peers)
{
	/* peer_t has no dynamically allocated variables */
	linkedlist_destroy(peers);
}

/**
 * Fetch available peers from 'peers' into 'available_peers'.
 *
 * @param	peers		All peers known to us.
 * @param	available_peers	The peers marked as available.
 *
 * @return	n		The number of available peers.
 */
size_t fetch_available_peers(const linkedlist_t *peers,
			     peer_t *available_peers[MAX_PEERS_SIZE])
{
	linkedlist_node_t	*current_node;
	peer_t			*current_peer;
	size_t			n = 0;

	current_node = linkedlist_get_first(peers);
	while (current_node != NULL) {
		current_peer = (peer_t *) current_node->data;
		if (current_peer->is_available) {
			/* we can use this function with 'available_peers'
			 * set to NULL just to get the number of available
			 * peers as return value
			 */
			if (available_peers != NULL) {
				available_peers[n++] = current_peer;
			} else {
				n++;
			}
		}
		current_node = linkedlist_get_next(peers, current_node);
	}

	return n;
}

/**
 * Fetch peers from 'peers_path' file into 'peers' linkedlist.
 *
 * @param	peers_path	Path to 'peers' file.
 * @param	peers		Fetch loaded peers in here.
 *
 * @return	0		Successfully fetched.
 * @return	1		Failure.
 */
int fetch_peers(const char *peers_path, linkedlist_t *peers)
{
	struct in6_addr	addr;
	FILE		*peers_file;

	peers_file = fopen(peers_path, "rb");
	if (peers_file == NULL) {
		log_error("fetch_peers - peers_file not found");
		return 1;
	}

	while (fread(&addr, sizeof(struct in6_addr), 1, peers_file) == 1) {
		add_peer(peers, &addr);
	}

	fclose(peers_file);
	return 0;
}

/**
 * Find peer in the linkedlist by their address.
 *
 * @param	peers	Linkedlist of peers.
 * @param	addr	Address of the peer we want.
 *
 * @return	peer_t	Requested peer.
 * @return	NULL	Peer not found.
 */
peer_t *find_peer_by_addr(const linkedlist_t	*peers,
			  const struct in6_addr *addr)
{
	const linkedlist_node_t *current = linkedlist_get_first(peers);

	while (current != NULL) {
		peer_t *current_data = (peer_t *) current->data;

		/* ip addresses match => requested peer found */
		if (memcmp(&current_data->addr, addr, 16) == 0) {
			/* return node's data; struct s_peer */
			return current_data;
		}
		current = linkedlist_get_next(peers, current);
	}
	/* requested peer not found */
	return NULL;
}

/* TODO: allocate memory for the 'output', don't assume any buffer size */
/**
 * '\n' separated 'output' of peer addresses in readable form from 'peers'.
 *
 * @param	peers	List of peers.
 * @param	output	Output string.
 */
void peers_to_str(const linkedlist_t *peers, char *output)
{
	linkedlist_node_t	*current_node;
	peer_t			*current_peer;
	size_t			output_size = 0;
	char			text_ip[INET6_ADDRSTRLEN];

	output[0] = '\0';
	current_node = linkedlist_get_first(peers);
	while (current_node != NULL) {
		current_peer = (peer_t *) current_node->data;

		/* binary ip to text ip conversion */
		inet_ntop(AF_INET6,
			  &current_peer->addr,
			  text_ip,
			  INET6_ADDRSTRLEN);
		output_size += strlen(text_ip);
		strcat(output, text_ip);

		current_node = linkedlist_get_next(peers, current_node);
		/* if it's not the last peer, append '\n' */
		if (current_node != NULL) {
			output[output_size++] = '\n';
		}
		output[output_size] = '\0';
	}
}

/**
 * Set all peers available.
 *
 * @param	peers	The peers to be set as available.
 */
void reset_peers_availability(linkedlist_t *peers)
{
	linkedlist_node_t	*current_node;
	peer_t			*current_peer;

	current_node = linkedlist_get_first(peers);
	while (current_node != NULL) {
		current_peer = (peer_t *) current_node->data;

		current_peer->is_available = 1;
		current_node = linkedlist_get_next(peers, current_node);
	}
}

/**
 * Shuffle the input array 'peers'.
 *
 * @param	peers		The peers to be shuffled.
 * @param	peers_size	Number of peers to be shuffled.
 */
void shuffle_peers_arr(peer_t *peers[MAX_PEERS_SIZE], size_t peers_size)
{
	size_t	i;
	size_t	idx;
	peer_t	*tmp;

	for (i = 0; i < peers_size; i++) {
		/* don't swap with already swapped */
		idx = i + rand() % (peers_size - i);

		tmp = peers[i];
		peers[i] = peers[idx];
		peers[idx] = tmp;
	}
}

/**
 * Store peers from the linkedlist 'peers' into file at 'peers_path'.
 *
 * @param	peers_path		Path to 'peers' file.
 * @param	peers			Linkedlist of the peers to be stored.
 *
 * @return	0			Successfully stored.
 * @return	1			Failure.
 */
int store_peers(const char *peers_path, const linkedlist_t *peers)
{
	const linkedlist_node_t *current;
	const peer_t		*current_peer;
	FILE			*peers_file;

	peers_file = fopen(peers_path, "wb");
	if (peers_file == NULL) {
		log_error("store_peers - unable to create %s", peers_path);
		return 1;
	}

	current = linkedlist_get_first(peers);
	while (current != NULL) {
		current_peer = (peer_t *) current->data;
		fwrite(&current_peer->addr,
		       sizeof(struct in6_addr),
		       1,
		       peers_file);
		current = linkedlist_get_next(peers, current);
	}

	fclose(peers_file);
	return 0;
}

