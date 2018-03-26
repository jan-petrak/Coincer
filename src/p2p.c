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

#include "linkedlist.h"
#include "neighbours.h"
#include "p2p.h"

/**
 * Simple helper for conversion of binary IP to readable IP address.
 *
 * @param	binary_ip	Binary represented IP address.
 * @param	ip		Readable IP address.
 */
static void ip_to_string(unsigned char *binary_ip, char *ip)
{
	inet_ntop(AF_INET6, binary_ip, ip, INET6_ADDRSTRLEN);
}

/**
 * Processing a P2P message.
 *
 * @param	bev	Buffer to read data from.
 * @param	ctx	Pointer to global_state_t to process message sender.
 */
static void p2p_process(struct bufferevent *bev, void *ctx)
{
	char 		*data;
	global_state_t	*global_state;
	struct evbuffer	*input;
	size_t		len;
	neighbour_t	*neighbour;
	struct evbuffer	*output;

	global_state = (global_state_t *) ctx;

	/* find the neighbour based on their buffer event */
	neighbour = find_neighbour(&global_state->neighbours, bev);

	/* message from unknown neighbour; quit p2p_process */
	if (neighbour == NULL) {
		return;
	}

	/* refresh neighbour's failed pings */
	neighbour->failed_pings = 0;

	/* read from the input buffer, write to output buffer */
	input	= bufferevent_get_input(bev);
	output	= bufferevent_get_output(bev);

	/* get length of the input messaage */
	len = evbuffer_get_length(input);

	/* allocate memory for the input message including '\0' */
	data = (char *) malloc((len + 1) * sizeof(char));

	/* FOR TESTING PURPOSES */

	/* drain input buffer into data; -1 if evbuffer_remove failed */
	if (evbuffer_remove(input, data, len) == -1) {
		free(data);
		return;
	} else {
		data[len] = '\0';
	}	

	printf("Sending back: %s", data);

	/* copy string data to the output buffer */
	evbuffer_add_printf(output, "%s", data);

	/* deallocate input message */
	free(data);
}

/**
 * Process the timeout invoked by event_cb.
 *
 * @param	neighbours	Linked list of neighbours.
 * @param	neighbour	Timeout invoked on this neighbour.
 */
static void timeout_process(linkedlist_t	*neighbours,
			    neighbour_t		*neighbour)
{
	char text_ip[INET6_ADDRSTRLEN];

	/* initialize text_ip */
	ip_to_string(neighbour->ip_addr, text_ip);

	/* the neighbour hasn't failed enough pings to be deleted */
	if (neighbour->failed_pings < 3) {
		/* bufferevent was disabled when timeout flag was set */
		bufferevent_enable(neighbour->buffer_event,
			EV_READ | EV_WRITE | EV_TIMEOUT);

		printf("Sending ping to %s. Failed pings: %lu\n",
			text_ip, neighbour->failed_pings);

		/* send ping to the inactive neighbour;
		 * 5 is the length of bytes to be transmitted
		 */
		evbuffer_add(bufferevent_get_output(neighbour->buffer_event),
			"ping", 5);

		neighbour->failed_pings++;
	} else {
		printf("3 failed pings. Removing %s from neighbours\n",
			text_ip);
		delete_neighbour(neighbours, neighbour->buffer_event);
	}
}

/**
 * Callback for bufferevent event detection.
 *
 * @param	bev	bufferevent on which the event occured.
 * @param	events	Flags of the events occured.
 * @param	ctx	Pointer to global_state_t to determine the neighbour.
 */
static void event_cb(struct bufferevent *bev, short events, void *ctx)
{
	neighbour_t	*neighbour;
	char		text_ip[INET6_ADDRSTRLEN];

	global_state_t *global_state = (global_state_t *) ctx;

	/* find neighbour with 'bev' */
	neighbour = find_neighbour(&global_state->neighbours, bev);

	/* unknown neighbour; release 'bev' and stop processing the event */
	if (neighbour == NULL) {
		bufferevent_free(bev);
		return;
	}

	/* initialize text_ip */
	ip_to_string(neighbour->ip_addr, text_ip);

	if (events & BEV_EVENT_ERROR) {
		perror("Error from bufferevent");
		printf("%s was removed\n", text_ip);
		delete_neighbour(&global_state->neighbours, bev);
		return;
	}
	if (events & (BEV_EVENT_EOF)) {
		printf("%s disconnected\n", text_ip);
		delete_neighbour(&global_state->neighbours, bev);
		return;
	}

	/* timeout flag on 'bev' */
	if (events & BEV_EVENT_TIMEOUT) {
		timeout_process(&global_state->neighbours, neighbour);
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
	unsigned char		inet6_ip[sizeof(struct in6_addr)];
	struct timeval		read_timeout;
	char			text_ip[INET6_ADDRSTRLEN];
	struct timeval		write_timeout;

	global_state_t *global_state = (struct s_global_state *) ctx;
	struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) addr;

	/* get the event_base */
	base = evconnlistener_get_base(listener);

	/* setup a bufferevent for the new connection */
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

	/* subscribe every received P2P message to be processed */
	bufferevent_setcb(bev, p2p_process, NULL, event_cb, ctx);

	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_TIMEOUT);

	/* after READ_TIMEOUT or WRITE_TIMEOUT seconds invoke event_cb */
	write_timeout.tv_sec = WRITE_TIMEOUT;
	write_timeout.tv_usec = 0;

	read_timeout.tv_sec = READ_TIMEOUT;
	read_timeout.tv_usec = 0;

	bufferevent_set_timeouts(bev, &read_timeout, &write_timeout);

	/* put binary representation of IP to inet6_ip */
	memcpy(inet6_ip, addr_in6->sin6_addr.s6_addr, sizeof(struct in6_addr));

	/* add the new connection to the list of our neighbours */
	if (!add_new_neighbour(&global_state->neighbours,
			       inet6_ip,
			       bev)) {
		/* free the bufferevent if adding failed */
		bufferevent_free(bev);
		return;
	}

	ip_to_string(inet6_ip, text_ip);

	printf("New connection from [%s]:%d\n", text_ip,
		ntohs(addr_in6->sin6_port));
}

/**
 * Callback for listener error detection.
 *
 * @param	listener	Listener on which the error occured.
 * @param	ctx		Pointer to global_state_t.
 */
static void accept_error_cb(struct evconnlistener *listener,
			    void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);
	global_state_t *global_state = (struct s_global_state *) ctx;

	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));

	/* TODO: Handling of error with listener */

	/* stop the event loop */
	event_base_loopexit(base, NULL);

	/* delete neighbours */
	clear_neighbours(&global_state->neighbours);
}
/**
 * Initialize listening and set up callbacks.
 *
 * @param	listener	The even loop listener.
 * @param	global_state	Data for the event loop to work with.
 *
 * @return	0 if successfully initialized.
 * @return	1 if an error occured.
 */
int listen_init(struct evconnlistener 	**listener,
		global_state_t		*global_state)
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

	*listener = evconnlistener_new_bind(*base, accept_connection,
					   global_state,
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
