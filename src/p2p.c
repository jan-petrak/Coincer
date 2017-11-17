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
 * @param	ctx	optional programmer-defined data to be passed into this
 * 			callback
 */
static void p2p_process(struct bufferevent *bev,
			void *ctx __attribute__((unused)))
{

	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);

	/* FOR TESTING PURPOSES */

	/* Display the input data */
	size_t len = evbuffer_get_length(input);
	char *data = (char *) malloc(len * sizeof(char));
	evbuffer_copyout(input, data, len);
	printf("Sending back: %s", data);
	free(data);

	/* Copy all the data from the input buffer to the output buffer */
	evbuffer_add_buffer(output, input);

	/* TODO/FIXME: this is wrong PoC: the input buffer should be discarded
	 * and the output one filled with our data
	 */
}

/**
 * @brief Callback for bufferevent event detection
 * @param	bev	bufferevent on which the event occured
 * @param	events	flags of the events occured
 * @param	ctx	optional programmer-defined data to be passed into this
 * 			callback
 */
static void event_cb(struct bufferevent *bev, short events,
		     void *ctx __attribute__((unused)))
{
	if (events & BEV_EVENT_ERROR) {
		fprintf(stderr, "Error from bufferevent");
	}
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
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
 * @param	ctx		optional programmer-defined data to be passed
 * 				into this callback; is NULL
 */
static void accept_connection(struct evconnlistener *listener,
			      evutil_socket_t fd, struct sockaddr *addr,
			      int socklen __attribute__((unused)),
			      void *ctx __attribute__((unused)))
{
	/* to display IP address of the other peer */
	char ip[INET6_ADDRSTRLEN + 1];	/* NOTE: not sure if +1 is needed... */

	/* Set up a bufferevent for a new connection */
	struct event_base *base = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(base, fd,
							 BEV_OPT_CLOSE_ON_FREE);

	struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) addr;

	/* subscribe every received P2P message to be processed */
	bufferevent_setcb(bev, p2p_process, NULL, event_cb, NULL);

	bufferevent_enable(bev, EV_READ | EV_WRITE);

	ip_to_string(addr, ip);
	printf("New connection from [%s]:%d\n", ip, ntohs(addr_in6->sin6_port));
}

/**
 * @brief Callback for listener error detection
 * @param	listener	listener on which the error occured
 * @param	ctx		optional programmer-defined data to be passed
 * 				into this callback
 */
static void accept_error_cb(struct evconnlistener *listener,
			    void *ctx __attribute__((unused)))
{
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
}

/**
 * @brief Initialize listening and set up callbacks
 *
 * @param	base	The event loop
 *
 * @return	1 if an error occured
 * @return	0 if successfully initialized
 */
int listen_init(struct event_base **base)
{
	struct evconnlistener *listener;
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

	listener = evconnlistener_new_bind(*base, accept_connection, NULL,
					   LEV_OPT_CLOSE_ON_FREE |
					   LEV_OPT_REUSEABLE, -1,
					   (struct sockaddr *) &sock_addr,
					   sizeof(sock_addr));
	if (!listener) {
		perror("Couldn't create listener: evconnlistener_new_bind");
		return 1;
	}
	evconnlistener_set_error_cb(listener, accept_error_cb);
	
	return 0;
}
