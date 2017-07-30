#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "p2p.h"

static void read_cb(struct bufferevent *bev, void *ctx)
{

	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);

	/* Copy all the data from the input buffer to the output buffer. */
	evbuffer_add_buffer(output, input);
}

static void event_cb(struct bufferevent *bev, short events, void *ctx)
{

	if (events & BEV_EVENT_ERROR) {
		perror("Error from bufferevent");
	}
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
	}
}

static void accept_conn_cb(struct evconnlistener *listener,
	evutil_socket_t fd, struct sockaddr *address, int socklen,
	void *ctx)
{
	/* Set up a bufferevent for a new connection. */
	struct event_base *base = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(
		base, fd, BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);

	bufferevent_enable(bev, EV_READ|EV_WRITE);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
}

int listen_init()
{

	struct event_base *base;
	struct evconlistener *listener;
	struct sockaddr_in sin;

	int port = 31070;

	base = event_base_new();
	if (!base) {
		puts("Couldn't open event base");
		return 1;
	}

	memset(&sin, 0x0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(0);
	sin.sin_port = htons(port);

	listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
		LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
		(struct sockaddr*)&sin, sizeof(sin));
	if (!listener) {
		perror("Couldn't create listener");
		return 1;
	}
	evconnlistener_set_error_cb(listener, accept_error_cb);

	event_base_dispatch(base);

	return 0;
}
