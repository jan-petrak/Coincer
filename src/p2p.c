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

#include <arpa/inet.h>		/* inet_ntop */
#include <errno.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <netinet/in.h>		/* sockaddr_in6 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p2p.h"

/**
 * Processing a P2P message.
 *
 * @brief Callback for reading an input buffer
 * @param	bev	buffer to read data from
 * @param	ctx	struct s_global_state *
 */
static void p2p_process(struct bufferevent *bev, void *ctx)
{
	/* Using this we can know who's sent us the message and
	   reset their amount of failed pings */
	struct s_global_state *global_state = (struct s_global_state *) ctx;

	/* Find the neighbour based on their buffer event */
	struct Neighbour *neighbour =
		find_neighbour(&global_state->neighbours, bev);

	/* Read from input buffer, write to output buffer */
	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);

	size_t len = evbuffer_get_length(input);
	char *data = (char *) malloc(len * (sizeof(char) + 1));

	/* FOR TESTING PURPOSES */

	/* Drain input buffer into data */
	if (evbuffer_remove(input, data, len) == -1) {
		free(data);
		return;
	} else {
		data[len] = '\0';
	}

	if (neighbour == NULL) {
		free(data);
		return;
	}

	neighbour->failed_pings = 0;

	printf("Sending back: %s", data);

	/* Copy string data to the output buffer */
	evbuffer_add_printf(output, "%s", data);
	free(data);
}

static void timeout_process(struct Neighbours *neighbours,
			    struct Neighbour *neighbour)
{
	if (neighbour->failed_pings < 3) {
		/* bufferevent was disabled when timeout flag was set */
		bufferevent_enable(neighbour->buffer_event,
			EV_READ | EV_WRITE | EV_TIMEOUT);

		printf("Sending ping to %s. Failed pings: %lu\n",
			neighbour->ip_addr, neighbour->failed_pings);

		/* Send ping to the inactive neighbour.
		   5 is the length of bytes to be transmitted */
		evbuffer_add(bufferevent_get_output(neighbour->buffer_event),
			"ping", 5);

		neighbour->failed_pings++;
	} else {
		printf("3 failed pings. Removing %s from neighbours\n",
			neighbour->ip_addr);
		delete_neighbour(neighbours, neighbour->buffer_event);
	}
}

/**
 * @brief Callback for bufferevent event detection
 * @param	bev	bufferevent on which the event occured
 * @param	events	flags of the events occured
 * @param	ctx	struct s_global_state *
 */
static void event_cb(struct bufferevent *bev, short events, void *ctx)
{
	struct s_global_state *global_state = (struct s_global_state *) ctx;
	struct Neighbour *neighbour =
		find_neighbour(&global_state->neighbours, bev);

	if (neighbour == NULL) {
		return;
	}

	if (events & BEV_EVENT_ERROR) {
		perror("Error from bufferevent");
		printf("Removing %s from neighbours.\n", neighbour->ip_addr);
		delete_neighbour(&global_state->neighbours, bev);
	}
	if (events & (BEV_EVENT_EOF)) {
		printf("%s has disconnected.\n", neighbour->ip_addr);
		delete_neighbour(&global_state->neighbours, bev);
	}
	if (events & BEV_EVENT_TIMEOUT) {
		/* Bufferevent bev received timout event */
		timeout_process(&global_state->neighbours, neighbour);
	}
}

/**
 * Simple helper for conversion of sockaddr to a textual IP address.
 *
 * @param	addr	sockaddr data
 * @param	ip	Destination string buffer
 */
static void ip_to_string(struct sockaddr *addr, char *ip)
{
	struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) addr;
	inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip, INET6_ADDRSTRLEN);
}

/**
 * Callback function. It handles accepting new connections.
 *
 * @brief Callback for accepting a new connection
 * @param	listener	incoming connection
 * @param	fd		file descriptor for the new connection
 * @param	address		routing information
 * @param	socklen		size of address
 * @param	ctx		struct s_global_state *
 */
static void accept_connection(struct evconnlistener *listener,
			      evutil_socket_t fd, struct sockaddr *addr,
			      int socklen __attribute__((unused)),
			      void *ctx)
{
	/* We need to add the new connection to the list of our neighbours */
	struct s_global_state *global_state = (struct s_global_state *) ctx;
	struct timeval write_timeout;
	struct timeval read_timeout;

	/* To display IP address of the other peer */
	char ip[INET6_ADDRSTRLEN];

	/* Bufferevent for a new connection */
	struct event_base *base;
	struct bufferevent *bev;

	/* Set up the bufferevent for a new connection */
	base = evconnlistener_get_base(listener);
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

	struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) addr;

	/* subscribe every received P2P message to be processed */
	bufferevent_setcb(bev, p2p_process, NULL, event_cb, ctx);

	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_TIMEOUT);

	/* after READ_TIMEOUT or WRITE_TIMEOUT seconds invoke event_cb */
	write_timeout.tv_sec = WRITE_TIMEOUT;
	write_timeout.tv_usec = 0;

	read_timeout.tv_sec = READ_TIMEOUT;
	read_timeout.tv_usec = 0;

	bufferevent_set_timeouts(bev, &read_timeout, &write_timeout);

	ip_to_string(addr, ip);

	/* Add the new connection to the list of our neighbours */
	if (!add_new_neighbour(&global_state->neighbours, ip, bev)) {
		bufferevent_free(bev);
		return;
	}

	printf("New connection from [%s]:%d\n", ip, ntohs(addr_in6->sin6_port));
}

/**
 * @brief Callback for listener error detection
 * @param	listener	listener on which the error occured
 * @param	ctx		struct s_global_state *
 */
static void accept_error_cb(struct evconnlistener *listener,
			    void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);
	struct s_global_state *global_state = (struct s_global_state *) ctx;
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
	clear_neighbours(&global_state->neighbours);
}

/**
 * @brief Initialize listening and set up callbacks
 *
 * @param	listener	The even loop listener
 * @param	global_state	Data for the event loop to work with
 *
 * @return	1 if an error occured
 * @return	0 if successfully initialized
 */
int listen_init(struct evconnlistener 	**listener,
		struct s_global_state 	*global_state)
{
	struct event_base **base = &global_state->event_loop;
	struct sockaddr_in6 sock_addr;

	int port = DEFAULT_PORT;

	*base = event_base_new();
	if (!*base) {
		puts("Couldn't open event base");
		return 1;
	}

	/* listening on :: and a default port */
	memset(&sock_addr, 0x0, sizeof(sock_addr));
	sock_addr.sin6_family = AF_INET6;
	sock_addr.sin6_addr = in6addr_any;
	sock_addr.sin6_port = htons(port);

	*listener = evconnlistener_new_bind(*base, accept_connection, global_state,
					   LEV_OPT_CLOSE_ON_FREE |
					   LEV_OPT_REUSEABLE, -1,
					   (struct sockaddr *) &sock_addr,
					   sizeof(sock_addr));

	if (!*listener) {
		perror("Couldn't create listener: evconnlistener_new_bind");
		return 1;
	}

	evconnlistener_set_error_cb(*listener, accept_error_cb);

	return 0;
}
