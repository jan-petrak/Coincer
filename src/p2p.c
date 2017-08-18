#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "p2p.h"

/**
 * @brief callback for reading an input buffer
 * @param bev buffer to read data from
 * @param ctx optional programmer-defined data to be passed into this callback
 */
static void read_cb(struct bufferevent *bev, void *ctx)
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
}

/**
 * @brief callback for bufferevent event detection
 * @param bev bufferevent on which the event occured
 * @param events flags of the events occured
 * @param ctx optional programmer-defined data to be passed into this callback
 */
static void event_cb(struct bufferevent *bev, short events, void *ctx)
{

	if (events & BEV_EVENT_ERROR) {
		perror("Error from bufferevent");
	}
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
	}
}

/**
 * @brief callback for accepting a new connection
 * @param listener incoming connection
 * @param fd file descriptor for the new connection
 * @param address routing information
 * @param socklen size of address
 * @param ctx optional programmer-defined data to be passed into this callback
 */
static void accept_conn_cb(struct evconnlistener *listener,
	evutil_socket_t fd, struct sockaddr *address, int socklen,
	void *ctx)
{
	/* Set up a bufferevent for a new connection */
	struct event_base *base = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(
		base, fd, BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);

	bufferevent_enable(bev, EV_READ|EV_WRITE);


	/* Display IP address of the one connecting to us */
	char *s = NULL;

	switch (address->sa_family) {
		case AF_INET: {
			struct sockaddr_in *addr_in = (struct sockaddr_in *) address;
			s = malloc(INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);
			break;
		}
		case AF_INET6: {
			struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *) address;
			s = malloc(INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &(addr_in6->sin6_addr), s, INET6_ADDRSTRLEN);
			break;
		}
		default:
			break;
	}

	printf("%s has connected to you.\n", s);
	free(s);
}

/**
 * @brief callback for listener error detection
 * @param listener listener on which the error occured
 * @param ctx optional programmer-defined data to be passed into this callback
 */
static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
}

int listen_init(struct event_base **base)
{

	struct evconnlistener *listener;
	struct sockaddr_in sin;

	int port = 31070;

	*base = event_base_new();
	if (!*base) {
		puts("Couldn't open event base");
		return 1;
	}

	memset(&sin, 0x0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0);
	sin.sin_port = htons(port);

	listener = evconnlistener_new_bind(*base, accept_conn_cb, NULL,
		LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
		(struct sockaddr*)&sin, sizeof(sin));
	if (!listener) {
		perror("Couldn't create listener");
		return 1;
	}
	evconnlistener_set_error_cb(listener, accept_error_cb);
	
	return 0;
}
