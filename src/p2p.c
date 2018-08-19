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

#define _POSIX_SOURCE		/* strtok_r */

#include <arpa/inet.h>		/* inet_ntop */
#include <assert.h>
#include <errno.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <netinet/in.h>		/* sockaddr_in6 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hosts.h"
#include "linkedlist.h"
#include "log.h"
#include "neighbours.h"
#include "p2p.h"

/**
 * Simple helper for conversion of binary IP to readable IP address.
 *
 * @param	binary_ip	Binary represented IP address.
 * @param	ip		Readable IP address.
 */
static void ip_to_string(const struct in6_addr *binary_ip, char *ip)
{
	inet_ntop(AF_INET6, binary_ip, ip, INET6_ADDRSTRLEN);
}

/**
 * Ask a neighbour for a list of addresses.
 *
 * @param	global_state	Data for the event loop to work with.
 * @param	neighbour	The neighbour to be asked.
 */
static void ask_for_addresses(neighbour_t *neighbour)
{
	struct bufferevent *bev;

	if (neighbour == NULL || (bev = neighbour->buffer_event) == NULL) {
		return;
	}

	/* send message "hosts" to the neighbour, as a request
	 * for the list of hosts; 6 is the number of bytes to be transmitted */
	evbuffer_add(bufferevent_get_output(bev), "hosts", 6);
	/* accept addresses only from those neighbours that we've asked */
	set_neighbour_flags(neighbour, NEIGHBOUR_ADDRS_REQ);
}

/**
 * Process received list of host addresses.
 *
 * @param	global_state	Data for the event loop to work with.
 * @param	hosts		'\n'-separated list of host addresses.
 */
static void process_hosts(global_state_t *global_state, char *hosts)
{
	struct in6_addr	addr;
	const char	delim[2] = "\n";
	char		*line;
	char		*save_ptr;

	line = strtok_r(hosts, delim, &save_ptr);

	while (line != NULL) {
		if (inet_pton(AF_INET6, line, &addr) == 1) {
			save_host(&global_state->hosts, &addr);
		}
		line = strtok_r(NULL, delim, &save_ptr);
	}
}

/**
 * Processing a P2P message.
 *
 * @param	bev	Buffer to read data from.
 * @param	ctx	Pointer to global_state_t to process message sender.
 */
static void p2p_process(struct bufferevent *bev, void *ctx)
{
	global_state_t	*global_state;
	struct evbuffer	*input;
	size_t		len;
	char		*message;
	neighbour_t	*neighbour;
	struct evbuffer	*output;
	char		response[2048]; /* TODO: adjust size */
	char		text_ip[INET6_ADDRSTRLEN];

	global_state = (global_state_t *) ctx;

	/* find the neighbour based on their bufferevent */
	neighbour = find_neighbour(&global_state->neighbours, bev);

	assert(neighbour != NULL);

	/* reset neighbour's failed pings */
	neighbour->failed_pings = 0;

	/* read from the input buffer, write to output buffer */
	input	= bufferevent_get_input(bev);
	output	= bufferevent_get_output(bev);

	/* get length of the input message */
	len = evbuffer_get_length(input);

	/* allocate memory for the input message including '\0' */
	message = (char *) malloc((len + 1) * sizeof(char));
	if (message == NULL) {
		log_error("Received message allocation");
		return;
	}

	/* drain input buffer into data; -1 if evbuffer_remove failed */
	if (evbuffer_remove(input, message, len) == -1) {
		free(message);
		return;
	} else {
		message[len] = '\0';
	}

	ip_to_string(&neighbour->addr, text_ip);
	log_debug("p2p_process - received: %s from %s", message, text_ip);

	response[0] = '\0';

	/* TODO: Replace with JSON messages */
	if (strcmp(message, "ping") == 0) {
		strcpy(response, "pong");
	/* ignore "pong" */
	} else if (strcmp(message, "pong") == 0) {
	/* "hosts" is a request for list of addresses */
	} else if (strcmp(message, "hosts") == 0) {
		hosts_to_str(&global_state->hosts, response);
	/* list of addresses */
	} else {
		if (neighbour->flags & NEIGHBOUR_ADDRS_REQ) {
			process_hosts(global_state, message);
			unset_neighbour_flags(neighbour, NEIGHBOUR_ADDRS_REQ);
		}
	}

	if (response[0] != '\0') {
		log_debug("p2p_process - responding with: %s", response);
		/* copy response to the output buffer */
		evbuffer_add_printf(output, "%s", response);
	}

	/* deallocate input message */
	free(message);
}

/**
 * Process a timeout invoked by event_cb.
 *
 * @param	neighbours	Linked list of neighbours.
 * @param	neighbour	Timeout invoked on this neighbour.
 */
static void timeout_process(linkedlist_t	*neighbours,
			    neighbour_t		*neighbour)
{
	char text_ip[INET6_ADDRSTRLEN];

	/* initialize text_ip */
	ip_to_string(&neighbour->addr, text_ip);

	/* the neighbour hasn't failed enough pings to be deleted */
	if (neighbour->failed_pings < 3) {
		/* bufferevent was disabled when timeout flag was set */
		bufferevent_enable(neighbour->buffer_event,
			EV_READ | EV_WRITE | EV_TIMEOUT);

		log_debug("timeout_process - sending ping to %s. "
			  "Failed pings: %lu", text_ip,
					       neighbour->failed_pings);
		/* send ping to the inactive neighbour;
		 * 5 is the length of bytes to be transmitted */
		evbuffer_add(bufferevent_get_output(neighbour->buffer_event),
			"ping", 5);

		neighbour->failed_pings++;
	} else {
		log_info("%s timed out", text_ip);
		delete_neighbour(neighbours, neighbour->buffer_event);
	}
}

/**
 * Delete a neighbour from pending neighbours and add it into neighbours.
 *
 * @param	global_state	Global state.
 * @param	neighbour	Neighbour to be moved.
 *
 * @param	neighbour_t	The new neighbour added into neighbours.
 * @param	NULL		If adding failed.
 */
static neighbour_t *move_neighbour_from_pending(global_state_t	*global_state,
						neighbour_t	*neighbour)
{
	linkedlist_node_t	*neighbour_node;
	neighbour_t		*new_neighbour;

	new_neighbour = add_new_neighbour(&global_state->neighbours,
					  &neighbour->addr,
					  neighbour->buffer_event);
	/* if the add was unsuccessful, perform just the full delete */
	if (new_neighbour == NULL) {
		bufferevent_free(neighbour->buffer_event);
	}
	neighbour_node = linkedlist_find(&global_state->pending_neighbours,
					 neighbour);
	linkedlist_delete(neighbour_node);

	return new_neighbour;
}

/**
 * Process the event that occurred on our pending neighbour.
 *
 * @param	global_state	Global state.
 * @param	neighbour	The event occurred on this pending neighbour.
 * @param	events		What event occurred.
 */
static void process_pending_neighbour(global_state_t	*global_state,
				      neighbour_t	*neighbour,
				      short		events)
{
	int		available_hosts_size;
	int		needed_conns;
	neighbour_t	*new_neighbour;
	char		text_ip[INET6_ADDRSTRLEN];

	/* fetch hosts with HOST_AVAILABLE flag set;
	 * no allocation with NULL parameter -> result always >=0 */
	available_hosts_size = fetch_specific_hosts(&global_state->hosts,
						    NULL,
						    HOST_AVAILABLE);
	/* initialize text_ip */
	ip_to_string(&neighbour->addr, text_ip);

	/* we've successfully connected to the neighbour */
	if (events & BEV_EVENT_CONNECTED) {
		log_info("%s successfully connected", text_ip);
		/* we've got a new neighbour;
		 * we can't just delete the neighbour from pending
		 * and add it into 'neighbours' as the delete would
		 * free'd the bufferevent
		 */
		new_neighbour = move_neighbour_from_pending(global_state,
							    neighbour);
		if (new_neighbour == NULL) {
			return;
		}
		needed_conns = MIN_NEIGHBOURS -
			       linkedlist_size(&global_state->neighbours);
		/* if we need more neighbours */
		if (needed_conns > 0) {
			/* and we don't have enough available */
			if (available_hosts_size < needed_conns) {
				ask_for_addresses(new_neighbour);
			}
		}
	/* connecting to the neighbour was unsuccessful */
	} else {
		log_debug("process_pending_neighbour - connecting to "
			  "%s was unsuccessful", text_ip);
		/* the host is no longer a pending neighbour */
		delete_neighbour(&global_state->pending_neighbours,
				 neighbour->buffer_event);
	}
}

/**
 * Process the event that occurred on our neighbour.
 *
 * @param	global_state	Global state.
 * @param	neighbour	The event occurred on this neighbour.
 * @param	events		What event occurred.
 */
static void process_neighbour(global_state_t	*global_state,
			      neighbour_t	*neighbour,
			      short		events)
{
	char text_ip[INET6_ADDRSTRLEN];

	/* initialize text_ip */
	ip_to_string(&neighbour->addr, text_ip);

	if (events & BEV_EVENT_ERROR) {
		log_info("Connection error, removing %s", text_ip);
		delete_neighbour(&global_state->neighbours,
				 neighbour->buffer_event);
	} else if (events & BEV_EVENT_EOF) {
		log_info("%s disconnected", text_ip);
		delete_neighbour(&global_state->neighbours,
				 neighbour->buffer_event);
	/* timeout flag on 'bev' */
	} else if (events & BEV_EVENT_TIMEOUT) {
		timeout_process(&global_state->neighbours, neighbour);
	}
}

/**
 * Callback for bufferevent event detection.
 *
 * @param	bev	bufferevent on which the event occurred.
 * @param	events	Flags of the events occurred.
 * @param	ctx	Pointer to global_state_t to determine the neighbour.
 */
static void event_cb(struct bufferevent *bev, short events, void *ctx)
{
	neighbour_t	*neighbour;
	global_state_t	*global_state = (global_state_t *) ctx;

	/* find neighbour with 'bev' */
	neighbour = find_neighbour(&global_state->neighbours, bev);
	if (neighbour != NULL) {
		process_neighbour(global_state, neighbour, events);
	/* no such neighbour found; try finding it at pending_neighbours */
	} else {
		neighbour = find_neighbour(&global_state->pending_neighbours,
					   bev);
		/* 'bev' must belong to either 'neighbours' or
		 * 'pending_neighbours' */
		assert(neighbour != NULL);
		process_pending_neighbour(global_state, neighbour, events);
	}
}

/**
 * Callback function for accepting new connections.
 *
 * @param	listener	Incoming connection.
 * @param	fd		File descriptor for the new connection.
 * @param	address		Routing information.
 * @param	socklen		The size of the address.
 * @param	ctx		Pointer to global_state_t to add new neighbour.
 */
static void accept_connection(struct evconnlistener *listener,
			      evutil_socket_t fd, struct sockaddr *addr,
			      int socklen __attribute__((unused)),
			      void *ctx)
{
	struct event_base	*base;
	struct bufferevent	*bev;
	struct in6_addr		*new_addr;
	host_t			*new_host;
	char			text_ip[INET6_ADDRSTRLEN];
	struct timeval		timeout;

	global_state_t *global_state = (struct s_global_state *) ctx;
	struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) addr;

	/* put binary representation of IP to 'new_addr' */
	new_addr = &addr_in6->sin6_addr;

	ip_to_string(new_addr, text_ip);

	if (find_neighbour_by_addr(&global_state->pending_neighbours,
				   new_addr)) {
		log_debug("accept_connection - peer %s already at "
			  "pending neighbours", text_ip);
		return;
	}

	/* get the event_base */
	base = evconnlistener_get_base(listener);

	/* setup a bufferevent for the new connection */
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

	/* subscribe every received P2P message to be processed;
	 * p2p_process for read callback, NULL for write callback */
	bufferevent_setcb(bev, p2p_process, NULL, event_cb, ctx);

	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_TIMEOUT);

	timeout.tv_sec = TIMEOUT_TIME;
	timeout.tv_usec = 0;

	/* after TIMEOUT_TIME seconds invoke event_cb */
	bufferevent_set_timeouts(bev, &timeout, NULL);

	/* add the new connection to the list of our neighbours */
	if (!add_new_neighbour(&global_state->neighbours,
			       new_addr,
			       bev)) {
		log_debug("accept_connection - adding failed");
		bufferevent_free(bev);
		return;
	}

	log_info("New connection from [%s]:%d", text_ip,
		ntohs(addr_in6->sin6_port));
	if ((new_host = save_host(&global_state->hosts, new_addr))) {
		/* we are now connected to this host, hence unavailable */
		unset_host_flags(new_host, HOST_AVAILABLE);
	}
}

/**
 * Callback for listener error detection.
 *
 * @param	listener	Listener on which the error occurred.
 * @param	ctx		Pointer to global_state_t.
 */
static void accept_error_cb(struct evconnlistener *listener,
			    void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);
	global_state_t *global_state = (struct s_global_state *) ctx;

	int err = EVUTIL_SOCKET_ERROR();

	/* TODO: If we have enough connections, don't stop the event loop */
	log_error("Error %d (%s) on the listener, shutting down",
		  err, evutil_socket_error_to_string(err));

	/* stop the event loop */
	event_base_loopexit(base, NULL);

	/* delete neighbours */
	clear_neighbours(&global_state->neighbours);
}

/**
 * Initialize listening and set up callbacks.
 *
 * @param	listener	The event loop listener.
 * @param	global_state	Data for the event loop to work with.
 *
 * @return	0		If successfully initialized.
 * @return	1		If an error occurred.
 */
int listen_init(struct evconnlistener 	**listener,
		global_state_t		*global_state)
{
	struct event_base **base = &global_state->event_loop;
	struct sockaddr_in6 sock_addr;

	int port = DEFAULT_PORT;

	*base = event_base_new();
	if (!*base) {
		log_error("Creating eventbase");
		return 1;
	}

	/* listening on :: and a default port */
	memset(&sock_addr, 0x0, sizeof(sock_addr));
	sock_addr.sin6_family = AF_INET6;
	sock_addr.sin6_addr = in6addr_any;
	sock_addr.sin6_port = htons(port);

	*listener = evconnlistener_new_bind(*base, accept_connection,
					    global_state,
					    LEV_OPT_CLOSE_ON_FREE |
					    LEV_OPT_REUSEABLE, -1,
					    (struct sockaddr *) &sock_addr,
					    sizeof(sock_addr));

	if (!*listener) {
		log_error("Creating listener");
		return 1;
	}

	evconnlistener_set_error_cb(*listener, accept_error_cb);

	return 0;
}

/**
 * Attempt to connect to a particular addr.
 *
 * @param	global_state	Data for the event loop to work with.
 * @param	addr		Binary IP of a host that we want to connect to.
 *
 * @return	0		The connection attempt was successful.
 * @return	1		The host is already our neighbour.
 * @return	2		The host is pending to become our neighbour.
 * @return	3		Adding new pending neighbour unsuccessful.
 */
int connect_to_addr(global_state_t		*global_state,
		    const struct in6_addr	*addr)
{
	struct bufferevent	*bev;
	host_t			*host;
	struct sockaddr		*sock_addr;
	struct sockaddr_in6	sock_addr6;
	int			sock_len;
	char			text_ip[INET6_ADDRSTRLEN];
	struct timeval		timeout;

	/* get textual representation of the input ip address */
	ip_to_string(addr, text_ip);

	/* don't connect to already connected host */
	if (find_neighbour_by_addr(&global_state->neighbours, addr) != NULL) {
		log_debug("connect_to_addr - host already connected");
		return 1;
	}

	/* don't attempt to connect to already pending connection */
	if (linkedlist_find(&global_state->pending_neighbours, addr) != NULL) {
		log_debug("connect_to_addr - host is in the pending conns");
		return 2;
	}

	memset(&sock_addr6, 0x0, sizeof(sock_addr6));
	sock_addr6.sin6_family = AF_INET6;
	sock_addr6.sin6_port   = htons(DEFAULT_PORT);
	memcpy(&sock_addr6.sin6_addr, addr, 16);

	sock_addr = (struct sockaddr *) &sock_addr6;
	sock_len  = sizeof(sock_addr6);

	/* it is safe to set file descriptor to -1 if we define it later */
	bev = bufferevent_socket_new(global_state->event_loop,
				     -1,
				     BEV_OPT_CLOSE_ON_FREE);

	/* subscribe every received P2P message to be processed;
	 * p2p_process as read callback, NULL as write callback */
	bufferevent_setcb(bev,
			  p2p_process,
			  NULL,
			  event_cb,
			  global_state);

	/* enable bufferevent for read, write and timeout */
	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_TIMEOUT);

	/* set the timeout value */
	timeout.tv_sec = TIMEOUT_TIME;
	timeout.tv_usec = 0;

	/* after TIMEOUT_TIME seconds invoke event_cb */
	bufferevent_set_timeouts(bev, &timeout, &timeout);

	/* add host to the list of pending neighbours and let event_cb
	 * determine whether the host is our neighbour now */
	if (!add_new_neighbour(&global_state->pending_neighbours, addr, bev)) {
		log_debug("connect_to_addr - neighbour %s NOT ADDED into "
			  "pending neighbours", text_ip);
		bufferevent_free(bev);
		return 3;
	} else {
		log_debug("connect_to_addr - neighbour %s ADDED into "
			  "pending neighbours", text_ip);
	}

	host = find_host_by_addr(&global_state->hosts, addr);
	if (host != NULL) {
		unset_host_flags(host, HOST_AVAILABLE);
	}

	/* connect to the host; socket_connect also assigns fd */
	bufferevent_socket_connect(bev, sock_addr, sock_len);

	return 0;
}

/**
 * Attempt to connect to more hosts.
 *
 * @param	global_state	Data for the event loop to work with.
 * @param	conns_amount	Prefered number of new connections.
 */
void add_more_connections(global_state_t *global_state, size_t conns_amount)
{
	struct in6_addr	addr;
	int		available_hosts_size;
	host_t		**available_hosts;
	size_t		cur_conn_attempts = 0;
	size_t		idx;
	neighbour_t	*neigh;
	size_t		result;
	host_t		*selected_host;

	available_hosts_size = fetch_specific_hosts(&global_state->hosts,
						    &available_hosts,
						    HOST_AVAILABLE);
	/* if fetch_specific_hosts had allocation failure */
	if (available_hosts_size == -1) {
		log_error("Adding more connections");
		return;
	}

	/* only if we don't have any non-default hosts available */
	if (available_hosts_size == 0) {
		log_debug("add_more_connections - "
			  "choosing random default host");
		/* choose random default host */
		idx = rand() % DEFAULT_HOSTS_SIZE;
		memcpy(&addr, DEFAULT_HOSTS[idx], 16);

		result = connect_to_addr(global_state, &addr);
		/* the connecting attempt was successful */
		if (result == 0) {
			/* if the host becomes our neighbour,
			 * and we need more connections,
			 * get a list of hosts from them and
			 * attempt to connect to them;
			 * it's our goal to use as few
			 * default hosts as possible
			 */
			log_debug("add_more_connections - "
				  "connect attempt succeeded");
		/* the host is our neighbour; ask them for more addrs */
		} else if (result == 1) {
			neigh = find_neighbour_by_addr(&global_state->
						       neighbours,
						       &addr);
			assert(neigh != NULL);
			ask_for_addresses(neigh);
			log_debug("add_more_connections - "
				  "asking for hosts");
		/* the host is a pending connection */
		} else if (result == 2) {
			/* wait for them to reject/accept us */
			log_debug("add_more_connections - "
				  "pending, do nothing");
		} else {
			log_debug("add_more_connections - "
				  "connect attempt didn't succeed");
		}
	/* we've got some available hosts */
	} else {
		/* we need to choose 'conns_amount' of random connections */
		shuffle_hosts_arr(available_hosts, available_hosts_size);
		/* clamp to 'available_hosts_size' */
		if (conns_amount > (size_t) available_hosts_size) {
			conns_amount = available_hosts_size;
		}

		while (cur_conn_attempts < conns_amount) {
			idx = cur_conn_attempts;
			selected_host = available_hosts[idx];
			/* perform a connection attempt */
			connect_to_addr(global_state,
					&selected_host->addr);
			cur_conn_attempts++;
		}
	}

	free(available_hosts);
}
